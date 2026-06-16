/**
 * @file battery_monitor.c
 */

#include "battery_monitor.h"

#include "lvgl.h"

#define PAGE_COUNT 2U

typedef struct {
    lv_obj_t * page_cont[PAGE_COUNT];
    lv_obj_t * page_title;
    lv_obj_t * page_indicator;
    lv_obj_t * temperature_value;
    lv_obj_t * bat_voltage_value;
    lv_obj_t * vbus_voltage_value;
    lv_obj_t * system_voltage_value;
    lv_obj_t * bat_percent_value;
    lv_obj_t * is_charging_value;
    lv_obj_t * is_discharge_value;
    lv_obj_t * is_standby_value;
    lv_obj_t * is_vbus_in_value;
    lv_obj_t * is_vbus_good_value;
    lv_obj_t * charge_status_value;
    lv_obj_t * last_input_value;
    uint8_t page_index;
    battery_monitor_data_t data;
} battery_monitor_ctx_t;

static battery_monitor_ctx_t g_ctx;

static void on_gesture(lv_event_t * e);

static const battery_monitor_data_t g_default_data = {
    .temperature_x10 = 260,
    .bat_voltage_mv = 3850U,
    .vbus_voltage_mv = 5000U,
    .system_voltage_mv = 3300U,
    .bat_percent = 78U,
    .is_charging = 1U,
    .is_discharge = 0U,
    .is_standby = 0U,
    .is_vbus_in = 1U,
    .is_vbus_good = 1U,
    .charge_status = "Fast charge",
};

static void set_label_font(lv_obj_t * label, const lv_font_t * font, lv_color_t color)
{
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
}

static const char * button_source_text(battery_monitor_button_source_t source)
{
    switch(source) {
        case BATTERY_MONITOR_BUTTON_SOURCE_GPIO0:
            return "GPIO0";
        case BATTERY_MONITOR_BUTTON_SOURCE_PWRON:
            return "PWRON";
        default:
            return "Unknown";
    }
}

static const char * button_press_text(battery_monitor_button_press_t press_type)
{
    switch(press_type) {
        case BATTERY_MONITOR_BUTTON_PRESS_SHORT:
            return "short";
        case BATTERY_MONITOR_BUTTON_PRESS_LONG:
            return "long";
        default:
            return "unknown";
    }
}

static lv_obj_t * create_page(lv_obj_t * parent)
{
    lv_obj_t * page = lv_obj_create(parent);
    lv_obj_remove_style_all(page);
    lv_obj_set_size(page, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(page, 18, 0);
    lv_obj_set_style_pad_right(page, 18, 0);
    lv_obj_set_style_pad_top(page, 68, 0);
    lv_obj_set_style_pad_bottom(page, 42, 0);
    lv_obj_set_style_pad_row(page, 10, 0);
    lv_obj_add_flag(page, LV_OBJ_FLAG_GESTURE_BUBBLE);
    return page;
}

static lv_obj_t * create_row(lv_obj_t * parent, const char * name)
{
    lv_obj_t * row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 42);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(row, 12, 0);
    lv_obj_set_style_pad_right(row, 12, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x1A2330), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 6, 0);
    lv_obj_add_flag(row, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_t * name_label = lv_label_create(row);
    lv_label_set_text(name_label, name);
    lv_label_set_long_mode(name_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(name_label, 142);
    set_label_font(name_label, &lv_font_montserrat_14, lv_color_hex(0xAEB8C6));
    lv_obj_add_flag(name_label, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_t * value_label = lv_label_create(row);
    lv_label_set_text(value_label, "--");
    lv_label_set_long_mode(value_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(value_label, 170);
    lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_RIGHT, 0);
    set_label_font(value_label, &lv_font_montserrat_18, lv_color_hex(0xF7FAFF));
    lv_obj_add_flag(value_label, LV_OBJ_FLAG_GESTURE_BUBBLE);

    return value_label;
}

static void refresh_data_labels(void)
{
    int32_t temp_abs_x10 = g_ctx.data.temperature_x10 < 0 ? -g_ctx.data.temperature_x10 : g_ctx.data.temperature_x10;
    lv_label_set_text_fmt(g_ctx.temperature_value, "%s%ld.%ld", g_ctx.data.temperature_x10 < 0 ? "-" : "",
                          (long)(temp_abs_x10 / 10), (long)(temp_abs_x10 % 10));
    lv_label_set_text_fmt(g_ctx.bat_voltage_value, "%lu", (unsigned long)g_ctx.data.bat_voltage_mv);
    lv_label_set_text_fmt(g_ctx.vbus_voltage_value, "%lu", (unsigned long)g_ctx.data.vbus_voltage_mv);
    lv_label_set_text_fmt(g_ctx.system_voltage_value, "%lu", (unsigned long)g_ctx.data.system_voltage_mv);
    lv_label_set_text_fmt(g_ctx.bat_percent_value, "%u", (unsigned int)g_ctx.data.bat_percent);
    lv_label_set_text_fmt(g_ctx.is_charging_value, "%u", (unsigned int)(g_ctx.data.is_charging ? 1U : 0U));
    lv_label_set_text_fmt(g_ctx.is_discharge_value, "%u", (unsigned int)(g_ctx.data.is_discharge ? 1U : 0U));
    lv_label_set_text_fmt(g_ctx.is_standby_value, "%u", (unsigned int)(g_ctx.data.is_standby ? 1U : 0U));
    lv_label_set_text_fmt(g_ctx.is_vbus_in_value, "%u", (unsigned int)(g_ctx.data.is_vbus_in ? 1U : 0U));
    lv_label_set_text_fmt(g_ctx.is_vbus_good_value, "%u", (unsigned int)(g_ctx.data.is_vbus_good ? 1U : 0U));
    lv_label_set_text(g_ctx.charge_status_value, g_ctx.data.charge_status != NULL ? g_ctx.data.charge_status : "Unknown");
}

static void update_page_visibility(void)
{
    for(uint8_t index = 0; index < PAGE_COUNT; index++) {
        if(index == g_ctx.page_index) {
            lv_obj_remove_flag(g_ctx.page_cont[index], LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(g_ctx.page_cont[index], LV_OBJ_FLAG_HIDDEN);
        }
    }

    lv_label_set_text_fmt(g_ctx.page_title, "Battery Monitor %u/%u", (unsigned int)(g_ctx.page_index + 1U), (unsigned int)PAGE_COUNT);
    lv_label_set_text(g_ctx.page_indicator, g_ctx.page_index == 0U ? "<  PAGE 1  >" : "<  PAGE 2  >");
}

static void switch_page(int8_t delta)
{
    int16_t next_page = (int16_t)g_ctx.page_index + delta;
    if(next_page < 0 || next_page >= (int16_t)PAGE_COUNT) {
        return;
    }

    g_ctx.page_index = (uint8_t)next_page;
    update_page_visibility();
}

static void on_gesture(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_GESTURE) {
        return;
    }

    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if(dir == LV_DIR_LEFT) {
        switch_page(1);
        lv_event_stop_bubbling(e);
    }
    else if(dir == LV_DIR_RIGHT) {
        switch_page(-1);
        lv_event_stop_bubbling(e);
    }
}

void battery_monitor_set_data(const battery_monitor_data_t * data)
{
    if(data == NULL) {
        return;
    }

    g_ctx.data = *data;
    if(g_ctx.temperature_value != NULL) {
        refresh_data_labels();
    }
}

void battery_monitor_handle_button_event(battery_monitor_button_source_t source, battery_monitor_button_press_t press_type)
{
    if(g_ctx.last_input_value == NULL) {
        return;
    }

    lv_label_set_text_fmt(g_ctx.last_input_value, "%s %s", button_source_text(source), button_press_text(press_type));
}

void battery_monitor_ui_init(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_clean(scr);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D1118), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(scr, on_gesture, LV_EVENT_GESTURE, NULL);

    lv_obj_t * root = lv_obj_create(scr);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, WIDGET_SCREEN_WIDTH, WIDGET_SCREEN_HEIGHT);
    lv_obj_center(root);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x0D1118), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_add_flag(root, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(root, on_gesture, LV_EVENT_GESTURE, NULL);

    g_ctx.page_title = lv_label_create(root);
    set_label_font(g_ctx.page_title, &lv_font_montserrat_20, lv_color_hex(0xFFFFFF));
    lv_obj_align(g_ctx.page_title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_add_flag(g_ctx.page_title, LV_OBJ_FLAG_GESTURE_BUBBLE);

    g_ctx.page_cont[0] = create_page(root);
    lv_obj_add_event_cb(g_ctx.page_cont[0], on_gesture, LV_EVENT_GESTURE, NULL);
    g_ctx.temperature_value = create_row(g_ctx.page_cont[0], "temperature");
    g_ctx.bat_voltage_value = create_row(g_ctx.page_cont[0], "batVoltage (mV)"); 
    g_ctx.vbus_voltage_value = create_row(g_ctx.page_cont[0], "vbusVoltage (mV)");
    g_ctx.system_voltage_value = create_row(g_ctx.page_cont[0], "systemVoltage (mV)");
    g_ctx.bat_percent_value = create_row(g_ctx.page_cont[0], "batPercent %");

    g_ctx.page_cont[1] = create_page(root);
    lv_obj_add_event_cb(g_ctx.page_cont[1], on_gesture, LV_EVENT_GESTURE, NULL);
    g_ctx.is_charging_value = create_row(g_ctx.page_cont[1], "isCharging");
    g_ctx.is_discharge_value = create_row(g_ctx.page_cont[1], "is Discharge");
    g_ctx.is_standby_value = create_row(g_ctx.page_cont[1], "isStandby");
    g_ctx.is_vbus_in_value = create_row(g_ctx.page_cont[1], "isVbusIn");
    g_ctx.is_vbus_good_value = create_row(g_ctx.page_cont[1], "isVbusGood");
    g_ctx.charge_status_value = create_row(g_ctx.page_cont[1], "Charge status");
    g_ctx.last_input_value = create_row(g_ctx.page_cont[1], "lastInput");

    g_ctx.page_indicator = lv_label_create(root);
    set_label_font(g_ctx.page_indicator, &lv_font_montserrat_14, lv_color_hex(0x7CD7FF));
    lv_obj_align(g_ctx.page_indicator, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_obj_add_flag(g_ctx.page_indicator, LV_OBJ_FLAG_GESTURE_BUBBLE);

    g_ctx.page_index = 0U;
    battery_monitor_set_data(&g_default_data);
    lv_label_set_text(g_ctx.last_input_value, "None");
    update_page_visibility();
}
