#include "ping.h"
#include <stddef.h>
#include <unistd.h>

int main(int argc, char** argv) {
    ping_config_t config = {
		.mode = PING_MODE_DEFAULT,
		.process_id = getpid() & 0xFFFF,
		.destination_addr = {0},
		.socket_fd = -1 ,
		.destination = NULL};

    apply_input_flags(&config, argc, argv);
	init_config(&config);
    create_icmp_packet(ICMP_ECHO_REQUEST, &config);
    return 0;
}