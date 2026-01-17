#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "minigui.h"
#include "storage.h"
#include "dlogger.h"
#include "app_bridge.h"
#include "esp_log.h"
#include "lvgl.h"
#include <string.h>

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
    dlogger_hook_lvgl_log();
    
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