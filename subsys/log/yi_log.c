#include "yi_log.h"
#include "yi_console.h"
#include <stddef.h>

#define YI_LOG_BUFFER_SIZE 192U

static yi_log_level_t minimum_level = YI_LOG_LEVEL_DEBUG;

static const char *yi_log_prefix(yi_log_level_t level)
{
    static const char * const prefixes[] =
    {
        "[DEBUG] ", "[INFO] ", "[WARN] ", "[ERROR] "
    };
    return (level <= YI_LOG_LEVEL_ERROR) ? prefixes[level] : NULL;
}

void yi_log_set_level(yi_log_level_t level)
{
    if(level <= YI_LOG_LEVEL_NONE)
    {
        minimum_level = level;
    }
}

int yi_log_write(yi_log_level_t level, const char *message)
{
    uint8_t frame[YI_LOG_BUFFER_SIZE];
    const char *prefix;
    yi_device_t *console;
    uint32_t length = 0U;

    if((message == NULL) || (level > YI_LOG_LEVEL_ERROR))
    {
        return -1;
    }
    if(level < minimum_level)
    {
        return 0;
    }
    console = yi_console_get_default();
    prefix = yi_log_prefix(level);
    if((console == NULL) || (prefix == NULL))
    {
        return -1;
    }
    while((*prefix != '\0') && (length < (YI_LOG_BUFFER_SIZE - 2U)))
    {
        frame[length++] = (uint8_t)*prefix++;
    }
    while((*message != '\0') && (length < (YI_LOG_BUFFER_SIZE - 2U)))
    {
        frame[length++] = (uint8_t)*message++;
    }
    frame[length++] = '\r';
    frame[length++] = '\n';
    return yi_console_write(console, frame, length);
}

int yi_log_debug(const char *message)   { return yi_log_write(YI_LOG_LEVEL_DEBUG, message); }
int yi_log_info(const char *message)    { return yi_log_write(YI_LOG_LEVEL_INFO, message); }
int yi_log_warning(const char *message) { return yi_log_write(YI_LOG_LEVEL_WARNING, message); }
int yi_log_error(const char *message)   { return yi_log_write(YI_LOG_LEVEL_ERROR, message); }
