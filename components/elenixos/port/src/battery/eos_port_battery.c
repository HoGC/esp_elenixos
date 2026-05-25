#include <stdbool.h>

#include "eos_dev_battery.h"
#include "eos_service_battery.h"

static void battery_request_update(void)
{
    const eos_battery_raw_t raw = {
        .percent = -1,
        .voltage_mv = -1,
        .current_ma = 0,
        .charging = false,
    };
    eos_battery_report_raw(&raw);
}

static const eos_battery_dev_ops_t s_battery_ops = {
    .request_update = battery_request_update,
};

void eos_port_battery_init(void)
{
    eos_dev_battery_register(&s_battery_ops, EOS_BATTERY_CAPACITY_UNDEFINED);
}
