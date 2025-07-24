/**
 * A basic C/C++ color log macro.
 *
 * @author Anthony Pena <anthony.pena@outlook.fr>
 * @version 0.0.5 (Fixed for MSVC and improved Windows compatibility)
 */
#ifndef __COLOR_LOG__
#define __COLOR_LOG__

 // Include stdio.h once, as it's needed by printf in all log macros
#include <stdio.h>

// Define base color codes
#define __LOG_COLOR_RED "1;31"
#define __LOG_COLOR_GREEN "0;32"
#define __LOG_COLOR_BLUE "1;34"
#define __LOG_COLOR_YELLOW "0;33"
#define __LOG_COLOR_WHITE "0;37"

// Force disabling coloration on Windows systems by default
// Users can define _LOG_COLOR_FORCE_ANSI to re-enable colors if their
// terminal supports it (e.g., Windows Terminal, or SetConsoleMode is used).
#if defined(_WIN32) && !defined(_LOG_COLOR_FORCE_ANSI)
#define _LOG_NO_COLOR
#endif

// Main logging macro
// Using standard C99/C++11 variadic macro syntax (__VA_ARGS__ and ##__VA_ARGS__)
#ifdef _LOG_NO_COLOR
    // No color version
#define __LOG_COLOR(CLR,CTX,TXT, ...) printf("  %s : "#TXT" \n", CTX, ##__VA_ARGS__)
#else
    // Color version
#define __LOG_COLOR(CLR,CTX,TXT, ...) printf("  \033[%sm%s : "#TXT" \033[m\n", CLR, CTX, ##__VA_ARGS__)
#endif

// Blanket enable all log types if _LOG_ALL is defined
#ifdef _LOG_ALL
#ifndef _LOG_VERBOSE
#define _LOG_VERBOSE
#endif
#ifndef _LOG_WARNING
#define _LOG_WARNING
#endif
#ifndef _LOG_ALERT
#define _LOG_ALERT
#endif
#ifndef _LOG_INFO
#define _LOG_INFO
#endif
#ifndef _LOG_SUCCESS
#define _LOG_SUCCESS
#endif
#endif

// Individual log type definitions
// Note: stdio.h is already included at the top, so no need for redundant includes

#if defined(_LOG_VERBOSE) && !defined(_LOG_NO_VERBOSE)
#define VERBOSE(CTX,TXT, ...) __LOG_COLOR(__LOG_COLOR_WHITE,CTX,TXT,##__VA_ARGS__)
#else
#define VERBOSE(CTX,TXT, ...)
#endif

#if defined(_LOG_WARNING) && !defined(_LOG_NO_WARNING)
#define WARNING(CTX,TXT, ...) __LOG_COLOR(__LOG_COLOR_YELLOW,CTX,TXT,##__VA_ARGS__)
#else
#define WARNING(CTX,TXT, ...)
#endif

#if defined(_LOG_INFO) && !defined(_LOG_NO_INFO)
#define INFO(CTX,TXT, ...) __LOG_COLOR(__LOG_COLOR_BLUE,CTX,TXT,##__VA_ARGS__)
#else
#define INFO(CTX,TXT, ...)
#endif

#if defined(_LOG_ALERT) && !defined(_LOG_NO_ALERT)
#define ALERT(CTX,TXT, ...) __LOG_COLOR(__LOG_COLOR_RED,CTX,TXT,##__VA_ARGS__)
#else
#define ALERT(CTX,TXT, ...)
#endif

#if defined(_LOG_SUCCESS) && !defined(_LOG_NO_SUCCESS)
#define SUCCESS(CTX,TXT, ...) __LOG_COLOR(__LOG_COLOR_GREEN,CTX,TXT,##__VA_ARGS__)
#else
#define SUCCESS(CTX,TXT, ...)
#endif

#endif /* __COLOR_LOG__*/