#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// PURE DATA STRUCTURES - NO FORMATTING
// ============================================================================

/**
 * @brief Log source enumeration
 * 
 * These values are stored in the `source` field of dlogger_entry_t
 */
typedef enum {
    LOG_SOURCE_ESP  = 0,  ///< ESP-IDF system logs
    LOG_SOURCE_LVGL = 1,  ///< LVGL framework logs  
    LOG_SOURCE_USER = 2,  ///< Application user logs
    LOG_SOURCE_COUNT = 3
} dlogger_source_t;

/**
 * @brief Log level enumeration
 * 
 * These values are stored in the `level` field of dlogger_entry_t
 */
typedef enum {
    LOG_LEVEL_ERROR = 0,  ///< Error conditions
    LOG_LEVEL_WARN  = 1,  ///< Warning conditions
    LOG_LEVEL_INFO  = 2,  ///< Informational messages
    LOG_LEVEL_DEBUG = 3,  ///< Debug-level messages
    LOG_LEVEL_COUNT = 4
} dlogger_level_t;

/**
 * @brief Raw log entry structure (196 bytes total)
 * 
 * This is the pure data structure stored in buffers and files.
 * No formatting or UI-specific fields.
 */
typedef struct {
    uint32_t timestamp;      ///< Milliseconds since boot (esp_timer_get_time() / 1000)
    uint8_t source;          ///< dlogger_source_t value (0=ESP, 1=LVGL, 2=USER)
    uint8_t level;           ///< dlogger_level_t value (0=ERROR, 1=WARN, 2=INFO, 3=DEBUG)
    char message[188];       ///< Raw log message (null-terminated)
} dlogger_entry_t;

/**
 * @brief Buffer statistics structure
 */
typedef struct {
    size_t entries_in_buffer;  ///< Number of entries currently in active buffer
    bool flush_pending;        ///< Whether a buffer flush is pending
    uint8_t active_buffer;     ///< Which buffer is active (0 or 1)
    size_t total_capacity;     ///< Total buffer capacity in entries
} dlogger_stats_t;

// ============================================================================
// PURE DATA APIs - NO UI FORMATTING
// ============================================================================

/**
 * @brief Initialize the double buffer logging system
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t dlogger_init(void);

/**
 * @brief Log a user message (application-level logging)
 * 
 * @param format Format string (printf-style)
 * @param ... Arguments for format string
 * @return ESP_OK on success, ESP_ERR_NO_MEM if buffer full
 */
esp_err_t dlogger_log(const char *format, ...);

/**
 * @brief Get raw log entries from buffer (most recent first)
 * 
 * This is the primary data access API. Returns raw log entries
 * in reverse chronological order (newest first).
 * 
 * @param dest Destination array for log entries
 * @param max_entries Maximum number of entries to copy
 * @return Number of entries actually copied
 */
size_t dlogger_get_raw_entries(dlogger_entry_t *dest, size_t max_entries);

/**
 * @brief Get current buffer statistics
 * 
 * @param stats Pointer to stats structure to fill
 */
void dlogger_get_stats(dlogger_stats_t *stats);

/**
 * @brief Manually trigger a buffer flush
 * 
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if flush already pending
 */
esp_err_t dlogger_force_flush(void);

/**
 * @brief Hook into ESP-IDF logging system
 * 
 * Redirects all ESP_LOGx calls to dlogger
 */
void dlogger_hook_esp_log(void);

/**
 * @brief Add a raw log entry from any source
 * 
 * This is the generic entry point for adding logs to the buffer.
 * 
 * @param source The log source (ESP, LVGL, USER)
 * @param level The log level (ERROR, WARN, INFO, DEBUG)
 * @param message The log message string
 * @return ESP_OK on success, ESP_ERR_NO_MEM if buffer full
 */
esp_err_t dlogger_add_entry(dlogger_source_t source, dlogger_level_t level, const char *message);

/**
 * @brief Get current log file path
 * 
 * @return Path to current log file
 */
const char* dlogger_get_current_log_filepath(void);

/**
 * @brief Deinitialize logging system and free resources
 * 
 * Note: Not typically needed in production, useful for testing
 */
void dlogger_deinit(void);

#ifdef __cplusplus
}
#endif