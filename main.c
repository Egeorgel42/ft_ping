#include "ping.h"

#include "parsing.h"
#include <stddef.h>

int main(int argc, char** argv) {
    ping_config_t config = { .mode = PING_MODE_DEFAULT };
    apply_input_flags(&config, argc, argv);
    create_icmp_packet(ICMP_ECHO_REQUEST);
    return 0;
}