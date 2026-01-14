#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the device logger.
 *
 * This sets up the Ring Buffer and starts the background logging task.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t dlogger_init(void);

/**
 * @brief Logs a message to the internal buffer.
 *
 * The message is formatted and sent to a Ring Buffer. A background task
 * will eventually write this to the file system.
 *
 * @param format The format string for the message.
 * @param ... The arguments for the format string.
 * @return ESP_OK on success, ESP_ERR_NO_MEM if buffer is full.
 */
esp_err_t dlogger_log(const char *format, ...);

/**
 * @brief Hooks into the ESP-IDF logging system to redirect logs to dlogger.
 */
void dlogger_hook_esp_log(void);

/**
 * @brief Hooks into the LVGL logging system to redirect logs to dlogger.
 */
void dlogger_hook_lvgl_log(void);

/**
 * @brief Gets the full path of the current log file.
 * * Useful for the UI to know which file to read from.
 *
 * @return The full file path (e.g., "/spiffs/123456_LogFile.txt").
 */
const char* dlogger_get_current_log_filepath(void);

#ifdef __cplusplus
}
#endif