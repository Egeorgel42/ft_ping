#include "ping.h"
#include "util.h"
#include <stddef.h>
#include <unistd.h>

int main(int argc, char** argv) {
    ping_config_t config = {
		.mode = PING_MODE_DEFAULT,
		.process_id = getpid() & 0xFFFF,
        .stored_transactions = create_circular_buffer(sizeof(ping_transaction_t), 10),
		.destination_addr = {0},
		.socket_fd = -1 ,
		.destination = NULL};

    apply_input_flags(&config, argc, argv);
	init_config(&config);
    create_icmp_packet(ICMP_ECHO_REQUEST, &config);
    free_circular_buffer(&config.stored_transactions);
    return 0;
}