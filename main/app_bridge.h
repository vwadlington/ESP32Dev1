#ifndef APP_BRIDGE_H
#define APP_BRIDGE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_BRIDGE_MAX_LOGS 50

// ============================================================================
// BRIDGE LAYER DATA STRUCTURES (Formatted for UI)
// ============================================================================

/**
 * @brief Formatted log entry for UI display
 * 
 * This is the ONLY structure the UI layer should see.
 * All data transformation happens in the bridge.
 */
typedef struct {
    char timestamp[16];  // Formatted: "12345678"
    char source[8];      // Formatted: "ESP", "LVGL", "USER"
    char level[2];       // Formatted: "E", "W", "I", "D"
    char message[100];   // Truncated/cleaned message
} formatted_log_entry_t;

// ============================================================================
// BRIDGE LAYER APIs - DATA TRANSFORMATION ONLY
// ============================================================================

/**
 * @brief Initialize the bridge layer
 * 
 * This connects the data layer (dlogger) to the UI layer (minigui)
 * without creating direct dependencies.
 */
void app_bridge_init(void);

/**
 * @brief Get formatted logs for UI display
 * 
 * This is the primary API for the UI layer to get display-ready logs.
 * It performs all necessary filtering and formatting.
 * 
 * @param logs Destination array for formatted logs
 * @param max_logs Maximum number of logs to return
 * @param filter Filter string ("ALL", "ESP", "LVGL", "USER")
 * @return Number of logs actually returned
 */
size_t app_bridge_get_formatted_logs(formatted_log_entry_t *logs, 
                                     size_t max_logs, 
                                     const char *filter);

#ifdef __cplusplus
}
#endif

#endif // APP_BRIDGE_H