#include "esp_err.h"

/**
 * @brief Initializes the storage component.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t storage_init(void);

/**
 * @brief Gets the base path of the storage.
 *
 * @return The base path of the storage.
 */
const char* storage_get_base_path(void);