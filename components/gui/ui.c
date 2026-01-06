#include "lvgl.h"

static lv_obj_t *label_hello_world;

void create_ui(void)
{
    // Get the active screen
    lv_obj_t *scr = lv_scr_act();

    // Create a label
    label_hello_world = lv_label_create(scr);
    lv_label_set_text(label_hello_world, "Hello World!");

    // Center the label on the screen
    lv_obj_align(label_hello_world, LV_ALIGN_CENTER, 0, 0);
}