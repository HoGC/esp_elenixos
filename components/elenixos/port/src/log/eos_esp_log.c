#include "eos_log.h"

#include "esp_rom_sys.h"

static const char *esp_eos_log_level_to_str(eos_log_level_t level)
{
    switch (level) {
    case EOS_LOG_LEVEL_DEBUG:
        return "DEBUG";
    case EOS_LOG_LEVEL_INFO:
        return "INFO";
    case EOS_LOG_LEVEL_WARN:
        return "WARN";
    case EOS_LOG_LEVEL_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

static void esp_eos_log_listener(eos_log_level_t level, const char *buf, size_t len, void *user)
{
    (void)len;
    (void)user;

    esp_rom_printf("[%s] %s\n", esp_eos_log_level_to_str(level), buf);
}

eos_result_t eos_service_log_std_register(void)
{
    eos_log_listener_id_t id = eos_log_register_listener(
        "esp_log",
        esp_eos_log_listener,
        NULL,
        EOS_LOG_FLAG_SYSTEM
    );

    return (id >= 0) ? EOS_OK : EOS_FAILED;
}
