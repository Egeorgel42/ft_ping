#include "ping.h"

#include "parsing.h"
#include <stddef.h>

int main() {
    create_icmp_packet(ICMP_ECHO_REQUEST);
    const flag_t* flag_defs = init_flag_definition();
    flag_input_t input[] = {
        {'v', NULL},
        {'x', NULL},
        {'h', NULL},
        {'\0', NULL}
    };
    ping_config_t config = { .mode = PING_MODE_DEFAULT };
    apply_flags(&config, input, flag_defs);
    return 0;
}