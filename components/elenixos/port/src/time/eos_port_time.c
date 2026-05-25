#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "eos_dev_time.h"

static eos_datetime_t time_get_datetime(void)
{
    eos_datetime_t dt = {0};
    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t now = (time_t)tv.tv_sec;
    struct tm tm_info;
    localtime_r(&now, &tm_info);

    dt.year = (uint16_t)(tm_info.tm_year + 1900);
    dt.month = (uint8_t)(tm_info.tm_mon + 1);
    dt.day = (uint8_t)tm_info.tm_mday;
    dt.hour = (uint8_t)tm_info.tm_hour;
    dt.min = (uint8_t)tm_info.tm_min;
    dt.sec = (uint8_t)tm_info.tm_sec;
    dt.ms = (uint16_t)(tv.tv_usec / 1000);
    dt.day_of_week = (uint8_t)tm_info.tm_wday;

    return dt;
}

static const eos_dev_time_ops_t s_time_ops = {
    .get_datetime = time_get_datetime,
};

void eos_port_time_init(void)
{
    eos_dev_time_register(&s_time_ops);
}
