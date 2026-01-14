#include "storage.h"
#include "esp_spiffs.h"
#include "esp_log.h"

static const char *TAG = "storage";
static const char *base_path = "/storage";

esp_err_t storage_init(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = base_path,
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    esp_spiffs_info(conf.partition_label, &total, &used);
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}

const char* storage_get_base_path(void) {
    return base_path;
}