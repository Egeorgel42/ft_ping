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

static char* addr_to_str(void const* addr) {
    static char dest[INET_ADDRSTRLEN];
    error_if(!inet_ntop(AF_INET, addr, dest, INET_ADDRSTRLEN),
        INET_NTOP_ERR, strerror(errno));
    return dest;
}

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

static icmp_packet_t*
parse_ipv4_icmp(const uint8_t *buffer,
                size_t buffer_length,
                struct sockaddr_in const *packet_source) {
    if (buffer_length < sizeof(struct iphdr)){
        print_error(PACKET_TOO_SHORT, buffer_length, addr_to_str(&packet_source->sin_addr));
        return NULL;
    }
    
    struct iphdr *ip = (struct iphdr *)buffer;

    if (ip->version != 4) {
        print_error(VERSION_INVALID);
        return NULL;
    }

    size_t ip_header_length = ip->ihl * 4;

    if (ip_header_length < sizeof(struct iphdr)) {
        print_error(PACKET_TOO_SHORT, buffer_length, addr_to_str(&packet_source->sin_addr));
        return NULL;
    }
    if (buffer_length < ip_header_length + sizeof(icmp_packet_t) || ip_header_length > 60) {
        print_error(MALFORMED_PACKET, addr_to_str(&packet_source->sin_addr));
        return NULL;
    }

    uint16_t total_length = ntohs(ip->tot_len);

    if (total_length < ip_header_length + sizeof(icmp_packet_t)) {
        print_error(PACKET_TOO_SHORT, buffer_length, addr_to_str(&packet_source->sin_addr));
        return NULL;
    }

    if (ip->protocol != IPPROTO_ICMP) {
        return NULL;
    }

    uint16_t fragment_info = ntohs(ip->frag_off);

    if ((fragment_info & IP_OFFMASK) != 0 ||
        (fragment_info & IP_MF) != 0) {
        print_error(MALFORMED_PACKET, addr_to_str(&packet_source->sin_addr));
        return NULL;
    }

    return (icmp_packet_t *)(buffer + ip_header_length);
}

static void print_unreachable_error(uint8_t code) {
    switch (code) {
        case NETWORK_UNREACHABLE:
            print_message(NETWORK_UNREACHABLE_ERR);
            return;
        case HOST_UNREACHABLE:
            print_message(HOST_UNREACHABLE_ERR);
            return;
        case PROTOCOL_UNREACHABLE:
            print_message(PROTOCOL_UNREACHABLE_ERR);
            return;
        case PORT_UNREACHABLE:
            print_message(PORT_UNREACHABLE_ERR);
            return;
        case FRAGMENTATION_REQUIRED:
            print_message(FRAGMENTATION_REQUIRED_ERR);
            return;
        case SOURCE_ROUTE_FAILED:
            print_message(SOURCE_ROUTE_FAILED_ERR);
            return;
        case NETWORK_UNKNOWN:
            print_message(NETWORK_UNKNOWN_ERR);
            return;
        case HOST_UNKNOWN:
            print_message(HOST_UNKNOWN_ERR);
            return;
        case SOURCE_HOST_ISOLATED:
            print_message(SOURCE_HOST_ISOLATED_ERR);
            return;
        case NETWORK_UNREACHABLE_FOR_TOS:
            print_message(NETWORK_UNREACHABLE_FOR_TOS_ERR);
            return;
        case HOST_UNREACHABLE_FOR_TOS:
            print_message(HOST_UNREACHABLE_FOR_TOS_ERR);
            return;
        case COMMUNICATION_ADMINISTRATIVELY_PROHIBITED:
            print_message(COMMUNICATION_ADMINISTRATIVELY_PROHIBITED_ERR);
            return;
        case HOST_PRECEDENCE_VIOLATION:
            print_message(HOST_PRECEDENCE_VIOLATION_ERR);
            return;
        case PRECEDENCE_CUTOFF_IN_EFFECT:
            print_message(PRECEDENCE_CUTOFF_IN_EFFECT_ERR);
            return;
        default:
            print_message(UNKNOWN_UNREACHABLE_ERR, code);
            return;
    }
}

static void print_time_exceeded_error(uint8_t code) {
    switch (code) {
        case 0:
            print_message(TIME_EXCEEDED_ERR);
            return;
        case 1:
            print_message(FRAG_REASSEMBLY_TIME_EXCEEDED_ERR);
            return;
        default:
            print_message(UNKNOWN_TIME_EXCEEDED_ERR, code);
            return;
    }
}

static void print_transaction_error(icmp_packet_t *icmp_packet) {
    switch (icmp_packet->type) {
        case ICMP_UNREACHABLE:
            print_unreachable_error(icmp_packet->code);
            return;
        case ICMP_TIME_EXCEEDED:
            print_time_exceeded_error(icmp_packet->code);
            return;
        default:
            print_message(UNKNOWN_ICMP_TYPE_ERR, icmp_packet->type);
            return;
    }
}

static void print_verbose_error_dump(ping_config_t *config, struct iphdr *iphdr_payload, icmp_packet_t *icmp_payload) {
    if (!(config->mode & PING_VERBOSE)) {
        return;
    }
    size_t ip_header_length = iphdr_payload->ihl * 4;
    uint16_t fragment_info = ntohs(iphdr_payload->frag_off);
    char source[INET_ADDRSTRLEN];
    char destination[INET_ADDRSTRLEN];

    if (!inet_ntop(AF_INET, &iphdr_payload->saddr,
                   source, sizeof(source)) ||
        !inet_ntop(AF_INET, &iphdr_payload->daddr,
                   destination, sizeof(destination))) {
        return;
    }

    print_message("IP Hdr Dump:\n ");
    for (size_t i = 0; i < sizeof(struct iphdr); i++) {
        print_message("%02x%s",
            ((uint8_t const *)iphdr_payload)[i],
            (i % 2) ? " " : "");
    }

    print_message(
        "\nVr HL TOS  Len   ID Flg  off TTL Pro  cks      Src\tDst\tData\n");
    print_message(
        " %1x  %1x  %02x %04x %04x   %1x %04x  %02x  %02x %04x %s  %s",
        iphdr_payload->version,
        iphdr_payload->ihl,
        iphdr_payload->tos,
        ntohs(iphdr_payload->tot_len),
        ntohs(iphdr_payload->id),
        fragment_info >> 13,
        fragment_info & IP_OFFMASK,
        iphdr_payload->ttl,
        iphdr_payload->protocol,
        ntohs(iphdr_payload->check),
        source,
        destination);

    for (size_t i = sizeof(struct iphdr); i < ip_header_length; i++) {
        print_message("%02x%s",
            ((uint8_t const *)iphdr_payload)[i],
            ((i + 1) % 2) ? "" : " ");
    }

    print_message(
        "\nICMP: type %u, code %u, size %zu, id 0x%04x, seq 0x%04x\n",
        icmp_payload->type,
        icmp_payload->code,
        ntohs(iphdr_payload->tot_len) - ip_header_length,
        ntohs(icmp_payload->process_id),
        ntohs(icmp_payload->sequence_id));
}

static void err_transaction(ping_config_t *config,
                            struct iphdr const *outer_ip,
                            icmp_packet_t *icmp_packet,
                            size_t icmp_length,
                            struct sockaddr_in const *packet_source) {
    if (icmp_length < sizeof(*icmp_packet)) {
        return;
    }
    uint8_t *quoted_ip_bytes =
        (uint8_t *)icmp_packet + sizeof(*icmp_packet);
    size_t quoted_length = icmp_length - sizeof(*icmp_packet);

    icmp_packet_t *origin =
        parse_ipv4_icmp(quoted_ip_bytes, quoted_length, packet_source);
    if (origin == NULL) {
        return;
    }

    struct iphdr *quoted_ip = (struct iphdr *)quoted_ip_bytes;
    uint16_t sequence_id = ntohs(origin->sequence_id);

    if (origin->type != ICMP_ECHO_REQUEST ||
        origin->code != 0 ||
        ntohs(origin->process_id) != config->process_id ||
        quoted_ip->daddr != config->destination_addr.s_addr) {
        return;
    }

    ping_transaction_t const *transaction = read_element_at_index(
        &config->stored_transactions,
        sequence_id);
    if (transaction == NULL ||
        transaction->sequence_id != sequence_id) {
        return;
    }

    print_message("%zu bytes from %s: ",
        icmp_length,
        addr_to_str(&outer_ip->saddr));
    print_transaction_error(icmp_packet);
    print_verbose_error_dump(config, quoted_ip, origin);
    pop_element_at_index(&config->stored_transactions, sequence_id);
}

void process_packet(ping_config_t *config,
                    uint8_t *buffer,
                    size_t bytes,
                    struct sockaddr_in const *packet_source) {
    icmp_packet_t *icmp = parse_ipv4_icmp(buffer, bytes, packet_source);
    struct iphdr *ip = (struct iphdr *)buffer;

    if (icmp == NULL) {
        return;
    }

    uint16_t total_length = ntohs(ip->tot_len);
    if (total_length > bytes) {
        print_error(MALFORMED_PACKET, addr_to_str(&packet_source->sin_addr));
        return;
    }

    size_t ip_header_length = (uint8_t *)icmp - buffer;
    size_t icmp_length = total_length - ip_header_length;

    if (!verify_checksum(icmp, icmp_length)) {
        print_error("checksum mismatch from %s\n",
            addr_to_str(&ip->saddr));
        return;
    }
    if (icmp->type == ICMP_ECHO_REQUEST) {
        return;
    }
    if (icmp->type != ICMP_ECHO_REPLY) {
        err_transaction(config, ip, icmp, icmp_length, packet_source);
        return;
    }

    uint16_t packet_sequence_id = ntohs(icmp->sequence_id);
    uint16_t packet_process_id = ntohs(icmp->process_id);

    ping_transaction_t const *transaction = read_element_at_index(
        &config->stored_transactions,
        packet_sequence_id);
    if (transaction == NULL ||
        transaction->sequence_id != packet_sequence_id ||
        config->process_id != packet_process_id) {
        return;
    }

    double time_diff =
        (double)(get_time_us() - transaction->send_time) / 1000.0;

    print_message("%zu bytes from %s: icmp_seq=%d ttl=%u time=%.3f ms\n",
        icmp_length,
        addr_to_str(&ip->saddr),
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
    pop_element_at_index(&config->stored_transactions,
        packet_sequence_id);
    config->response_count++;
}

void read_and_process_packets(ping_config_t *config)
{
    static uint8_t buffer[MAX_READ_SIZE];
    struct sockaddr_in source_addr;
    socklen_t source_len = sizeof(source_addr);

    while (true) {
        ssize_t bytes = recvfrom(config->socket_fd,
                                 buffer,
                                 sizeof(buffer),
                                 0,
                                 (struct sockaddr *)&source_addr,
                                 &source_len);

        if (BRANCH_UNLIKELY(bytes == -1 &&
            (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))) {
            return;
        }
        error_if(bytes == -1, RECVFROM_ERR, strerror(errno));
        process_packet(config, buffer, bytes, &source_addr);
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
    int broadcast = true;
    error_if(setsockopt(config->socket_fd,
            SOL_SOCKET,
            SO_BROADCAST,
            &broadcast,
            sizeof(broadcast)) < 0,
        SETSOCKOPT_ERR, "SO_BROADCAST", strerror(errno));
    if (config->bypass_routing) {
        int dont_route = true;
        error_if(setsockopt(config->socket_fd,
                SOL_SOCKET,
                SO_DONTROUTE,
                &dont_route,
                sizeof(dont_route)) < 0,
            SETSOCKOPT_ERR, "SO_DONTROUTE", strerror(errno));
    }
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