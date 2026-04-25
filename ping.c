#include "ping.h"
#include "terminal_output.h"

icmp_packet_t create_echo_request() {
    icmp_packet_t packet;
    static uint16_t sequence_count = 0;

    packet.type = ICMP_ECHO_REQUEST;
    packet.code = 0;
    packet.id = getpid() & 0xFFFF; // Use process ID as identifier
    packet.sequence = sequence_count++;
    packet.payload = 0;

    return packet;
}

icmp_packet_t create_icmp_packet(icmp_type_e packet_type) {
    switch (packet_type) {
        case ICMP_ECHO_REQUEST:
            return create_echo_request();
        default:
            error(ICMP_TYPE_ERR);
    }
}