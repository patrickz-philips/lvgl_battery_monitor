#ifndef BATTERY_MONITOR_MODEL_H
#define BATTERY_MONITOR_MODEL_H

#include <stdbool.h>
#include <stdint.h>

#include "battery_monitor.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MODEL_EVENT_TYPE_PMU_DATA = 0,
    MODEL_EVENT_TYPE_BUTTON,
} model_event_type_t;

typedef struct {
    model_event_type_t type;
    battery_monitor_data_t pmu_data;
    battery_monitor_button_source_t button_source;
    battery_monitor_button_press_t button_press;
} model_event_t;

esp_err_t model_init(void);
bool model_post_pmu_data(const battery_monitor_data_t * data);
bool model_post_button_event(battery_monitor_button_source_t source, battery_monitor_button_press_t press_type);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_MONITOR_MODEL_H */