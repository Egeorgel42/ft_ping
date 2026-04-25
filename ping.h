#ifndef PING_H
# define PING_H

# include <stdint.h>
# include <netinet/if_ether.h>

typedef struct icmp_packet_s
{
    uint8_t     type;
    uint8_t     code;
    uint16_t    checksum;
    uint16_t    id;
    uint16_t    sequence;
    char*       payload;
} icmp_packet_t;

typedef enum icmp_type_e
{
    ICMP_ECHO_REPLY = 0,
    ICMP_ECHO_REQUEST = 8,
} icmp_type_e;

#endif