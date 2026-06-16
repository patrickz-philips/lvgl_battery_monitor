#include "model.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lvgl.h"

static const char * TAG = "model";
static const uint32_t MODEL_TIMER_PERIOD_MS = 20U;
static const UBaseType_t MODEL_EVENT_QUEUE_LEN = 16U;

static QueueHandle_t s_event_queue;
static lv_timer_t * s_model_timer;

static bool model_enqueue_event(const model_event_t * event)
{
    if (event == NULL || s_event_queue == NULL) {
        return false;
    }

    if (xQueueSendToBack(s_event_queue, event, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Model queue full, drop event type %u", (unsigned int)event->type);
        return false;
    }

    return true;
}

static void model_apply_event(const model_event_t * event)
{
    if (event == NULL) {
        return;
    }

    if (event->type == MODEL_EVENT_TYPE_PMU_DATA) {
        battery_monitor_set_data(&event->pmu_data);
    } else if (event->type == MODEL_EVENT_TYPE_BUTTON) {
        battery_monitor_handle_button_event(event->button_source, event->button_press);
    }
}

static void model_timer_cb(lv_timer_t * timer)
{
    (void)timer;

    model_event_t event = {0};
    while (s_event_queue != NULL && xQueueReceive(s_event_queue, &event, 0) == pdTRUE) {
        model_apply_event(&event);
    }
}

esp_err_t model_init(void)
{
    if (s_event_queue == NULL) {
        s_event_queue = xQueueCreate(MODEL_EVENT_QUEUE_LEN, sizeof(model_event_t));
        ESP_RETURN_ON_FALSE(s_event_queue != NULL, ESP_ERR_NO_MEM, TAG, "Failed to create model queue");
    }

    if (s_model_timer == NULL) {
        s_model_timer = lv_timer_create(model_timer_cb, MODEL_TIMER_PERIOD_MS, NULL);
        ESP_RETURN_ON_FALSE(s_model_timer != NULL, ESP_ERR_NO_MEM, TAG, "Failed to create model timer");
    }

    lv_timer_ready(s_model_timer);
    return ESP_OK;
}

bool model_post_pmu_data(const battery_monitor_data_t * data)
{
    model_event_t event = {0};
    if (data == NULL) {
        return false;
    }

    event.type = MODEL_EVENT_TYPE_PMU_DATA;
    event.pmu_data = *data;
    return model_enqueue_event(&event);
}

bool model_post_button_event(battery_monitor_button_source_t source, battery_monitor_button_press_t press_type)
{
    model_event_t event = {0};

    event.type = MODEL_EVENT_TYPE_BUTTON;
    event.button_source = source;
    event.button_press = press_type;
    return model_enqueue_event(&event);
}