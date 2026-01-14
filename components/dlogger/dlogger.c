#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "storage.h"
#include "dlogger.h"
#include "lvgl.h"

// --- Configuration ---
#define MAX_LOG_FILES 2
#define MAX_LOG_FILE_SIZE (1 * 1024 * 1024) // 1MB
#define LOG_FILE_PREFIX "_LogFile.txt"

// RAM Buffer Settings
#define LOG_RINGBUF_SIZE (8 * 1024)   // 8KB Buffer
#define LOG_TASK_STACK   (4 * 1024)   // 4KB Stack for Writer Task
#define LOG_TASK_PRIO    (tskIDLE_PRIORITY + 2) // Low priority background task

static const char *TAG = "dlogger";

// --- Globals ---
static RingbufHandle_t log_ringbuf = NULL;
static char current_log_filepath[128] = {0}; // Stores full path now for easier access

// --- Helper Functions ---

static void construct_filepath(const char *filename, char *filepath) {
    snprintf(filepath, 128, "%s/%s", storage_get_base_path(), filename);
}

// Finds the oldest file to delete when we need to rotate
static esp_err_t get_oldest_logfile(char *oldest_logfile) {
    DIR *dir = opendir(storage_get_base_path());
    if (!dir) return ESP_FAIL;

    struct dirent *entry;
    long long min_timestamp = -1;
    int log_file_count = 0;
    char found_filename[64] = {0};

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, LOG_FILE_PREFIX)) {
            log_file_count++;
            long long timestamp = atoll(entry->d_name);
            if (min_timestamp == -1 || timestamp < min_timestamp) {
                min_timestamp = timestamp;
                strcpy(found_filename, entry->d_name);
            }
        }
    }
    closedir(dir);

    if (log_file_count >= MAX_LOG_FILES && found_filename[0] != '\0') {
        strcpy(oldest_logfile, found_filename);
        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}

// Creates a new file based on current timestamp
static esp_err_t create_new_logfile(void) {
    long long timestamp = esp_timer_get_time() / 1000;
    char filename[64];
    snprintf(filename, sizeof(filename), "%lld%s", timestamp, LOG_FILE_PREFIX);
    
    // Check if we need to rotate (delete old)
    char oldest_logfile[64];
    if (get_oldest_logfile(oldest_logfile) == ESP_OK) {
        char filepath_to_delete[128];
        construct_filepath(oldest_logfile, filepath_to_delete);
        ESP_LOGI(TAG, "Rotating: Deleting %s", filepath_to_delete);
        remove(filepath_to_delete);
    }

    // Set new current path
    construct_filepath(filename, current_log_filepath);
    
    FILE *f = fopen(current_log_filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to create: %s", current_log_filepath);
        return ESP_FAIL;
    }
    fclose(f);
    ESP_LOGI(TAG, "New log file created: %s", current_log_filepath);

    return ESP_OK;
}

// Scans for the latest existing file to append to
static esp_err_t find_latest_logfile(void) {
    DIR *dir = opendir(storage_get_base_path());
    if (!dir) return ESP_FAIL;

    struct dirent *entry;
    long long max_timestamp = -1;
    char found_filename[64] = {0};

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, LOG_FILE_PREFIX)) {
            long long timestamp = atoll(entry->d_name);
            if (max_timestamp == -1 || timestamp > max_timestamp) {
                max_timestamp = timestamp;
                strcpy(found_filename, entry->d_name);
            }
        }
    }
    closedir(dir);

    if (found_filename[0] != '\0') {
        construct_filepath(found_filename, current_log_filepath);
        ESP_LOGI(TAG, "Appending to: %s", current_log_filepath);
        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}

// --- Background Task ---

static void dlogger_task(void *arg) {
    char *item;
    size_t item_size;

    while (1) {
        // 1. Wait for data in the Ring Buffer (Timeout 1s to allow periodic checks)
        item = (char *)xRingbufferReceive(log_ringbuf, &item_size, pdMS_TO_TICKS(1000));
        
        if (item != NULL) {
            // 2. Check file size limit before writing
            struct stat st;
            if (stat(current_log_filepath, &st) == 0) {
                if (st.st_size >= MAX_LOG_FILE_SIZE) {
                    create_new_logfile(); 
                }
            }

            // 3. Write to file
            FILE *f = fopen(current_log_filepath, "a");
            if (f != NULL) {
                fwrite(item, 1, item_size, f);
                // We add a newline if the item didn't have one, or just trust the logger.
                // Usually dlogger_log adds a newline.
                fclose(f);
            } else {
                // If we can't open the file, we just drop the log to prevent hanging
                // But we print to console to warn
                printf("dlogger: Disk Write Failed!\n");
            }

            // 4. Return item to buffer
            vRingbufferReturnItem(log_ringbuf, (void *)item);
        }
    }
}

// --- Public API ---

esp_err_t dlogger_log(const char *format, ...) {
    if (log_ringbuf == NULL) return ESP_ERR_INVALID_STATE;

    char buffer[256]; // Temp stack buffer
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len < 0) return ESP_FAIL;

    // Ensure newline
    if (len < sizeof(buffer) - 2 && buffer[len-1] != '\n') {
        strcat(buffer, "\n");
        len++;
    }

    // Send to Ring Buffer (Wait 0 ticks - if full, drop log to prevent blocking UI)
    BaseType_t res = xRingbufferSend(log_ringbuf, buffer, len, 0);
    
    if (res != pdTRUE) {
        // Buffer full - optionally count dropped logs
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

// --- Handlers ---

// ESP-IDF Log Handler
static int dlogger_esp_log_handler(const char *format, va_list args) {
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    // Strip trailing newline because dlogger_log adds one or handles it
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    dlogger_log("%s", buffer);
    return vprintf(format, args); // Also print to serial
}

// LVGL v9 Log Handler
static void dlogger_lvgl_log_handler(lv_log_level_t level, const char * buf) {
    // LVGL 9 passes the pre-formatted string in 'buf'
    // We just prepend a tag
    dlogger_log("[LVGL] %s", buf);
}

void dlogger_hook_esp_log(void) {
    esp_log_set_vprintf(dlogger_esp_log_handler);
}

void dlogger_hook_lvgl_log(void) {
    lv_log_register_print_cb(dlogger_lvgl_log_handler);
}

const char* dlogger_get_current_log_filepath(void) {
    return current_log_filepath;
}

esp_err_t dlogger_init(void) {
    esp_err_t err = ESP_OK;

    // 1. Create Ring Buffer
    log_ringbuf = xRingbufferCreate(LOG_RINGBUF_SIZE, RINGBUF_TYPE_NOSPLIT);
    if (log_ringbuf == NULL) {
        ESP_LOGE(TAG, "Failed to create RingBuffer");
        return ESP_ERR_NO_MEM;
    }

    // 2. Initialize File System State
    if (find_latest_logfile() != ESP_OK) {
        ESP_LOGI(TAG, "No log file found, creating new.");
        err = create_new_logfile();
    }

    // 3. Start Background Task
    if (err == ESP_OK) {
        BaseType_t res = xTaskCreate(dlogger_task, "dlogger_task", LOG_TASK_STACK, NULL, LOG_TASK_PRIO, NULL);
        if (res != pdPASS) {
            ESP_LOGE(TAG, "Failed to create logging task");
            return ESP_FAIL;
        }

        // 4. Hook inputs
        dlogger_hook_esp_log();
        dlogger_hook_lvgl_log();
        ESP_LOGI(TAG, "dlogger initialized (RingBuffer: %d bytes)", LOG_RINGBUF_SIZE);
    }
    
    return err;
}