#include "ping.h"
#include "util.h"
#include <stddef.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <limits.h>
#include <float.h>

static volatile sig_atomic_t g_stop = 0;

void handle_sigint(int signal) {
    (void)signal;
    g_stop = 1;
}

static double nabs(double value)
{
    return value < 0.0 ? -value : value;
}

static double nsqrt(double value, double precision)
{
    double x0;
    double x1;

    if (value < 0.0)
        return 0.0;

    if (value < precision)
        return 0.0;

    x1 = value / 2.0;

    do {
        x0 = x1;
        x1 = (x0 + value / x0) / 2.0;
    } while (nabs(x1 - x0) > precision);

    return x1;
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

    if (config->deadline) {
        config->deadline += get_time_us();
    }
    while (!g_stop) {
        int timeout;
        int64_t buf = (int64_t)config->next_send_time - (int64_t)get_time_us();
        if (buf < 0) {
            buf = 0;
        } else if (config->deadline && (int64_t)config->deadline - (int64_t)get_time_us() < buf) {
            buf = (int64_t)config->deadline - (int64_t)get_time_us();
        }

        buf = (buf + 999) / 1000;
        if (buf > INT_MAX) {
            timeout = INT_MAX;
        } else {
            timeout = (int)buf;
        }

        struct pollfd fds = unsent_packet ? fds_unsent_packet : fds_reading;
        poll_ready = poll(&fds, 1, timeout);

        if (poll_ready > 0 && fds.revents & POLLIN) {
            read_and_process_packets(config);
        }
        
        if (config->deadline && get_time_us() >= config->deadline) {
            break;
        }
        if (poll_ready > 0 && fds.revents & POLLOUT) {
            unsent_packet = send_echo_request(config, unsent_packet);
        }
        else if (poll_ready == 0) {
            unsent_packet = send_echo_request(config, unsent_packet);
        }
        else if (errno == EINTR) {
            continue;
        }
        error_if(poll_ready == -1, POLL_ERROR, strerror(errno));
    }
    if (!g_stop && config->wait_time) {
        config->wait_time += get_time_us();
    }
    while (!g_stop && config->response_count < config->sequence_count) {
        int64_t remaining = (int64_t)(config->wait_time - get_time_us());
        if (remaining <= 0) {
            break;
        }
        poll_ready = poll(&fds_reading, 1, (int)(remaining / 1000) + 1);
        if (poll_ready > 0 && fds_reading.revents & POLLIN) {
            read_and_process_packets(config);
        } else if (poll_ready == 0) {
            break;
        }
        error_if(poll_ready == -1 && errno != EINTR,
            POLL_ERROR, strerror(errno));
    }
    if (config->sequence_count == 0) {
        return;
    }
    print_message("--- %s ping statistics ---\n", config->hostname_str);
    uint32_t packet_lost = config->sequence_count - config->response_count;
    uint32_t lost_percent = (packet_lost * 100u) / config->sequence_count;
    print_message("%d packets transmitted, %d packets received, %d%% packet loss\n",
        config->sequence_count,
        config->response_count,
        lost_percent);
    if (config->response_count == 0) {
        return;
    }
    double avg_time = config->sum_time / config->response_count;
    double stddev = nsqrt(
        (config->sum_time_sq / config->response_count) -
        (avg_time * avg_time),
        0.0005);
    print_message("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
        config->min_time,
        avg_time,
        config->max_time,
        stddev);
}

int main(int argc, char** argv) {
    ping_config_t config = {
		.mode = 0,
		.process_id = getpid() & 0xFFFF,
        .sequence_count = 0,
        .stored_transactions = create_circular_buffer(sizeof(ping_transaction_t), 10),
		.destination_addr = {0},
        .interval = 1000000,
        .next_send_time = get_time_us(),
		.socket_fd = -1,
        .hostname_str = NULL,
		.destination_str = NULL,
        .ttl = 0,
        .payload_size = PAYLOAD_DEFAULT_SIZE,
        .bypass_routing = false,
        .deadline = 0,
        .wait_time = 0,
        .min_time = DBL_MAX,
        .max_time = 0,
        .sum_time = 0,
        .sum_time_sq = 0,
        .response_count = 0};

    struct sigaction sa = {0};
    sa.sa_handler = handle_sigint;
    error_if(sigaction(SIGINT, &sa, NULL) == -1, SIGACTION_ERR, strerror(errno));

    apply_input_flags(&config, argc, argv);
    config.send_buffer = malloc(config.payload_size + sizeof(icmp_packet_t));
    memset(config.send_buffer, '\0', config.payload_size + sizeof(icmp_packet_t));
	init_config(&config);
    error_if(setvbuf(stdout, NULL, _IOLBF, 0) != 0, "setvbuf error: %s\n", strerror(errno));
    print_message("PING %s (%s): %u data bytes",
        config.hostname_str,
        config.destination_str,
        config.payload_size);
    if (config.mode & PING_VERBOSE) {
        print_message(", id 0x%04x = %u", config.process_id, config.process_id);
    }
    print_message("\n");
    run_logic(&config);
    free_circular_buffer(&config.stored_transactions);
    free(config.destination_str);
    free(config.send_buffer);
    return config.response_count == 0;
}