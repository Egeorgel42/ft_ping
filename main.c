#include "ping.h"

int main() {
    create_icmp_packet(ICMP_ECHO_REQUEST);
    return 0;
}