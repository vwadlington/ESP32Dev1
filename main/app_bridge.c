#include "app_bridge.h"
#include "minigui.h"
#include "dlogger.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h" // For esp_log_timestamp()

/**
 * @brief Private bridge function that follows the minigui_log_provider_t signature.
 */
static void log_provider_impl(const char *filter, lv_obj_t *table, uint16_t max_rows) {
    const char *path = dlogger_get_current_log_filepath();
    FILE *f = fopen(path, "r");
    if (!f) return;

    // We no longer set headers here if you use the "Two-Table" static header trick
    char line[256];
    uint16_t row = 0; // Start at 0 if the data table is separate from header

    while (fgets(line, sizeof(line), f) && row < max_rows) {
        char extracted_tag[16] = "SYS";
        char *message_body = line;

        // Extract Tag (e.g., [LVGL])
        if (line[0] == '[') {
            char *closing = strchr(line, ']');
            if (closing) {
                size_t tag_len = closing - (line + 1);
                if (tag_len < sizeof(extracted_tag)) {
                    strncpy(extracted_tag, line + 1, tag_len);
                    extracted_tag[tag_len] = '\0';
                    message_body = closing + 1;
                }
            }
        }

        // Filtering Logic
        if (strcmp(filter, "ALL") == 0 || strcmp(extracted_tag, filter) == 0) {
            // Get system time in ms
            char time_str[12];
            snprintf(time_str, sizeof(time_str), "%lu", (unsigned long)esp_log_timestamp());

            lv_table_set_cell_value(table, row, 0, time_str);      // Column 0: TIME
            lv_table_set_cell_value(table, row, 1, extracted_tag); // Column 1: TAG
            lv_table_set_cell_value(table, row, 2, message_body);  // Column 2: MESSAGE
            row++;
        }
    }
    fclose(f);
}

void app_bridge_init(void) {
    // Connect the log provider
    minigui_register_log_provider(log_provider_impl);
    
    // You can also connect your brightness callback here later!
    // minigui_register_brightness_cb(hardware_led_set_brightness);
}