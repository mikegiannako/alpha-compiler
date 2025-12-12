#ifndef _ERRORS_H_
#define _ERRORS_H_

#define RED_TEXT "\033[31m"
#define GREEN_TEXT "\033[32m"
#define ORANGE_TEXT "\033[33m"

#define RESET_TEXT "\033[0m"

// Internal macros - don't call these directly
#define _ERROR_MSG(filename, line, error_type, format, ...) \
    do { \
        fprintf(stderr, RED_TEXT "[%s ERROR]" RESET_TEXT " - ", error_type); \
        fprintf(stderr, format, ##__VA_ARGS__); \
        fprintf(stderr, " - (%s @ %d)\n", filename, line); \
        exit(EXIT_FAILURE); \
    } while (0)

#define _WARNING_MSG(filename, line, warning_type, format, ...) \
    do { \
        fprintf(stderr, ORANGE_TEXT "[%s WARNING]" RESET_TEXT " - ", warning_type); \
        fprintf(stderr, format, ##__VA_ARGS__); \
        fprintf(stderr, " - (%s @ %d)\n", filename, line); \
    } while (0)

// Public macros that automatically include file and line information
#define ERROR_MSG(error_type, format, ...) \
    _ERROR_MSG(__FILE__, __LINE__, error_type, format, ##__VA_ARGS__)

#define WARNING_MSG(warning_type, format, ...) \
    _WARNING_MSG(__FILE__, __LINE__, warning_type, format, ##__VA_ARGS__)

#endif // _ERRORS_H_