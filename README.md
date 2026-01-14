# ESP32-S3 Smart Home Controller

A professional-grade ESP-IDF framework project utilizing the ESP32-S3 to control smart home devices via a high-resolution touch interface.

## 🚀 Project Goals
- **Professional UI/UX:** High-quality graphics using **LVGL 9** on an 800x480 RGB screen.
- **Observability:** Unified logging for system and UI events with persistence for field debugging.
- **Portability:** Decoupling the UI manager into a reusable component.

## 🛠️ Hardware Specifications
- **Controller:** ESP32-S3-LCD-EV-Board2 (Espressif)
- **Sub-board:** Sub-board 3 (800x480 RGB interface)
- **Touch Driver:** GT1151
- **Storage:** Internal SPIFFS for logs and settings.

## 🏗️ Architecture & Development
### 1. GUI Manager (minigui)
The GUI is handled by the **minigui** component, which is designed to be a standalone, reusable repository.
* **Repository:** [https://github.com/vwadlington/minigui](https://github.com/vwadlington/minigui)
* **Memory Strategy:** Uses dynamic management and non-persistent screens. Only the active screen is held in RAM to support the high-resolution buffer requirements of the S3.
* **Navigation:** Hamburger menu and side drawer are managed globally via `lv_layer_top()`.

### 2. Unified Logging (dlogger)
Captures and redirects all system and UI output to a circular file system.
* **Streams:** Integrated `ESP_LOG` and `LVGL` log handlers.
* **Rotation:** Max 2 files, 1MB each, stored on the `/storage` partition.
* **Observability:** Logs are viewable via a dedicated **Logs Screen** in the UI with category filtering.

### 3. Planned Screen Layouts
| Screen | Features |
| :--- | :--- |
| **Home** | Outside/Inside Temp, Weather outlook, Clock. |
| **Logs** | Table/Grid view of file logs with filter (ESP/LVGL/USER). |
| **Settings** | Brightness slider, Wi-Fi configuration, System info. |

## 📂 Project Layout
```text
.
├── components
│   ├── dlogger        # Custom logging wrapper
│   ├── minigui        # UI Component (Dynamic screen loader)
│   └── storage        # SPIFFS initialization & management
├── main
│   ├── main.c         # Hardware init and app orchestration
│   └── idf_component  # Managed BSP dependencies
└── partitions.csv     # Custom flash layout (16MB config)