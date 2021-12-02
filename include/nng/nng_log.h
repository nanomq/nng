#ifndef NNG_LOG_H
#define NNG_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define LOG_VERSION "0.1.0"

typedef struct {
    va_list     ap;
    const char *fmt;
    const char *file;
    const char *func;
    struct tm * time;
    void *      udata;
    int         line;
    int         level;
} log_Event;

typedef void (*log_LogFn)(log_Event *ev);
typedef void (*log_LockFn)(bool lock, void *udata);

enum { NNG_LOG_TRACE, NNG_LOG_DEBUG, NNG_LOG_INFO, NNG_LOG_WARN, NNG_LOG_ERROR, NNG_LOG_FATAL };

extern const char *log_level_string(int level);
extern void        log_set_lock(log_LockFn fn, void *udata);
extern void        log_set_level(int level);
extern void        log_set_quiet(bool enable);
extern int         log_add_callback(log_LogFn fn, void *udata, int level);
extern int         log_add_fp(FILE *fp, int level);
extern void log_log(int level, const char *file, int line, const char *func,
                    const char *fmt, ...);

#define log_trace(...) \
    log_log(NNG_LOG_TRACE, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_debug(...) \
    log_log(NNG_LOG_DEBUG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_info(...) \
    log_log(NNG_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_warn(...) \
    log_log(NNG_LOG_WARN, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_error(...) \
    log_log(NNG_LOG_ERROR, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_fatal(...) \
    log_log(NNG_LOG_FATAL, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif

