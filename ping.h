#ifndef PING_H
# define PING_H

#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200112L
#endif

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

# include <stdint.h>
# include <netinet/in.h>

typedef struct icmp_packet_s {
    uint8_t     type;
    uint8_t     code;
    uint16_t    checksum;
    uint16_t    process_id;
    uint16_t    sequence_id;
} icmp_packet_t;

typedef enum icmp_type_e {
    ICMP_ECHO_REPLY = 0,
    ICMP_ECHO_REQUEST = 8,
} icmp_type_e;

typedef enum ping_mode_e {
    PING_MODE_DEFAULT,
    PING_MODE_VERBOSE,
} ping_mode_e;

typedef struct ping_config_s {
    ping_mode_e 	mode;
	char const* 	destination;
    uint16_t    	process_id;
	int 		    socket_fd;
	struct in_addr	destination_addr;
} ping_config_t;


void 			apply_input_flags(ping_config_t* config, int argc, char** argv);
void 			init_config(ping_config_t *config);
icmp_packet_t	create_echo_request(ping_config_t *config);
icmp_packet_t	create_icmp_packet(icmp_type_e packet_type, ping_config_t *config);

#endif