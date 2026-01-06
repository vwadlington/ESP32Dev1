idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES log espressif__esp32_s3_lcd_ev_board gui)
