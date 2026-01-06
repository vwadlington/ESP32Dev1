#include "bsp/esp-bsp.h"
#include "ui.h"
#include "esp_log.h"

//static const char *TAG = "app_main";

void app_main(void)
{
    /* Initialize display */
    bsp_display_start();
    
    create_ui();
}