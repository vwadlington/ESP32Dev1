#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "minigui.h"
#include "storage.h"
#include "dlogger.h"
#include "app_bridge.h"
#include <string.h>

static void brightness_wrapper(uint8_t val) {
    bsp_display_brightness_set(val);
}

void app_main(void)
{
    /* Initialize basic services */
    storage_init();
    dlogger_init();

    /* Initialize display hardware via BSP */
    bsp_display_start();
    
    /* Register hardware-specific brightness control */
    minigui_register_brightness_cb(brightness_wrapper);
    
    /* UI Initialization must be within display lock */
    // bsp_display_lock(timeout_ms). Use 0 to wait indefinitely or a value like 100
    if (bsp_display_lock(0))
    {
        minigui_init();
        
        /* Unlock MUST happen inside the if-block where lock was successful */
        bsp_display_unlock();
    }
    //connect minigui and dlogger
    app_bridge_init();
    dlogger_log("Application Initialized and UI Started.");
}
