idf_component_register(SRCS "dlogger.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "storage"
                    PRIV_REQUIRES esp_timer log lvgl__lvgl esp_ringbuf)
