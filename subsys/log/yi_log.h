#ifndef YI_LOG_H
#define YI_LOG_H

typedef enum
{
    YI_LOG_LEVEL_DEBUG = 0,
    YI_LOG_LEVEL_INFO,
    YI_LOG_LEVEL_WARNING,
    YI_LOG_LEVEL_ERROR,
    YI_LOG_LEVEL_NONE
} yi_log_level_t;

void yi_log_set_level(yi_log_level_t level);
int yi_log_write(yi_log_level_t level, const char *message);
int yi_log_debug(const char *message);
int yi_log_info(const char *message);
int yi_log_warning(const char *message);
int yi_log_error(const char *message);

#endif
