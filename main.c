#include "ping.h"
#include "util.h"
#include <stddef.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <limits.h>

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

    struct pollfd fds_reading = {
        .fd = config->socket_fd,
        .events = POLLIN
    };

    struct pollfd fds_unsent_packet = {
        .fd = config->socket_fd,
        .events = POLLIN | POLLOUT
    };
    while (!g_stop) {
        int timeout;
        int64_t buf = (int64_t)config->next_send_time - (int64_t)get_time_us();
        if (buf < 0) {
            timeout = 0;
        } else {
            buf = buf / 1000;
            if (buf > INT_MAX) {
                timeout = INT_MAX;
            } else {
                timeout = (int)buf;
            }
        }
        struct pollfd fds = unsent_packet ? fds_unsent_packet : fds_reading;
        poll_ready = poll(&fds, 1, timeout);

        if (poll_ready > 0 && fds.revents & POLLIN) {
            read_and_process_packets(config);
        }
        if (poll_ready > 0 && fds.revents & POLLOUT) {
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
        .interval = 1000000,
        .next_send_time = get_time_us(),
		.socket_fd = -1 ,
		.destination_str = NULL,
        .ttl = -1,
        .payload_size = PAYLOAD_DEFAULT_SIZE};

    struct sigaction sa = {0};
    sa.sa_handler = handle_sigint;
    error_if(sigaction(SIGINT, &sa, NULL) == -1, SIGACTION_ERR, strerror(errno));

    apply_input_flags(&config, argc, argv);
    config.send_buffer = malloc(config.payload_size + sizeof(icmp_packet_t));
    memset(config.send_buffer, '\0', config.payload_size + sizeof(icmp_packet_t));
	init_config(&config);
    print_message("PING %s (%s) %u bytes of data.\n",
        config.hostname_str,
        config.destination_str,
        config.payload_size);
    run_logic(&config);
    free_circular_buffer(&config.stored_transactions);
    free(config.destination_str);
    free(config.send_buffer);
    return 0;
}