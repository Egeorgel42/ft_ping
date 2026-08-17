#include "ping.h"
#include "terminal_output.h"

#include <stdbool.h>
#include <assert.h>
#include <unistd.h>

static uint32_t do_sum(void *obj, size_t byte_size) {
    uint32_t sum = 0;
    size_t two_byte_size = byte_size / 2;
    if (byte_size % 2){
        error(CHECKSUM_SIZE_ERR);
    }
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

icmp_packet_t create_echo_request() {
    icmp_packet_t packet = {0};
    static uint16_t sequence_count = 0;

    packet.type = ICMP_ECHO_REQUEST;
    packet.code = 0;
    packet.id = getpid() & 0xFFFF; // Use process ID as identifier
    packet.sequence = sequence_count++;
    packet.checksum = 0;
    packet.checksum = get_checksum(&packet, sizeof(packet));
    libassert(verify_checksum(&packet, sizeof(packet)),
		"create_echo_request: invalid get/verify checksum parallel");
    return packet;
}

icmp_packet_t create_icmp_packet(icmp_type_e packet_type) {
    switch (packet_type) {
        case ICMP_ECHO_REQUEST:
            return create_echo_request();
        default:
            ASSERT_UNREACHABLE(ICMP_TYPE_ERR);
    }
}