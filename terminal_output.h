#ifndef TERMINAL_OUTPUT_H
#define TERMINAL_OUTPUT_H
#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)

#define PROGRAM_NAME "ft_ping: "
#define PROGRAM_PREFIX_MESSAGE    PROGRAM_NAME __FILE__ ", " STRINGIFY(__LINE__) ": "

#define USAGE_ERR			PROGRAM_NAME "usage error: %s\n"
#define USAGE_ERR_DEST_REQ  USAGE_ERR, "Destination address required"
#define UNKNOWN_NAME_ERR	PROGRAM_NAME "%s: Name or service not known\n"
#define MISSING_HOST_OP 	PROGRAM_NAME "missing host operand\nTry 'ping --help' for more information.\n"
#define HELP_MESSAGE \
"\n\
Usage:\n\
    " PROGRAM_NAME " [options] <destination>\n\
Options:\n\
    -h, -?    Show this help message\n\
    -v        Verbose output\n\
\n"
#define INVALID_OPTION		PROGRAM_NAME "invalid option -- '%c'\n" HELP_MESSAGE
#define INVALID_STR_OPTION	PROGRAM_NAME "invalid option -- '%s'\n" HELP_MESSAGE
#define OPTION_REQ_ARG		PROGRAM_NAME "option requires an argument -- '%s'\n" HELP_MESSAGE
#define PACKET_TOO_SHORT    PROGRAM_NAME "packet too short (%ld bytes) from %s\n"
#define MALFORMED_PACKET    PROGRAM_NAME "malformed packet from %s\n"
#define VERSION_INVALID     PROGRAM_NAME "invalid version in IP header, expected 4\n"
#define CHECKSUM_SIZE_ERR	PROGRAM_PREFIX_MESSAGE "Unexpected Checksum size, data is not 2byte aligned\n"
#define ALLOC_ERR 			PROGRAM_PREFIX_MESSAGE "Memory allocation failed\n"
#define ASSERT_ERR			PROGRAM_PREFIX_MESSAGE "Assert error: %s\n"
#define SOCKET_ERR			PROGRAM_PREFIX_MESSAGE "socket error: %s\n"
#define SENDTO_ERR			PROGRAM_PREFIX_MESSAGE "sendto error: %s\n"
#define RECVFROM_ERR		PROGRAM_PREFIX_MESSAGE "recvfrom error: %s\n"
#define SIGACTION_ERR		PROGRAM_PREFIX_MESSAGE "sigaction error: %s\n"
#define CLOCK_ERR		    PROGRAM_PREFIX_MESSAGE "clock error: %s\n"
#define POLL_ERROR		    PROGRAM_PREFIX_MESSAGE "poll error: %s\n"
#define INET_NTOP_ERR		PROGRAM_PREFIX_MESSAGE "inet_ntop error: %s\n"
#define SETSOCKOPT_ERR		PROGRAM_PREFIX_MESSAGE "setsockopt(%s) error: %s\n"

#define UNKNOWN_ICMP_TYPE_ERR	"Unknown ICMP type: %d\n"
#define TIME_EXCEEDED_ERR       "Time to live exceeded\n"
#define FRAG_REASSEMBLY_TIME_EXCEEDED_ERR	"Frag reassembly time exceeded\n"
#define UNKNOWN_TIME_EXCEEDED_ERR	"Time exceeded, unknown code: %d\n"

#define NETWORK_UNREACHABLE_ERR	        "Destination Net Unreachable\n"
#define HOST_UNREACHABLE_ERR		    "Destination Host Unreachable\n"
#define PROTOCOL_UNREACHABLE_ERR	    "Destination Protocol Unreachable\n"
#define PORT_UNREACHABLE_ERR		    "Destination Port Unreachable\n"
#define FRAGMENTATION_REQUIRED_ERR	    "Fragmentation needed and DF set\n"
#define SOURCE_ROUTE_FAILED_ERR	        "Source route failed\n"
#define NETWORK_UNKNOWN_ERR		        "Destination Network Unknown\n"
#define HOST_UNKNOWN_ERR		        "Host Unknown\n"
#define SOURCE_HOST_ISOLATED_ERR	    "Host Isolated\n"
#define NETWORK_UNREACHABLE_FOR_TOS_ERR	"Destination Network Unreachable At This TOS\n"
#define HOST_UNREACHABLE_FOR_TOS_ERR	"Destination Host Unreachable At This TOS\n"
#define COMMUNICATION_ADMINISTRATIVELY_PROHIBITED_ERR	"Packet Filtered\n"
#define HOST_PRECEDENCE_VIOLATION_ERR	"Precedence Violation\n"
#define PRECEDENCE_CUTOFF_IN_EFFECT_ERR	"Precedence Cutoff\n"
#define UNKNOWN_UNREACHABLE_ERR	        "Dest Unreachable, Unknown Code: %d\n"


enum unreachable_err_codes {
    NETWORK_UNREACHABLE = 0,
    HOST_UNREACHABLE = 1,
    PROTOCOL_UNREACHABLE = 2,
    PORT_UNREACHABLE = 3,
    FRAGMENTATION_REQUIRED = 4,
    SOURCE_ROUTE_FAILED = 5,
    NETWORK_UNKNOWN = 6,
    HOST_UNKNOWN = 7,
    SOURCE_HOST_ISOLATED = 8,
    NETWORK_UNREACHABLE_FOR_TOS = 11,
    HOST_UNREACHABLE_FOR_TOS = 12,
    COMMUNICATION_ADMINISTRATIVELY_PROHIBITED = 13,
    HOST_PRECEDENCE_VIOLATION = 14,
    PRECEDENCE_CUTOFF_IN_EFFECT = 15
};

#define print_message(format, ...)                  \
    fprintf(stdout, format, ##__VA_ARGS__);

#define error(format, ...) {                       \
    fprintf(stderr, format, ##__VA_ARGS__);         \
    exit(1);                                        \
    COMPILER_UNREACHABLE();                         \
}

#define error_if(condition, format, ...) {			\
    if (BRANCH_UNLIKELY(condition)) {				\
        error(format, ##__VA_ARGS__);				\
    }}

#if defined(NDEBUG)
	#define libassert(condition, msg) (void)0;
#else
	#define libassert(condition, msg) error_if(!(condition), ASSERT_ERR, msg);
#endif

#endif