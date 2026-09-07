#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include "ui_milktea.h"
#include "client/client.h"

static lv_display_t *hal_init(int32_t w, int32_t h)
{
    lv_group_set_default(lv_group_create());

    lv_display_t *disp = lv_sdl_window_create(w, h);

    lv_indev_t *mouse = lv_sdl_mouse_create();
    lv_indev_set_group(mouse, lv_group_get_default());
    lv_indev_set_display(mouse, disp);
    lv_display_set_default(disp);

    lv_indev_t *mousewheel = lv_sdl_mousewheel_create();
    lv_indev_set_display(mousewheel, disp);

    lv_indev_t *keyboard = lv_sdl_keyboard_create();
    lv_indev_set_display(keyboard, disp);
    lv_indev_set_group(keyboard, lv_group_get_default());

    return disp;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    /*Initialize LVGL*/
    lv_init();

    /*Initialize the display, and the input devices*/
    hal_init(1024, 600);
    
    /*Initialize the client*/
    client_init("127.0.0.1", 50021);
    /*receive*/
    recv_pthread_init();
    /*打开点餐界面*/
    ui_milktea_create();

    while (1)
    {
        /* Periodically call the lv_task handler. */
        lv_timer_handler();
        usleep(10 * 1000);
    }

    return 0;
}
