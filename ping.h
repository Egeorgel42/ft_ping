#ifndef PING_H
# define PING_H

# include <stdint.h>
# include <netinet/if_ether.h>

typedef struct icmp_packet_s {
    uint8_t     type;
    uint8_t     code;
    uint16_t    checksum;
    uint16_t    id;
    uint16_t    sequence;
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
    ping_mode_e mode;
} ping_config_t;

typedef struct flag_input_s {
    const char flag;
    const char* arg;
} flag_input_t;

icmp_packet_t create_echo_request();
icmp_packet_t create_icmp_packet(icmp_type_e packet_type);

#endif