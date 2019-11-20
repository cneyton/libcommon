#ifndef LOG_H_
#define LOG_H_

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>

#define likely(x)   __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

namespace common
{

using Logger = std::shared_ptr<spdlog::logger>;

// -------------------------------- Log macros ---------------------------------
// log_trace(logger, const char * format, ...)
// log_debug(logger, const char * format, ...)
// log_info(logger, const char * format, ...)
// log_warn(logger, const char * format, ...)
// log_error(logger, const char * format, ...)
// log_crit(logger, const char * format, ...)
//
// ----------------------------- common_die macros -----------------------------
// common_die         (return_value, const char * format, ...)
// common_die_void    (const char * format, ...)
// common_die_goto    (value_to_set, return_value, goto_flag, const char * format, ...)
//
// common_die_zero         (value_to_check, return_value, const char * format, ...)
// common_die_zero_void    (value_to_check, const char * format, ...)
// common_die_zero_goto    (value_to_check, value_to_set, return_value, goto_flag, const char * format, ...)
//
// common_die_null         (value_to_check, return_code, const char * format, ...)
// common_die_null_void    (value_to_check, const char * format, ...)
// common_die_null_goto    (value_to_check, value_to_set, return_value, goto_flag, const char * format, ...)
//


/******************************************************************************/
/*                                   log_trace                                */
/******************************************************************************/
#define log_trace(...)                                              \
    do {                                                            \
        SPDLOG_LOGGER_TRACE(__VA_ARGS__);                           \
    } while(0)

/******************************************************************************/
/*                                   log_debug                                */
/******************************************************************************/
#define log_debug(...)                                              \
    do {                                                            \
        SPDLOG_LOGGER_DEBUG(__VA_ARGS__);                           \
    } while(0)

/******************************************************************************/
/*                                   log_info                                 */
/******************************************************************************/
#define log_info(...)                                               \
    do {                                                            \
        SPDLOG_LOGGER_INFO(__VA_ARGS__);                            \
    } while(0)

/******************************************************************************/
/*                                   log_warn                                 */
/******************************************************************************/
#define log_warn(...)                                               \
    do {                                                            \
        SPDLOG_LOGGER_WARN(__VA_ARGS__);                            \
    } while(0)

/******************************************************************************/
/*                                   log_error                                */
/******************************************************************************/
#define log_error(...)                                              \
    do {                                                            \
        SPDLOG_LOGGER_ERROR(__VA_ARGS__);                           \
    } while(0)


/******************************************************************************/
/*                                   log_critical                             */
/******************************************************************************/
#define log_crit(...)                                               \
    do {                                                            \
        SPDLOG_LOGGER_CRIT(__VA_ARGS__);                            \
    } while(0)


/******************************************************************************/
/*                                 common_die                                 */
/******************************************************************************/
#define common_die(logger, return_code, ...) {                      \
    log_error(logger, __VA_ARGS__);                                 \
    return return_code;                                             \
}

/******************************************************************************/
/*                              common_die_throw                              */
/******************************************************************************/
#define common_die_throw(logger, expr, ...) {                       \
    log_error(logger, __VA_ARGS__);                                 \
    throw expr;                                                     \
}


/******************************************************************************/
/*                               common_die_void                              */
/******************************************************************************/
#define common_die_void(logger, ...) {                              \
    log_error(logger, __VA_ARGS__);                                 \
    return;                                                         \
}

/******************************************************************************/
/*                             common_die_throw_void                          */
/******************************************************************************/
#define common_die_throw_void(logger, ...) {                        \
    log_error(logger, __VA_ARGS__);                                 \
    throw;                                                          \
}


/******************************************************************************/
/*                               common_die_goto                              */
/******************************************************************************/
#define common_die_goto(logger, value_to_set, return_value, goto_flag, ...) \
    do{                                                                     \
        log_error(logger, __VA_ARGS__);                                     \
        value_to_set = return_value;                                        \
        goto goto_flag;                                                     \
    } while(0)


/******************************************************************************/
/*                              common_die_zero                               */
/******************************************************************************/
#define common_die_zero(logger, value_to_check, return_code, ...)   \
    do{                                                             \
        if(unlikely((value_to_check) < 0)) {                        \
            log_error(logger, __VA_ARGS__);                         \
            return return_code;                                     \
        }                                                           \
    } while(0)


/******************************************************************************/
/*                            common_die_zero_void                            */
/******************************************************************************/
#define common_die_zero_void(logger, value_to_check, ...)           \
    do{                                                             \
        if(unlikely(value_to_check < 0)) {                          \
            log_error(logger, __VA_ARGS__);                         \
            return;                                                 \
        }                                                           \
    } while(0)


/******************************************************************************/
/*                           common_die_zero_goto                             */
/******************************************************************************/
#define common_die_zero_goto(logger, value_to_check, value_to_set, return_value, goto_flag, ...)\
    do{                                                                                         \
        if(unlikely(value_to_check < 0)) {                                                      \
            log_error(__VA_ARGS__);                                                             \
            value_to_set = return_value;                                                        \
            goto goto_flag;                                                                     \
        }                                                                                       \
    } while(0)

/******************************************************************************/
/*                              common_die_null                               */
/******************************************************************************/
#define common_die_null(logger, value_to_check, return_code, ...)   \
    do{                                                             \
        if(unlikely(value_to_check == nullptr)) {                   \
            log_error(logger, __VA_ARGS__);                         \
            return return_code;                                     \
        }                                                           \
    } while(0)


/******************************************************************************/
/*                            common_die_null_void                            */
/******************************************************************************/
#define common_die_null_void(logger, value_to_check, ...)           \
    do{                                                             \
        if(unlikely(value_to_check == nullptr)) {                   \
            log_add(__VA_ARGS__);                                   \
            return;                                                 \
        }                                                           \
    } while(0)


/******************************************************************************/
/*                           common_die_null_goto                             */
/******************************************************************************/
#define common_die_null_goto(value_to_check, value_to_set, return_value, goto_flag, ...)    \
    do{                                                                                     \
        if(unlikely(value_to_check == nullptr)) {                                           \
            log_add(__VA_ARGS__);                                                           \
            value_to_set = return_value;                                                    \
            goto goto_flag;                                                                 \
        }                                                                                   \
    } while(0)

} /* namespace common */

#endif /* LOG_H */
