SET(SOURCES screens/ui_Screen1.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    fonts/ui_font_lcd_mono_30.c
    fonts/ui_font_xlm_42.c)

add_library(ui ${SOURCES})
