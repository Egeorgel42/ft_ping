#include "ping.h"
#include "terminal_output.h"

#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

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

icmp_packet_t create_icmp_packet(icmp_type_e packet_type, ping_config_t *config) {
    switch (packet_type) {
        case ICMP_ECHO_REQUEST:
            return create_echo_request(config);
        default:
            ASSERT_UNREACHABLE(ICMP_TYPE_ERR);
    }
}

void get_destination_addr(ping_config_t *config) {
	if (!inet_aton(config->destination, &config->destination_addr)) {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_ICMP;
		struct addrinfo *result = NULL;
		int ret = getaddrinfo(config->destination, NULL, &hints, &result);
		error_if(ret != 0, UNKNOWN_NAME_ERR, config->destination);
		config->destination_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr;
		freeaddrinfo(result);
	}
}

void init_config(ping_config_t *config) {
	config->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	error_if(config->socket_fd == -1, SOCKET_ERR, strerror(errno));
	if (config->destination_addr.s_addr == 0) {
		get_destination_addr(config);
	}
}