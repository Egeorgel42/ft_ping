#include "ping.h"
#include "util.h"
#include <stddef.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <string.h>

static volatile sig_atomic_t g_stop = 0;

void handle_sigint(int signal) {
    (void)signal;
    g_stop = 1;
}

void find_expired_requests(ping_config_t const* config, circular_buffer_t* buffer) {
    uint64_t now = get_time_us();
    for (uint32_t i = 0; i < buffer->buffer_size; i++) {
        ping_transaction_t const* transaction = read_element_at_index(buffer, i);
        if (!transaction) {
            continue;
        }
        if (now - transaction->send_time >= config->interval) {
            pop_element_at_index(buffer, i);
        }
    }
}

void run_logic(ping_config_t* config) {
    int poll_ready;
    bool unsent_packet = false;

    struct pollfd fds = {
        .fd = config->socket_fd,
        .events = POLLIN | POLLOUT
    };
    while (!g_stop) {
        int timeout = config->next_send_time - get_time_us();
        poll_ready = poll(&fds, 1, timeout);

        if (poll_ready > 0 && fds.revents & POLLIN) {
            read_and_process_packets(config);
        }
        if (poll_ready > 0 && fds.revents & POLLOUT && unsent_packet) {
            unsent_packet = send_echo_request(config, unsent_packet);
        }
        else if (poll_ready == 0) {
            unsent_packet = send_echo_request(config, unsent_packet);
            find_expired_requests(config, &config->stored_transactions);
        }
        else if (errno == EINTR) {
            continue;
        }
        error_if(poll_ready == -1, POLL_ERROR, strerror(errno));
    }
    //print_stats();
}

int main(int argc, char** argv) {
    ping_config_t config = {
		.mode = 0,
		.process_id = getpid() & 0xFFFF,
        .stored_transactions = create_circular_buffer(sizeof(ping_transaction_t), 10),
		.destination_addr = {0},
		.socket_fd = -1 ,
		.destination = NULL};

    struct sigaction sa = {0};
    sa.sa_handler = handle_sigint;
    error_if(sigaction(SIGINT, &sa, NULL) == -1, SIGACTION_ERR, strerror(errno));

    apply_input_flags(&config, argc, argv);
	init_config(&config);
    if (config.destination_addr_str) {
        print_message("PING %s (%s) %lu bytes of data.\n",
            config.destination,
            config.destination_addr_str,
            sizeof(icmp_packet_t));
    } else {
        print_message("PING %s %lu bytes of data.\n",
            config.destination,
            sizeof(icmp_packet_t));
    }
    run_logic(&config);
    free_circular_buffer(&config.stored_transactions);
    free(config.destination_addr_str);
    return 0;
}