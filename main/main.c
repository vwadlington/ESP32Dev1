#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "minigui.h"
#include "storage.h"
#include "dlogger.h"
#include "app_bridge.h"
#include "esp_log.h"
#include "lvgl.h"
#include <string.h>

static void lvgl_log_handler(lv_log_level_t lvgl_level, const char *buf) {
    uint8_t log_level = LOG_LEVEL_INFO;
    
    // Map LVGL levels based on observed serial output:
    // level 0 = Trace, 1 = Info, 2 = Warn, 3 = Error, 4 = User
    if (lvgl_level == 3) {          // LV_LOG_LEVEL_ERROR
        log_level = LOG_LEVEL_ERROR;
    } else if (lvgl_level == 2) {   // LV_LOG_LEVEL_WARN
        log_level = LOG_LEVEL_WARN;
    } else if (lvgl_level == 1) {   // LV_LOG_LEVEL_INFO
        log_level = LOG_LEVEL_INFO;
    } else if (lvgl_level == 4) {   // LV_LOG_LEVEL_USER
        // Map USER to INFO so user actions appear in table
        log_level = LOG_LEVEL_INFO;
    } else {                        // LV_LOG_LEVEL_TRACE (0) and others
        log_level = LOG_LEVEL_DEBUG;
    }
    
    dlogger_add_entry(LOG_SOURCE_LVGL, log_level, buf);
}

static void brightness_wrapper(uint8_t val) {
    bsp_display_brightness_set(val);
}

void app_main(void)
{
    /* Wait 10 seconds to allow USB serial monitor to connect */
    ESP_LOGI("MAIN", "Waiting 10 seconds for USB serial monitor to connect...");
    for (int i = 10; i > 0; i--) {
        ESP_LOGI("MAIN", "%d...", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI("MAIN", "Starting application...");

    /* Initialize basic services */
    storage_init();
    dlogger_init();

    /* Initialize display hardware via BSP */
    bsp_display_start();
    
    /* Re-hook LVGL logs after BSP initialization (BSP might override) */
    lv_log_register_print_cb(lvgl_log_handler);
    
    /* Register hardware-specific brightness control */
    minigui_register_brightness_cb(brightness_wrapper);
    
    /* UI Initialization must be within display lock */
    if (bsp_display_lock(0))
    {
        minigui_init();
        bsp_display_unlock();
    }
    
    // Initialize the bridge layer (connects data to UI)
    app_bridge_init();

    // SIMPLE TEST LOGS - wait 2 seconds for everything to initialize
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Generate test logs with DIFFERENT LEVELS
    ESP_LOGE("TEST", "This is an ERROR level log");
    ESP_LOGW("TEST", "This is a WARNING level log");
    ESP_LOGI("TEST", "This is an INFO level log");
    ESP_LOGD("TEST", "This is a DEBUG level log");
    
    // LVGL logs
    LV_LOG_ERROR("LVGL ERROR test");
    LV_LOG_WARN("LVGL WARN test");
    LV_LOG_INFO("LVGL INFO test");
    LV_LOG_USER("User action logged via LVGL");
    
    // User level logs
    dlogger_log("Application Initialized and UI Started.");
    
    // Force a flush to ensure logs are written
    dlogger_force_flush();
}