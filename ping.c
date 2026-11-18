#include "ping.h"
#include "terminal_output.h"

#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static uint32_t fold_checksum(uint32_t sum) {
    while (sum >> 16) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return sum;
}

static uint32_t do_sum(const void *obj, size_t byte_size) {
    uint32_t sum = 0;
    const uint8_t *bytes = (const uint8_t *)obj;

    for (size_t i = 0; i + 1 < byte_size; i += 2) {
        sum += ((uint32_t)bytes[i] << 8) | bytes[i + 1];
    }
    if (byte_size % 2 != 0) {
        sum += ((uint32_t)bytes[byte_size - 1] << 8);
    }
    return fold_checksum(sum);
}

uint16_t get_checksum(const void *obj, size_t byte_size) {
    uint32_t sum = do_sum(obj, byte_size);
    return (uint16_t)(~sum & 0xFFFFu);
}

bool verify_checksum(const void *obj, size_t byte_size) {
    uint32_t sum = do_sum(obj, byte_size);
    return sum == 0xFFFFu;
}

icmp_packet_t create_echo_request(ping_config_t *config) {
    icmp_packet_t packet = {0};

    packet.type = ICMP_ECHO_REQUEST;
    packet.code = 0;
    packet.process_id = htons(config->process_id);
    packet.sequence_id = htons(config->sequence_count++);
    packet.checksum = 0;
    packet.checksum = htons(get_checksum(&packet, sizeof(packet)));
    libassert(verify_checksum(&packet, sizeof(packet)),
		"create_echo_request: invalid get/verify checksum parallel");
    return packet;
}

bool send_echo_request(ping_config_t *config, bool unsent_packet) {
    static icmp_packet_t packet;
    struct sockaddr_in dest_addr = {
        .sin_family = AF_INET,
        .sin_addr = config->destination_addr,
        .sin_port = 0,
        .sin_zero = {}};

    if (BRANCH_UNLIKELY(!unsent_packet)) {
        packet = create_echo_request(config);
    }
    memcpy(config->send_buffer, &packet, sizeof(packet));
    ssize_t sent_bytes = sendto(
        config->socket_fd,
        config->send_buffer,
        config->payload_size + sizeof(icmp_packet_t),
        0,
        (struct sockaddr *)&dest_addr,
        sizeof(dest_addr));
    if (BRANCH_UNLIKELY(sent_bytes == -1 &&
        (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))) {
        return true;
    }
    error_if(sent_bytes == -1, SENDTO_ERR, strerror(errno));
    uint64_t now = get_time_us();
    uint16_t packet_sequence_id = ntohs(packet.sequence_id);
    write_element_at_index(
        &config->stored_transactions,
        packet_sequence_id,
        &(ping_transaction_t){
            .sequence_id = packet_sequence_id,
            .send_time = now});
    config->next_send_time = now + config->interval;
    return false;
}

static icmp_packet_t* parse_ipv4_icmp(ping_config_t *config,
                            const uint8_t *buffer,
                           size_t buffer_length) {
    if (buffer_length < sizeof(struct iphdr)){
        print_message(PACKET_TOO_SHORT, buffer_length, config->destination_str);
        return NULL;
    }
    
    struct iphdr *ip = (struct iphdr *)buffer;

    if (ip->version != 4) {
        print_message(VERSION_INVALID);
        return NULL;
    }

    size_t ip_header_length = ip->ihl * 4;

    if (ip_header_length < sizeof(struct iphdr)) {
        print_message(PACKET_TOO_SHORT, buffer_length, config->destination_str);
        return NULL;
    }
    if (buffer_length < ip_header_length || ip_header_length > 60) {
        print_message(MALFORMED_PACKET, config->destination_str);
        return NULL;
    }

    uint16_t total_length = ntohs(ip->tot_len);

    if (total_length < ip_header_length + sizeof(icmp_packet_t)) {
        print_message(PACKET_TOO_SHORT, buffer_length, config->destination_str);
        return NULL;
    }

    if (ip->protocol != IPPROTO_ICMP) {
        return NULL;
    }

    uint16_t fragment_info = ntohs(ip->frag_off);

    if ((fragment_info & IP_OFFMASK) != 0 ||
        (fragment_info & IP_MF) != 0) {
        print_message(MALFORMED_PACKET, config->destination_str);
        return NULL;
    }

    return (icmp_packet_t *)(buffer + ip_header_length);
}

static char* addr_to_str(void* addr) {
    static char dest[INET_ADDRSTRLEN];
    error_if(!inet_ntop(AF_INET, addr, dest, INET_ADDRSTRLEN),
        INET_NTOP_ERR, strerror(errno));
    return dest;
}

static void err_transaction(ping_config_t *config, icmp_packet_t *icmp, size_t bytes) {
    struct iphdr* ip = (struct iphdr *)((uint8_t *)icmp);
    icmp_packet_t *origin = parse_ipv4_icmp(config, (uint8_t*)ip, bytes);
    if (origin == NULL) {
        return;
    }
    if (ntohs(origin->process_id) == config->process_id) {
        pop_element_at_index(&config->stored_transactions, ntohs(origin->sequence_id));
    }
}

void process_packet(ping_config_t *config, uint8_t *buffer, size_t bytes) {
    icmp_packet_t *icmp = parse_ipv4_icmp(config, buffer, bytes);
    struct iphdr* ip = (struct iphdr *)buffer;
    
    if(icmp == NULL) {
        return;
    }
    uint16_t total_length = ntohs(ip->tot_len);
    if (total_length > bytes) {
        print_message(MALFORMED_PACKET, config->destination_str);
        return;
    }
    char *rsp_src = addr_to_str(&ip->saddr);
    switch (icmp->type) {
        case ICMP_UNREACHABLE:
            err_transaction(config,
                icmp + sizeof(icmp_packet_t),
                bytes - ((void*)icmp - (void*)ip) - sizeof(icmp_packet_t));
                return;
        case ICMP_TIME_EXCEEDED:
            err_transaction(config,
                icmp,
                bytes - ((void*)icmp - (void*)ip) - sizeof(icmp_packet_t));
                return;
        case ICMP_ECHO_REPLY: break;
        default: return;
    }

    uint16_t packet_sequence_id = ntohs(icmp->sequence_id);
    uint16_t packet_process_id = ntohs(icmp->process_id);

    ping_transaction_t const* transaction = read_element_at_index(
        &config->stored_transactions,
        packet_sequence_id);
    if (transaction == NULL ||
        transaction->sequence_id != packet_sequence_id ||
        config->process_id != packet_process_id ||
        !verify_checksum(icmp, bytes - ((void*)icmp - (void*)buffer))) {
        return;
    }
    double time_diff = (double)(get_time_us() - transaction->send_time) / 1000.0;
    print_message("%ld bytes from %s: icmp_seq=%d ttl=%u time=%.3f ms\n",
        bytes - ((void*)icmp - (void*)buffer),
        rsp_src,
        packet_sequence_id,
        ip->ttl,
        time_diff);
    if (config->min_time > time_diff) {
        config->min_time = time_diff;
    }
    if (config->max_time < time_diff) {
        config->max_time = time_diff;
    }
    config->sum_time += time_diff;
    config->sum_time_sq += time_diff * time_diff;
    pop_element_at_index(&config->stored_transactions, packet_sequence_id);
    config->response_count++;
}

void read_and_process_packets(ping_config_t *config)
{
    static uint8_t buffer[MAX_READ_SIZE];
    while (true) {
        ssize_t bytes = recvfrom(config->socket_fd,
                                 buffer,
                                 sizeof(buffer),
                                 0,
                                 NULL,
                                 NULL);

        if (BRANCH_UNLIKELY(bytes == -1 &&
            (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))) {
            return;
        }
        error_if(bytes == -1, RECVFROM_ERR, strerror(errno));
        process_packet(config, buffer, bytes);
    }
}

void get_destination_addr(ping_config_t *config) {
	if (!inet_aton(config->hostname_str, &config->destination_addr)) {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = 0;
        hints.ai_protocol = IPPROTO_ICMP;
		struct addrinfo *result = NULL;
		int ret = getaddrinfo(config->hostname_str, NULL, &hints, &result);
		error_if(ret != 0, UNKNOWN_NAME_ERR, config->hostname_str);
		config->destination_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr;
        config->destination_str = malloc(INET_ADDRSTRLEN);
        error_if(!config->destination_str, ALLOC_ERR);
        error_if(!inet_ntop(AF_INET, &config->destination_addr, config->destination_str, INET_ADDRSTRLEN),
            INET_NTOP_ERR, strerror(errno));
		freeaddrinfo(result);
	} else {
        config->destination_str = strdup(config->hostname_str);
    }
}

void init_config(ping_config_t *config) {
	config->socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	error_if(config->socket_fd == -1, SOCKET_ERR, strerror(errno));
	if (config->destination_addr.s_addr == 0) {
		get_destination_addr(config);
	}
    if (config->ttl != 0) {
        error_if(setsockopt(config->socket_fd,
                    IPPROTO_IP,
                    IP_TTL,
                    &config->ttl,
                    sizeof(config->ttl)) < 0,
                SETSOCKOPT_ERR,
                "IP_TTL",
                strerror(errno));
    }
    int flags = fcntl(config->socket_fd, F_GETFL);
    fcntl(config->socket_fd, F_SETFL, flags | O_NONBLOCK);
}