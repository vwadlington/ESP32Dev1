#include "bsp/esp-bsp.h"
#include "minigui.h"
#include "esp_log.h"

//static const char *TAG = "app_main";

void app_main(void)
{
    /* Initialize display */
    bsp_display_start();
    
    if (bsp_display_lock(0))
    {
        minigui_init();
    }
    bsp_display_unlock();
}
