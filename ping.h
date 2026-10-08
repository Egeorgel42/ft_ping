#ifndef PING_H
# define PING_H

#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200112L
#endif

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#define MAX_READ_SIZE 65535
#define PAYLOAD_DEFAULT_SIZE 56

# include <stdint.h>
# include <netinet/in.h>
# include "util.h"

# define MODE_DEF(x) 1 << x

typedef struct {
    uint8_t     type;
    uint8_t     code;
    uint16_t    checksum;
    uint16_t    process_id;
    uint16_t    sequence_id;
} icmp_packet_t;

typedef enum {
    ICMP_ECHO_REPLY = 0,
    ICMP_ECHO_REQUEST = 8,
} icmp_type_e;

typedef enum {
    PING_VERBOSE = MODE_DEF(0),
} ping_mode_e;

typedef struct {
    uint8_t 	        mode;
    char*               send_buffer;
	char const* 	    hostname_str;
	char* 	            destination_str;
	struct in_addr	    destination_addr;
    uint16_t    	    process_id;
    uint64_t            next_send_time;
    uint64_t            interval;
    circular_buffer_t 	stored_transactions;
	int 		        socket_fd;
    uint32_t            payload_size;
    uint32_t            ttl;
} ping_config_t;

typedef struct {
    uint16_t sequence_id;
    uint64_t send_time;
} ping_transaction_t;

void apply_input_flags(ping_config_t* config, int argc, char** argv);
void init_config(ping_config_t *config);
void read_and_process_packets(ping_config_t *config);
bool send_echo_request(ping_config_t *config, bool unsent_packet);

#endif