#ifndef _ERRORS_H_
#define _ERRORS_H_

#define RED_TEXT "\033[31m"
#define GREEN_TEXT "\033[32m"
#define ORANGE_TEXT "\033[33m"
#define BLUE_TEXT "\033[34m"

#define RESET_TEXT "\033[0m"

// COMPILER DEVELOPER ERROR MACROS
// To be used for internal compiler errors/debugging (shows compiler source location)

// Internal macros - not to be called directly 
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

// Public macros that automatically include compiler source file and line information
#define ERROR_MSG(error_type, format, ...) \
    _ERROR_MSG(__FILE__, __LINE__, error_type, format, ##__VA_ARGS__)

#define WARNING_MSG(warning_type, format, ...) \
    _WARNING_MSG(__FILE__, __LINE__, warning_type, format, ##__VA_ARGS__)



// USER-FACING ERROR MACROS
// To be used for compilation errors in Alpha source code (no compiler internals exposed)

extern const char* sourceFileName;
extern int yylineno;
extern int yycolumn;
extern int yyleng;

#define USER_ERROR(error_type, format, ...) \
    do { \
        fprintf(stderr, RED_TEXT "[%s ERROR]" RESET_TEXT " - ", error_type); \
        fprintf(stderr, format, ##__VA_ARGS__); \
        fprintf(stderr, BLUE_TEXT " | %s:%d:%d" RESET_TEXT "\n", sourceFileName, yylineno, yycolumn - yyleng); \
        exit(EXIT_FAILURE); \
    } while (0)

#define USER_WARNING(warning_type, format, ...) \
    do { \
        fprintf(stderr, ORANGE_TEXT "[%s WARNING]" RESET_TEXT " - ", warning_type); \
        fprintf(stderr, format, ##__VA_ARGS__); \
        fprintf(stderr, BLUE_TEXT " | %s:%d:%d" RESET_TEXT "\n", sourceFileName, yylineno, yycolumn - yyleng); \
    } while (0)

#endif // _ERRORS_H_