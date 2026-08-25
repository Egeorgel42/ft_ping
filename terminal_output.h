#ifndef TERMINAL_OUTPUT_H
#define TERMINAL_OUTPUT_H
#include "compiler.h"

#define PROGRAM_NAME "ft_ping"

#define USAGE_ERR			PROGRAM_NAME ": usage error: %s\n"
#define USAGE_ERR_DEST_REQ  USAGE_ERR, "Destination address required"
#define UNKNOWN_NAME_ERR	PROGRAM_NAME ": %s: Name or service not known\n"
#define ICMP_TYPE_ERR		PROGRAM_NAME ": Invalid ICMP type\n"
#define CHECKSUM_SIZE_ERR	PROGRAM_NAME ": Unexpected Checksum size, data is not 2byte aligned\n"
#define HELP_MESSAGE \
"\n\
Usage:\n\
    " PROGRAM_NAME " [options] <destination>\n\
Options:\n\
    -h, -?    Show this help message\n\
    -v        Verbose output\n\
\n"
#define INVALID_OPTION		PROGRAM_NAME ": invalid option -- '%c'\n" HELP_MESSAGE
#define OPTION_REQ_ARG		PROGRAM_NAME ": option requires an argument -- '%c'\n" HELP_MESSAGE
#define ALLOC_ERR 			PROGRAM_NAME ": Memory allocation failed\n"
#define ASSERT_ERR			PROGRAM_NAME ": Assert error: %s\n"
#define SOCKET_ERR			PROGRAM_NAME ": socket error: %s\n"

void error(const char* format, ...);
void print_message(const char* format, ...);

#define error_if(condition, format, ...) {			\
    if (BRANCH_UNLIKELY(condition)) {				\
        error(format, ##__VA_ARGS__);				\
    }}

#if defined(NDEBUG)
	#define libassert(condition, msg) {				\
		(void)0;									\
	}
#else
	#define libassert(condition, msg) {				\
		error_if(!condition, ASSERT_ERR, msg); 		\
	}
#endif

#endif