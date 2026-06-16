#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      DEFINES
 *********************/
#define WIDGET_SCREEN_WIDTH  410
#define WIDGET_SCREEN_HEIGHT 502

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    int32_t temperature_x10;
    uint32_t bat_voltage_mv;
    uint32_t vbus_voltage_mv;
    uint32_t system_voltage_mv;
    uint8_t bat_percent;
    uint8_t is_charging;
    uint8_t is_discharge;
    uint8_t is_standby;
    uint8_t is_vbus_in;
    uint8_t is_vbus_good;
    const char * charge_status;
} battery_monitor_data_t;

typedef enum {
    BATTERY_MONITOR_BUTTON_SOURCE_GPIO0 = 0,
    BATTERY_MONITOR_BUTTON_SOURCE_PWRON,
} battery_monitor_button_source_t;

typedef enum {
    BATTERY_MONITOR_BUTTON_PRESS_SHORT = 0,
    BATTERY_MONITOR_BUTTON_PRESS_LONG,
} battery_monitor_button_press_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void battery_monitor_ui_init(void);
void battery_monitor_set_data(const battery_monitor_data_t * data);
void battery_monitor_handle_button_event(battery_monitor_button_source_t source, battery_monitor_button_press_t press_type);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*BATTERY_MONITOR_H*/
