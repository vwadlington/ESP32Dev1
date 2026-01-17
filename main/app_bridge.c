#include "app_bridge.h"
#include "dlogger.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// STATIC HELPERS - MINIMAL STACK USAGE
// ============================================================================

/**
 * @brief Check if raw log passes filter (minimal stack)
 */
static bool log_passes_filter(const dlogger_entry_t *raw, const char *filter) {
    if (!filter || strcmp(filter, "ALL") == 0) {
        return true;
    }
    
    // Convert source to string without function call overhead
    const char* source_str = "?";
    switch(raw->source) {
        case LOG_SOURCE_ESP:  source_str = "ESP"; break;
        case LOG_SOURCE_LVGL: source_str = "LVGL"; break;
        case LOG_SOURCE_USER: source_str = "USER"; break;
    }
    
    return (strcmp(filter, source_str) == 0);
}

/**
 * @brief Format timestamp (minimal stack - uses preallocated buffer)
 */
static void format_timestamp(uint32_t timestamp, char *buf) {
    snprintf(buf, 16, "%lu", (unsigned long)timestamp);
}

/**
 * @brief Format source (minimal stack - direct assignment)
 */
static void format_source(uint8_t source, char *buf) {
    switch(source) {
        case LOG_SOURCE_ESP:  strcpy(buf, "ESP"); break;
        case LOG_SOURCE_LVGL: strcpy(buf, "LVGL"); break;
        case LOG_SOURCE_USER: strcpy(buf, "USER"); break;
        default:              strcpy(buf, "?"); break;
    }
}

/**
 * @brief Format level (minimal stack - direct assignment)
 */
static void format_level(uint8_t level, char *buf) {
    switch(level) {
        case LOG_LEVEL_ERROR: strcpy(buf, "E"); break;
        case LOG_LEVEL_WARN:  strcpy(buf, "W"); break;
        case LOG_LEVEL_INFO:  strcpy(buf, "I"); break;
        case LOG_LEVEL_DEBUG: strcpy(buf, "D"); break;
        default:              strcpy(buf, "?"); break;
    }
}

/**
 * @brief Clean message in-place (no extra buffers)
 */
static void clean_message_inplace(char *msg) {
    int len = strlen(msg);
    while (len > 0 && (msg[len-1] == '\n' || msg[len-1] == '\r')) {
        msg[--len] = '\0';
    }
}

// ============================================================================
// PUBLIC API - MINIMAL STACK VERSION
// ============================================================================
size_t app_bridge_get_formatted_logs(formatted_log_entry_t *logs, 
                                     size_t max_logs, 
                                     const char *filter)
{
    if (!logs || max_logs == 0) return 0;
    
    // Allocate buffer for raw logs
    dlogger_entry_t *raw_logs = (dlogger_entry_t*)malloc(
        max_logs * sizeof(dlogger_entry_t));
    
    if (!raw_logs) return 0;
    
    // Get raw logs from dlogger (already in reverse chronological order)
    size_t raw_count = dlogger_get_raw_entries(raw_logs, max_logs);
    size_t formatted_count = 0;
    
    // Process each raw log
    for (size_t i = 0; i < raw_count && formatted_count < max_logs; i++) {
        const dlogger_entry_t *raw = &raw_logs[i];
        
        // Apply filter
        if (!log_passes_filter(raw, filter)) {
            continue;
        }
        
        // Format the log entry
        formatted_log_entry_t *fmt = &logs[formatted_count];
        
        // Format timestamp
        format_timestamp(raw->timestamp, fmt->timestamp);
        
        // Format source and level
        format_source(raw->source, fmt->source);
        format_level(raw->level, fmt->level);
        
        // Copy and clean message
        strncpy(fmt->message, raw->message, sizeof(fmt->message) - 1);
        fmt->message[sizeof(fmt->message) - 1] = '\0';
        clean_message_inplace(fmt->message);
        
        formatted_count++;
    }
    
    free(raw_logs);
    return formatted_count;
}

void app_bridge_init(void)
{
    // Bridge initialization
    // Note: No stack allocation here
}