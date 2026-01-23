#include "dlogger.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


// ============================================================================
// INTERNAL CONSTANTS AND TYPES
// ============================================================================

// Buffer configuration
#define LOG_BUFFER_CAPACITY  512    // 512 entries per buffer
#define FLUSH_INTERVAL_MS    500    // Flush every 500ms
#define MAX_MESSAGE_LENGTH   188    // Must match struct definition

// Internal buffer context
typedef struct {
    dlogger_entry_t *buffer_a;      ///< First buffer
    dlogger_entry_t *buffer_b;      ///< Second buffer  
    size_t capacity;                ///< Max entries per buffer
    volatile uint8_t active;        ///< Active buffer (0=buffer_a, 1=buffer_b)
    volatile size_t fill_idx;       ///< Entries in active buffer
    volatile bool flush_pending;    ///< True when inactive buffer needs flushing
    SemaphoreHandle_t mutex;        ///< Mutex for thread-safe operations
    TaskHandle_t flush_task;        ///< Background task handle
    volatile bool task_running;     ///< Controls background task
} dlogger_buffer_ctx_t;

// ============================================================================
// STATIC VARIABLES
// ============================================================================

static const char *TAG = "DLOGGER";
static char current_log_path[64] = "/storage/latest.log";
static FILE *log_file_handle = NULL;

static dlogger_buffer_ctx_t dlogger_ctx = {
    .buffer_a = NULL,
    .buffer_b = NULL,
    .capacity = LOG_BUFFER_CAPACITY,
    .active = 0,
    .fill_idx = 0,
    .flush_pending = false,
    .mutex = NULL,
    .flush_task = NULL,
    .task_running = false
};

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Convert log source to string (for file output only)
 */
static const char* source_to_string(uint8_t source) {
    switch(source) {
        case LOG_SOURCE_ESP:  return "ESP";
        case LOG_SOURCE_LVGL: return "LVGL";
        case LOG_SOURCE_USER: return "USER";
        default:              return "UNKNOWN";
    }
}

/**
 * @brief Convert log level to character (for file output only)
 */
static char level_to_char(uint8_t level) {
    switch(level) {
        case LOG_LEVEL_ERROR: return 'E';
        case LOG_LEVEL_WARN:  return 'W';
        case LOG_LEVEL_INFO:  return 'I';
        case LOG_LEVEL_DEBUG: return 'D';
        default:              return '?';
    }
}

/**
 * @brief Ensure log file is open (for internal file writing only)
 */
static void ensure_log_file_open(void) {
    if (log_file_handle == NULL) {
        log_file_handle = fopen(current_log_path, "a");
        if (log_file_handle) {
            setvbuf(log_file_handle, NULL, _IOFBF, 4096);
        }
    }
}

/**
 * @brief Close log file
 */
static void close_log_file(void) {
    if (log_file_handle) {
        fclose(log_file_handle);
        log_file_handle = NULL;
    }
}

/**
 * @brief Write single entry to file (internal use only)
 */
static void write_entry_to_file(const dlogger_entry_t *entry) {
    ensure_log_file_open();
    if (!log_file_handle || !entry) return;
    
    fprintf(log_file_handle, "%" PRIu32 " [%s][%c] %s\n",
            entry->timestamp,
            source_to_string(entry->source),
            level_to_char(entry->level),
            entry->message);
    
    fflush(log_file_handle);
}

/**
 * @brief Flush buffer to file (internal use only)
 */
static void flush_buffer_to_file(dlogger_entry_t *buffer, size_t count) {
    if (!buffer || count == 0) return;
    
    for (size_t i = 0; i < count; i++) {
        if (buffer[i].timestamp != 0) {
            write_entry_to_file(&buffer[i]);
        }
    }
    
    memset(buffer, 0, count * sizeof(dlogger_entry_t));
}

// ============================================================================
// BUFFER MANAGEMENT
// ============================================================================

/**
 * @brief Background flush task function
 */
static void flush_task_func(void *arg) {
    while (dlogger_ctx.task_running) {
        bool should_flush = false;
        dlogger_entry_t *buffer_to_flush = NULL;
        size_t entries_to_flush = 0;
        
        // Check if flush is needed
        xSemaphoreTake(dlogger_ctx.mutex, portMAX_DELAY);
        if (dlogger_ctx.flush_pending) {
            uint8_t buffer_to_flush_idx = !dlogger_ctx.active;
            buffer_to_flush = (buffer_to_flush_idx == 0) ? 
                             dlogger_ctx.buffer_a : dlogger_ctx.buffer_b;
            entries_to_flush = dlogger_ctx.capacity;
            should_flush = true;
            dlogger_ctx.flush_pending = false;
        }
        xSemaphoreGive(dlogger_ctx.mutex);
        
        // Perform flush if needed
        if (should_flush) {
            flush_buffer_to_file(buffer_to_flush, entries_to_flush);
        }
        
        vTaskDelay(pdMS_TO_TICKS(FLUSH_INTERVAL_MS));
    }
    
    vTaskDelete(NULL);
}

/**
 * @brief Add entry to active buffer (thread-safe)
 */
static bool buffer_add_entry(uint8_t source, uint8_t level, const char *message) {
    bool success = false;
    
    xSemaphoreTake(dlogger_ctx.mutex, portMAX_DELAY);
    
    // Check if buffer is full
    if (dlogger_ctx.fill_idx >= dlogger_ctx.capacity) {
        if (dlogger_ctx.flush_pending) {
            // Both buffers busy - drop entry
            success = false;
        } else {
            // Swap buffers
            dlogger_ctx.active = !dlogger_ctx.active;
            dlogger_ctx.fill_idx = 0;
            dlogger_ctx.flush_pending = true;
            success = true;
        }
    } else {
        success = true;
    }
    
    // Add entry if we have space
    if (success) {
        dlogger_entry_t *active_buffer = (dlogger_ctx.active == 0) ? 
                                        dlogger_ctx.buffer_a : dlogger_ctx.buffer_b;
        
        dlogger_entry_t *entry = &active_buffer[dlogger_ctx.fill_idx];
        entry->timestamp = (uint32_t)(esp_timer_get_time() / 1000);
        entry->source = source;
        entry->level = level;
        
        // Copy message with null termination
        strncpy(entry->message, message, MAX_MESSAGE_LENGTH - 1);
        entry->message[MAX_MESSAGE_LENGTH - 1] = '\0';
        
        dlogger_ctx.fill_idx++;
    }
    
    xSemaphoreGive(dlogger_ctx.mutex);
    return success;
}

// ============================================================================
// LOG HANDLERS (NO UI DEPENDENCIES)
// ============================================================================

/**
 * @brief ESP-IDF log handler - parses level from ESP log format
 */
static int esp_log_handler(const char *format, va_list args) {
    char message[256];
    vsnprintf(message, sizeof(message), format, args);
    
    // Parse ESP log level from first character
    // Format: "E (1234) tag: message", "W (1234) tag: message", etc.
    uint8_t log_level = LOG_LEVEL_INFO;
    if (message[0] == 'E') {
        log_level = LOG_LEVEL_ERROR;
    } else if (message[0] == 'W') {
        log_level = LOG_LEVEL_WARN;
    } else if (message[0] == 'I') {
        log_level = LOG_LEVEL_INFO;
    } else if (message[0] == 'D' || message[0] == 'V') {
        log_level = LOG_LEVEL_DEBUG;
    }
    
    // Add to buffer (skip the printf return for actual logging)
    buffer_add_entry(LOG_SOURCE_ESP, log_level, message);
    
    // Pass through to original output
    return vprintf(format, args);
}



// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

esp_err_t dlogger_init(void) {
    // Allocate buffers (prefer PSRAM)
    size_t buffer_size = LOG_BUFFER_CAPACITY * sizeof(dlogger_entry_t);
    
    dlogger_ctx.buffer_a = (dlogger_entry_t*)heap_caps_malloc(
        buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    dlogger_ctx.buffer_b = (dlogger_entry_t*)heap_caps_malloc(
        buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    
    // Fall back to SRAM if PSRAM fails
    if (!dlogger_ctx.buffer_a || !dlogger_ctx.buffer_b) {
        ESP_LOGE(TAG, "PSRAM allocation failed, using SRAM");
        if (dlogger_ctx.buffer_a) free(dlogger_ctx.buffer_a);
        if (dlogger_ctx.buffer_b) free(dlogger_ctx.buffer_b);
        
        dlogger_ctx.buffer_a = (dlogger_entry_t*)malloc(buffer_size);
        dlogger_ctx.buffer_b = (dlogger_entry_t*)malloc(buffer_size);
        
        if (!dlogger_ctx.buffer_a || !dlogger_ctx.buffer_b) {
            ESP_LOGE(TAG, "SRAM allocation failed");
            return ESP_ERR_NO_MEM;
        }
    }
    
    // Initialize buffers
    memset(dlogger_ctx.buffer_a, 0, buffer_size);
    memset(dlogger_ctx.buffer_b, 0, buffer_size);
    
    // Create mutex
    dlogger_ctx.mutex = xSemaphoreCreateMutex();
    if (!dlogger_ctx.mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        free(dlogger_ctx.buffer_a);
        free(dlogger_ctx.buffer_b);
        return ESP_ERR_NO_MEM;
    }
    
    // Start background flush task
    dlogger_ctx.task_running = true;
    BaseType_t task_created = xTaskCreatePinnedToCore(
        flush_task_func,
        "dlogger_flush",
        4096,
        NULL,
        tskIDLE_PRIORITY + 1,
        &dlogger_ctx.flush_task,
        PRO_CPU_NUM
    );
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create flush task");
        vSemaphoreDelete(dlogger_ctx.mutex);
        free(dlogger_ctx.buffer_a);
        free(dlogger_ctx.buffer_b);
        return ESP_FAIL;
    }
    
    // Hook into logging systems
    dlogger_hook_esp_log();
    
    
    // Log initialization
    dlogger_log("DLogger initialized with double buffer system");
    ESP_LOGI(TAG, "Double buffer logging initialized. Capacity: %d entries", 
             LOG_BUFFER_CAPACITY);
    
    return ESP_OK;
}

esp_err_t dlogger_log(const char *format, ...) {
    char message[256];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    bool success = buffer_add_entry(LOG_SOURCE_USER, LOG_LEVEL_INFO, message);
    return success ? ESP_OK : ESP_ERR_NO_MEM;
}

size_t dlogger_get_raw_entries(dlogger_entry_t *dest, size_t max_entries) {
    if (!dest || max_entries == 0) return 0;
    
    size_t entries_copied = 0;
    
    xSemaphoreTake(dlogger_ctx.mutex, portMAX_DELAY);
    
    // Get active buffer
    dlogger_entry_t *active_buffer = (dlogger_ctx.active == 0) ? 
                                    dlogger_ctx.buffer_a : dlogger_ctx.buffer_b;
    
    // Copy in reverse order (newest first)
    for (size_t i = 0; i < dlogger_ctx.fill_idx && i < max_entries; i++) {
        size_t src_idx = dlogger_ctx.fill_idx - 1 - i;
        memcpy(&dest[i], &active_buffer[src_idx], sizeof(dlogger_entry_t));
        entries_copied++;
    }
    
    xSemaphoreGive(dlogger_ctx.mutex);
    
    return entries_copied;
}

void dlogger_get_stats(dlogger_stats_t *stats) {
    if (!stats) return;
    
    xSemaphoreTake(dlogger_ctx.mutex, portMAX_DELAY);
    
    stats->entries_in_buffer = dlogger_ctx.fill_idx;
    stats->flush_pending = dlogger_ctx.flush_pending;
    stats->active_buffer = dlogger_ctx.active;
    stats->total_capacity = dlogger_ctx.capacity;
    
    xSemaphoreGive(dlogger_ctx.mutex);
}

esp_err_t dlogger_force_flush(void) {
    esp_err_t result = ESP_FAIL;
    
    xSemaphoreTake(dlogger_ctx.mutex, portMAX_DELAY);
    
    if (!dlogger_ctx.flush_pending && dlogger_ctx.fill_idx > 0) {
        dlogger_ctx.active = !dlogger_ctx.active;
        dlogger_ctx.flush_pending = true;
        dlogger_ctx.fill_idx = 0;
        result = ESP_OK;
    } else {
        result = ESP_ERR_INVALID_STATE;
    }
    
    xSemaphoreGive(dlogger_ctx.mutex);
    
    return result;
}

void dlogger_hook_esp_log(void) {
    esp_log_set_vprintf(esp_log_handler);
}

esp_err_t dlogger_add_entry(dlogger_source_t source, dlogger_level_t level, const char *message) {
    bool success = buffer_add_entry(source, level, message);
    return success ? ESP_OK : ESP_ERR_NO_MEM;
}

const char* dlogger_get_current_log_filepath(void) {
    return current_log_path;
}

void dlogger_deinit(void) {
    // Stop background task
    dlogger_ctx.task_running = false;
    if (dlogger_ctx.flush_task) {
        vTaskDelete(dlogger_ctx.flush_task);
        dlogger_ctx.flush_task = NULL;
    }
    
    // Flush any remaining logs
    xSemaphoreTake(dlogger_ctx.mutex, portMAX_DELAY);
    
    if (dlogger_ctx.fill_idx > 0) {
        dlogger_entry_t *active_buffer = (dlogger_ctx.active == 0) ? 
                                        dlogger_ctx.buffer_a : dlogger_ctx.buffer_b;
        flush_buffer_to_file(active_buffer, dlogger_ctx.fill_idx);
    }
    
    if (dlogger_ctx.flush_pending) {
        dlogger_entry_t *inactive_buffer = (dlogger_ctx.active == 0) ? 
                                          dlogger_ctx.buffer_b : dlogger_ctx.buffer_a;
        flush_buffer_to_file(inactive_buffer, dlogger_ctx.capacity);
    }
    
    xSemaphoreGive(dlogger_ctx.mutex);
    
    // Cleanup
    close_log_file();
    
    if (dlogger_ctx.buffer_a) free(dlogger_ctx.buffer_a);
    if (dlogger_ctx.buffer_b) free(dlogger_ctx.buffer_b);
    dlogger_ctx.buffer_a = NULL;
    dlogger_ctx.buffer_b = NULL;
    
    if (dlogger_ctx.mutex) {
        vSemaphoreDelete(dlogger_ctx.mutex);
        dlogger_ctx.mutex = NULL;
    }
}