#ifndef __LOG__
#define __LOG__

#include <stdio.h>

/*
\e[0;30m 	Black
\e[0;31m 	Red
\e[0;32m 	Green
\e[0;33m 	Yellow
\e[0;34m 	Blue
\e[0;35m 	Purple
\e[0;36m 	Cyan
\e[0;37m 	White
*/

#define ANSI_RED "\033[0;31m"
#define ANSI_GREEN "\033[0;32m"
#define ANSI_YELLOW "\033[0;33m"
#define ANSI_BLUE "\033[0;34m"
#define ANSI_RESET "\033[0m"

#define COLORED_ERROR ANSI_RED "[ERROR]" ANSI_RESET
#define COLORED_WARNING ANSI_YELLOW "[WARNING]" ANSI_RESET
#define COLORED_INFO ANSI_BLUE "[INFO]" ANSI_RESET

#define eprintf(string, ...) fprintf(stderr, COLORED_ERROR " %s:%i:%s: " string, __FILE__, __LINE__, __func__ __VA_OPT__(, ) __VA_ARGS__)

#endif // __LOG__