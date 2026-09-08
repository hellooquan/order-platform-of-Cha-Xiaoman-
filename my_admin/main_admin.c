/**
 * ============================================================
 *  茶小满 · 服务端后台管理 —— 程序入口(SDL 窗口 1024x600)
 * ============================================================
 *  编译方式(在你的 client 工程里,与点餐机 main 共存):
 *    1) 把 ui_admin.c / ui_admin.h / main_admin.c 拷进 client/ 目录;
 *    2) CMakeLists.txt 里在 add_executable(main ...) 附近追加:
 *
 *       add_executable(admin main_admin.c ui_admin.c)
 *       target_compile_definitions(admin PRIVATE LV_CONF_INCLUDE_SIMPLE)
 *       target_link_libraries(admin lvgl lvgl::examples lvgl::demos
 *                             lvgl::thorvg ${SDL2_LIBRARIES} m pthread)
 *
 *    3) cmake --build build && ./bin/admin
 * ============================================================
 */
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include "ui_admin.h"

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

    lv_init();

    hal_init(1024, 600);

    ui_admin_create();

    while (1)
    {
        lv_timer_handler();
        usleep(10 * 1000);
    }

    return 0;
}
