idf_component_register(SRCS "main.c" "app_bridge.c"
                       INCLUDE_DIRS "."
                       REQUIRES espressif__esp32_s3_lcd_ev_board minigui storage dlogger log)