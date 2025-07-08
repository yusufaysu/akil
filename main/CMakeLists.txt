idf_component_register(SRCS "main.c"
                            "ft6x36.c"
                            "touch_lvgl.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_lcd_st7701 lvgl)
