im tyring to create a professional looking ESP-IDF framework project, using an ESP32-S3 to contol other smart home devices.

I'm using the ESP32-S3-LCD-EV-Board2 with sub board 3 (800x480 RGB Screen, GT1151 touch driver)

I'm using GitHub as my repository "https://github.com/vwadlington/ESP32Dev1"

I plan to use the main folder for reading from sensors and communicating with other devices (wifi)

for graphics
-I will use lvgl initialized by my BSP "https://github.com/espressif/esp-bsp/tree/master/bsp/esp32_s3_lcd_ev_board" (bsp_display_start())
-I will create a reusable independent component "minigui" it will have its own GitHub repo to possibly be reused on other projects (https://github.com/vwadlington/minigui)
-minigui will have multiple screens on a single display setup
-minigui.c will be the screen manager using dynamic management and non persistent screens to save memory
-currently there would be three screens (screen_home, screen_logs, and screen_settings) all under a screen folder and having their own .c files and more screens possibly in the future
--screen_home will have a outside temp, inside temp, weather outlook
--screen_logs will have access to an internal logs file in a table/grid view with a filter for log type (ESP_LOG/LVGL/USER)
--screen_settings will have screen brightness, wifi, and maybe more

for logs
-i would like to grab LVGL logs and send them to an internal logs file
-i would like to grab ESP_LOG events and send them to an internal logs file
-i would like to display the internal logs file with a filter on the logs screen

the project currently has the following structure:
.devcontainer
.github
.vscode
build
components
-minigui
--include
---screens
----screen_home.h
----screen_logs.h
----screen_settings.h
---minigui.h
--src
---screens
----screen_home.c
----screen_logs.c
----screen_settings.c
---minigui.c
--.gitignore
--CMakeLists.txt
--LICENSE
--README.md
main
-CMakeLists.txt
-idf_component.yml
-main.c
managed_components
-BSP generated
.gemini-context.md
.gitignore
.gitmodules
CMakeLists.txt
dependencies.lock
partitions.csv
README.md (this file)
sdkconfig