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

static uint32_t do_sum(void *obj, size_t byte_size) {
    uint32_t sum = 0;
    size_t two_byte_size = byte_size / 2;
	error_if(byte_size % 2, CHECKSUM_SIZE_ERR);
    for (size_t i = 0; i < two_byte_size; i++) {
        sum += ((uint16_t *)obj)[i];
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return sum;
}

uint16_t get_checksum(void *obj, size_t byte_size) {
    uint32_t sum = do_sum(obj, byte_size);
    sum = ~sum;
    if (sum == 0) {
        sum = -1;
    }
    return sum;
}

bool verify_checksum(void *obj, size_t byte_size) {
    uint32_t sum = do_sum(obj, byte_size);
    if ((uint16_t)sum != 0xFFFF) {
        return false;
    }
    return true;
}

icmp_packet_t create_echo_request(ping_config_t *config) {
    icmp_packet_t packet = {0};
    static uint16_t sequence_count = 0;

    packet.type = ICMP_ECHO_REQUEST;
    packet.code = 0;
    packet.process_id = config->process_id;
    packet.sequence_id = sequence_count++;
    packet.checksum = 0;
    packet.checksum = get_checksum(&packet, sizeof(packet));
    libassert(verify_checksum(&packet, sizeof(packet)),
		"create_echo_request: invalid get/verify checksum parallel");
    return packet;
}

bool send_echo_request(ping_config_t *config, bool unsent_packet) {
    static icmp_packet_t packet;

    if (BRANCH_UNLIKELY(!unsent_packet)) {
        packet = create_echo_request(config);
    }
    ssize_t sent_bytes = sendto(
        config->socket_fd,
        &packet,
        sizeof(packet),
        0,
        (struct sockaddr *)&config->destination_addr,
        sizeof(config->destination_addr));
    if (BRANCH_UNLIKELY(sent_bytes == -1 &&
        (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))) {
        return true;
    }
    error_if(sent_bytes == -1, SENDTO_ERR, strerror(errno));
    uint64_t now = get_time_us();
    write_element_at_index(
        &config->stored_transactions,
        packet.sequence_id,
        &(ping_transaction_t){
            .sequence_id = packet.sequence_id,
            .send_time = now});
    config->next_send_time = now + config->interval;
    return false;
}

void process_packet(ping_config_t *config, uint8_t *buffer, size_t bytes) {
    struct ip *ip = (struct ip *)buffer;
    size_t ip_header_length;
    size_t ip_total_length;

    if (bytes < sizeof(ip)) {
        print_message(PACKET_TOO_SHORT, (int)bytes, config->destination);
        return;
    }
    if (ip->ip_v != 4) {
        print_message(VERSION_INVALID);
        return;
    }
    ip_header_length = ip->ip_hl * 4;
    ip_total_length = ntohs(ip->ip_len);
    
    if (ip_header_length < sizeof(*ip) ||
        ip_header_length > ip_total_length ||
        ip_total_length > bytes ||
        ip_total_length < ip_header_length + sizeof(icmp_packet_t)) {
        print_message(PACKET_TOO_SHORT, (int)bytes, config->destination);
        return;
    }
    icmp_packet_t *icmp = (icmp_packet_t *)(buffer + ip_header_length);

    if (icmp->type != ICMP_ECHO_REPLY) {
        return;
    }
    ping_transaction_t const* transaction = read_element_at_index(
        &config->stored_transactions,
        icmp->sequence_id);
    if (transaction == NULL || transaction->sequence_id != icmp->sequence_id) {
        return;
    }
    if (config->destination_addr_str) {
        print_message("%d bytes from %s (%s): icmp_seq=%d time=%.3f ms\n",
            (int)(ip_total_length - ip_header_length),
            config->destination,
            config->destination_addr_str,
            icmp->sequence_id,
            (double)(get_time_us() - transaction->send_time) / 1000.0);
    } else {
        print_message("%d bytes from %s: icmp_seq=%d time=%.3f ms\n",
            (int)(ip_total_length - ip_header_length),
            config->destination,
            icmp->sequence_id,
            (double)(get_time_us() - transaction->send_time) / 1000.0);
    }
    pop_element_at_index(&config->stored_transactions, icmp->sequence_id);
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
	if (!inet_aton(config->destination, &config->destination_addr)) {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = 0;
        hints.ai_protocol = IPPROTO_ICMP;
		struct addrinfo *result = NULL;
		int ret = getaddrinfo(config->destination, NULL, &hints, &result);
		error_if(ret != 0, UNKNOWN_NAME_ERR, config->destination);
		config->destination_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr;
        config->destination_addr_str = malloc(INET_ADDRSTRLEN);
        error_if(!inet_ntop(AF_INET, &config->destination_addr, config->destination_addr_str, INET_ADDRSTRLEN),
            INET_NTOP_ERR, strerror(errno));
		freeaddrinfo(result);
	}
}

void init_config(ping_config_t *config) {
	config->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	error_if(config->socket_fd == -1, SOCKET_ERR, strerror(errno));
    config->destination_addr_str = NULL;
	if (config->destination_addr.s_addr == 0) {
		get_destination_addr(config);
	}
    int flags = fcntl(config->socket_fd, F_GETFL);
    fcntl(config->socket_fd, F_SETFL, flags | O_NONBLOCK);
}