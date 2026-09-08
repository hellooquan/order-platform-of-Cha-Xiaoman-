/**
 * ============================================================
 *  茶小满 · 服务端后台管理 UI   (LVGL 9.2.x / 1024x600 / SDL)
 * ------------------------------------------------------------
 *  左边:奶茶价格设置(5 款,[+]/[-] 调价 → 保存)
 *  右边:点餐信息查看(order_list.txt,2 秒自动刷新 + 手动刷新)
 *
 *  数据文件(路径可在编译时用宏覆盖):
 *    价格: server/milktea_prices.txt
 *    订单: server/order_list.txt
 *
 *  中文字体:运行时 TinyTTF 加载 Noto CJK(与点餐机一致)
 * ============================================================
 */
#include "lvgl/lvgl.h"
#include "ui_admin.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if LV_USE_TINY_TTF
    #include "lvgl/src/libs/tiny_ttf/lv_tiny_ttf.h"
#endif

/* ======================== 可配置项 ======================== */

#ifndef ADMIN_PRC_PATH
#define ADMIN_PRC_PATH "/home/hyq/project/order_platrorm/my_server/milktea_prices.txt"
#endif
#ifndef ADMIN_ORD_PATH
#define ADMIN_ORD_PATH "/home/hyq/project/order_platrorm/my_server/order_list.txt"
#endif

/* 订单预览最多读文件尾部多少字节 */
#define ADMIN_ORD_TAIL  20000
#define ADMIN_SCREEN_W  1024
#define ADMIN_SCREEN_H  600

/* ======================== 配色(与品牌一致) ======================== */
#define A_BG1       0xFDF3E3
#define A_BG2       0xF6E2C8
#define A_COCOA     0x5A3619
#define A_BROWN     0x8C6A48
#define A_LIGHT_BR  0xB08A5F
#define A_CARAMEL   0xB5763A
#define A_CARAMEL2  0xDEA45F
#define A_CARD      0xFFFFFF
#define A_GREEN_TX  0x3E7C4F
#define A_SAVE_BG1  0xFFCE3D  /* 保存按钮:金黄(上) */
#define A_SAVE_BG2  0xF5911B  /* 保存按钮:渐变橙(下) */
#define A_SAVE_TXT  0x3F2410  /* 保存按钮文字:深咖啡(不用白) */
#define A_SAVE_EDGE 0xC2410C  /* 保存按钮描边:深橘红 */

/* ======================== 饮品数据 ======================== */

#define ITEM_NUM UI_ADMIN_PRODUCT_NUM

static const char *ADMIN_NAMES[ITEM_NUM] = {
    "招牌珍珠奶茶", "芋泥啵啵鲜奶", "杨枝甘露", "葡萄冻冻", "四季春柠檬茶",
};
static const int ADMIN_DEFAULT_PRICE[ITEM_NUM] = { 15, 18, 20, 16, 12 };

/* ======================== 全局状态 ======================== */

static int       s_price[ITEM_NUM];                /* 当前价格(元) */
static lv_obj_t *s_price_lbls[ITEM_NUM];           /* 每行价格 label */
static lv_obj_t *s_msg_lbl = NULL;                 /* 左下状态提示 */
static lv_obj_t *s_ord_lbl = NULL;                 /* 订单文本 label */
static lv_obj_t *s_ord_sc = NULL;                  /* 订单滚动容器 */
static long      s_ord_size = -1;                  /* 上次读到的大小 */

/* ---------- 中文字体 ---------- */
static int        s_zh_ok = 0;
static uint8_t   *s_ttf_buf = NULL;
static size_t     s_ttf_len = 0;
static const lv_font_t *s_f16 = NULL;
static const lv_font_t *s_f20 = NULL;
static const lv_font_t *s_f24 = NULL;
static const lv_font_t *s_f28 = NULL;
static const lv_font_t *s_f36 = NULL;

static void zh_fonts_init(void)
{
    const lv_font_t *f16 = &lv_font_montserrat_16;
    const lv_font_t *f20 = &lv_font_montserrat_20;
    const lv_font_t *f24 = &lv_font_montserrat_24;
    const lv_font_t *f28 = &lv_font_montserrat_28;
    const lv_font_t *f36 = &lv_font_montserrat_36;

#if LV_USE_TINY_TTF
    const char *paths[] = {
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc",
        NULL,
    };
    int i;
    for (i = 0; paths[i]; i++)
    {
        FILE *fp = fopen(paths[i], "rb");
        if (!fp)
            continue;
        fseek(fp, 0, SEEK_END);
        long sz = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (sz <= 0)
        {
            fclose(fp);
            continue;
        }
        uint8_t *buf = (uint8_t *)malloc((size_t)sz);
        if (!buf)
        {
            fclose(fp);
            break;
        }
        if (fread(buf, 1, (size_t)sz, fp) != (size_t)sz)
        {
            free(buf);
            fclose(fp);
            continue;
        }
        fclose(fp);

        s_ttf_buf = buf;
        s_ttf_len = (size_t)sz;

        lv_font_t *t16 = lv_tiny_ttf_create_data(s_ttf_buf, s_ttf_len, 16);
        lv_font_t *t20 = lv_tiny_ttf_create_data(s_ttf_buf, s_ttf_len, 20);
        lv_font_t *t24 = lv_tiny_ttf_create_data(s_ttf_buf, s_ttf_len, 24);
        lv_font_t *t28 = lv_tiny_ttf_create_data(s_ttf_buf, s_ttf_len, 28);
        lv_font_t *t36 = lv_tiny_ttf_create_data(s_ttf_buf, s_ttf_len, 36);
        if (t16 && t20 && t24 && t28 && t36)
        {
            f16 = t16; f20 = t20; f24 = t24; f28 = t28; f36 = t36;
            s_zh_ok = 1;
        }
        break;
    }
#endif
    s_f16 = f16; s_f20 = f20; s_f24 = f24; s_f28 = f28; s_f36 = f36;
}

/* ======================== 小工具 ======================== */

static lv_obj_t *mk_label(lv_obj_t *parent, const char *text,
                          const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    return l;
}

static void set_status(const char *zh, const char *en)
{
    if (!s_msg_lbl)
        return;
    lv_label_set_text(s_msg_lbl, s_zh_ok ? zh : en);
}

/* ======================== 价格:文件读写 ======================== */

static void price_row_refresh(uint8_t idx)
{
    if (idx >= ITEM_NUM)
        return;
    char t[16];
    snprintf(t, sizeof(t), s_zh_ok ? "¥%d" : "%d", s_price[idx]);
    if (s_price_lbls[idx])
        lv_label_set_text(s_price_lbls[idx], t);
}

/* 保存价格到文件(简单文本:名字=价格,一行一款) */
static void admin_save_prices(void)
{
    FILE *fp = fopen(ADMIN_PRC_PATH, "w");
    if (!fp)
    {
        set_status("保存失败:无法打开 milktea_prices.txt", "SAVE FAIL: file open error");
        return;
    }
    fprintf(fp, "# cha-xiao-man prices (admin ui)\n");
    int i;
    for (i = 0; i < ITEM_NUM; i++)
        fprintf(fp, "%s=%d\n", ADMIN_NAMES[i], s_price[i]);
    fclose(fp);
    set_status("已保存 ✓ milktea_prices.txt", "SAVED -> milktea_prices.txt");
}

/* 启动时尝试从文件读价格,读不到就用默认值 */
static void admin_load_prices(void)
{
    FILE *fp = fopen(ADMIN_PRC_PATH, "r");
    if (!fp)
    {
        set_status("未找到价格文件,已用默认价格", "no price file, using defaults");
        return;
    }
    char line[128];
    int changed = 0;
    while (fgets(line, sizeof(line), fp))
    {
        char *eq = strchr(line, '=');
        if (!eq)
            continue;
        *eq = '\0';
        int v = atoi(eq + 1);
        if (v < 1)
            v = 1;
        if (v > 99)
            v = 99;
        int i;
        for (i = 0; i < ITEM_NUM; i++)
        {
            if (strcmp(line, ADMIN_NAMES[i]) == 0)
            {
                s_price[i] = v;
                changed = 1;
                break;
            }
        }
    }
    fclose(fp);
    if (changed)
        set_status("已加载价格文件 milktea_prices.txt", "loaded price file");
}

/* ======================== 价格调价按钮 ======================== */

static void price_plus_cb(lv_event_t *e)
{
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    ui_admin_price_set(idx, s_price[idx] + 1);
}

static void price_minus_cb(lv_event_t *e)
{
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    ui_admin_price_set(idx, s_price[idx] - 1);
}

static void btn_default_cb(lv_event_t *e)
{
    (void)e;
    int i;
    for (i = 0; i < ITEM_NUM; i++)
    {
        s_price[i] = ADMIN_DEFAULT_PRICE[i];
        price_row_refresh((uint8_t)i);
    }
    set_status("已恢复默认价格(未保存)", "defaults restored (not saved)");
}

static void btn_save_cb(lv_event_t *e)
{
    (void)e;
    admin_save_prices();
}

/* ======================== 订单:文件读取与刷新 ======================== */

static void orders_load(int force)
{
    FILE *fp = fopen(ADMIN_ORD_PATH, "rb");
    if (!fp)
    {
        if (force)
            lv_label_set_text(s_ord_lbl,
                              s_zh_ok ? "(未找到 order_list.txt\n请先启动 server 服务)"
                                      : "(order_list.txt not found.\nstart server first.)");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    long start = (len > ADMIN_ORD_TAIL) ? (len - ADMIN_ORD_TAIL) : 0;
    size_t want = (size_t)(len - start);
    char *buf = (char *)malloc(want + 1);
    if (!buf)
    {
        fclose(fp);
        return;
    }
    fseek(fp, start, SEEK_SET);
    size_t got = fread(buf, 1, want, fp);
    fclose(fp);
    buf[got] = '\0';

    s_ord_size = len;

    /* 头部拼一行提示,再拼内容 */
    char *text = (char *)malloc(got + 128);
    if (!text)
    {
        free(buf);
        return;
    }
    if (s_zh_ok)
        snprintf(text, got + 128, "order_list.txt · %ld 字节(尾部)\n──────────────\n%s",
                 len, buf);
    else
        snprintf(text, got + 128, "order_list.txt · %ld bytes (tail)\n--------------\n%s",
                 len, buf);
    lv_label_set_text(s_ord_lbl, text);
    /* 新订单/手动刷新后,自动滚到底部,方便直接看到最新一单 */
    if (s_ord_sc)
    {
        lv_obj_update_layout(s_ord_sc);
        lv_obj_scroll_to_y(s_ord_sc, LV_COORD_MAX, LV_ANIM_OFF);
    }
    free(text);
    free(buf);
}

/* 2 秒定时器:文件大小变了就重读 */
static void order_timer_cb(lv_timer_t *t)
{
    (void)t;
    struct stat st;
    if (stat(ADMIN_ORD_PATH, &st) != 0)
        return;
    if (st.st_size != s_ord_size)
        orders_load(0);
}

static void btn_refresh_cb(lv_event_t *e)
{
    (void)e;
    orders_load(1);
}

/* ======================== 面板创建 ======================== */

/* 一张白色圆角面板 */
static lv_obj_t *mk_panel(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_remove_style_all(p);
    lv_obj_set_pos(p, x, y);
    lv_obj_set_size(p, w, h);
    lv_obj_set_style_bg_color(p, lv_color_hex(A_CARD), 0);
    lv_obj_set_style_radius(p, 24, 0);
    lv_obj_set_style_shadow_width(p, 22, 0);
    lv_obj_set_style_shadow_color(p, lv_color_hex(0x6B4626), 0);
    lv_obj_set_style_shadow_opa(p, 64, 0); /* 约 25% */
    lv_obj_set_style_shadow_ofs_y(p, 4, 0);
    return p;
}

/* 一行价格行 */
static void create_price_row(lv_obj_t *card, int idx, int y)
{
    const int RW = 470 - 36; /* 面板宽 470,左右各 18 */
    const int RH = 50;

    lv_obj_t *row = lv_obj_create(card);
    lv_obj_remove_style_all(row);
    lv_obj_set_pos(row, 18, y);
    lv_obj_set_size(row, RW, RH);
    lv_obj_set_style_bg_color(row, lv_color_hex(0xFFFBF2), 0);
    lv_obj_set_style_radius(row, 16, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0xEFD9BA), 0);

    /* 名称 */
    lv_obj_t *name = mk_label(row, s_zh_ok ? ADMIN_NAMES[idx] : "TEA",
                              s_zh_ok ? s_f20 : &lv_font_montserrat_16,
                              lv_color_hex(A_COCOA));
    lv_obj_align(name, LV_ALIGN_LEFT_MID, 16, 0);

    /* [+] 最右 */
    lv_obj_t *plus = lv_obj_create(row);
    lv_obj_remove_style_all(plus);
    lv_obj_set_size(plus, 34, 34);
    lv_obj_set_style_radius(plus, 999, 0);
    lv_obj_set_style_bg_color(plus, lv_color_hex(A_CARAMEL2), 0);
    lv_obj_set_style_border_width(plus, 2, 0);
    lv_obj_set_style_border_color(plus, lv_color_hex(A_CARAMEL), 0);
    lv_obj_set_style_translate_y(plus, 2, LV_STATE_PRESSED);
    lv_obj_add_flag(plus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(plus, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(plus, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_t *pt = mk_label(plus, "+", &lv_font_montserrat_28,
                            lv_color_hex(0x5A2C08)); /* 深棕,显眼且不是白色 */
    lv_obj_align(pt, LV_ALIGN_CENTER, 0, -1);
    lv_obj_add_event_cb(plus, price_plus_cb, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)idx);

    /* [-] 在 [+] 左边 */
    lv_obj_t *minus = lv_obj_create(row);
    lv_obj_remove_style_all(minus);
    lv_obj_set_size(minus, 34, 34);
    lv_obj_set_style_radius(minus, 999, 0);
    lv_obj_set_style_bg_color(minus, lv_color_hex(0xFFF1DE), 0);
    lv_obj_set_style_border_width(minus, 2, 0);
    lv_obj_set_style_border_color(minus, lv_color_hex(A_CARAMEL), 0);
    lv_obj_set_style_translate_y(minus, 2, LV_STATE_PRESSED);
    lv_obj_add_flag(minus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(minus, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(minus, LV_ALIGN_RIGHT_MID, -120, 0);
    lv_obj_t *mt = mk_label(minus, "-", &lv_font_montserrat_28,
                            lv_color_hex(A_COCOA));
    lv_obj_align(mt, LV_ALIGN_CENTER, 0, -1);
    lv_obj_add_event_cb(minus, price_minus_cb, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)idx);

    /* 价格:显示在 [−] 与 [+] 之间 */
    char t[16];
    snprintf(t, sizeof(t), s_zh_ok ? "¥%d" : "%d", s_price[idx]);
    lv_obj_t *pl = mk_label(row, t, s_zh_ok ? s_f28 : &lv_font_montserrat_28,
                            lv_color_hex(A_CARAMEL));
    lv_obj_align(pl, LV_ALIGN_RIGHT_MID, -58, 0);
    s_price_lbls[idx] = pl;
}

/* ======================== 主入口 ======================== */

void ui_admin_create(void)
{
    zh_fonts_init();

    /* 默认价格 */
    int i;
    for (i = 0; i < ITEM_NUM; i++)
        s_price[i] = ADMIN_DEFAULT_PRICE[i];

    lv_obj_t *scr = lv_scr_act();

    /* 根容器 */
    lv_obj_t *root = lv_obj_create(scr);
    lv_obj_remove_style_all(root);
    lv_obj_set_pos(root, 0, 0);
    lv_obj_set_size(root, ADMIN_SCREEN_W, ADMIN_SCREEN_H);
    lv_obj_set_style_bg_color(root, lv_color_hex(A_BG1), 0);
    lv_obj_set_style_bg_grad_color(root, lv_color_hex(A_BG2), 0);
    lv_obj_set_style_bg_grad_dir(root, LV_GRAD_DIR_VER, 0);

    /* ============ 顶部 ============ */
    lv_obj_t *logo = lv_obj_create(root);
    lv_obj_remove_style_all(logo);
    lv_obj_set_pos(logo, 24, 14);
    lv_obj_set_size(logo, 62, 62);
    lv_obj_set_style_radius(logo, 18, 0);
    lv_obj_set_style_bg_color(logo, lv_color_hex(0xFFF7EA), 0);
    lv_obj_set_style_bg_grad_color(logo, lv_color_hex(0xF4DCB6), 0);
    lv_obj_set_style_bg_grad_dir(logo, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(logo, 3, 0);
    lv_obj_set_style_border_color(logo, lv_color_hex(A_CARAMEL), 0);
    lv_obj_t *ltx = mk_label(logo, s_zh_ok ? "管" : "A",
                             s_zh_ok ? s_f28 : &lv_font_montserrat_28,
                             lv_color_hex(0x7A3B12));
    lv_obj_align(ltx, LV_ALIGN_CENTER, 0, -1);

    lv_obj_t *title = mk_label(root, s_zh_ok ? "茶小满 · 后台管理" : "CHA XIAO MAN",
                               s_zh_ok ? s_f28 : &lv_font_montserrat_28,
                               lv_color_hex(A_COCOA));
    lv_obj_set_pos(title, 104, 8);

    lv_obj_t *sub = mk_label(root, "SERVER ADMIN  -  PRICE & ORDERS",
                             &lv_font_montserrat_14, lv_color_hex(A_CARAMEL));
    lv_obj_set_style_text_letter_space(sub, 1, 0);
    lv_obj_set_pos(sub, 106, 54);

    /* 右上角文件提示:不显示(按需求去掉) */

    /* ============ 左面板:价格设置 ============ */
    lv_obj_t *left = mk_panel(root, 24, 100, 470, 486);

    lv_obj_t *lt = mk_label(left, s_zh_ok ? "奶茶价格设置" : "PRICE SETTING",
                            s_zh_ok ? s_f24 : &lv_font_montserrat_24,
                            lv_color_hex(A_COCOA));
    lv_obj_set_pos(lt, 18, 16);

    lv_obj_t *ls = mk_label(left, s_zh_ok ? "修改后点「保存价格」写入 milktea_prices.txt"
                                          : "save -> milktea_prices.txt",
                            s_zh_ok ? s_f16 : &lv_font_montserrat_16,
                            lv_color_hex(A_LIGHT_BR));
    lv_obj_set_pos(ls, 18, 54);

    for (i = 0; i < ITEM_NUM; i++)
        create_price_row(left, i, 100 + i * 56);

    /* 恢复默认 + 保存 */
    lv_obj_t *bd = lv_obj_create(left);
    lv_obj_remove_style_all(bd);
    lv_obj_set_pos(bd, 18, 396);
    lv_obj_set_size(bd, 130, 46);
    lv_obj_set_style_radius(bd, 23, 0);
    lv_obj_set_style_bg_color(bd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(bd, 2, 0);
    lv_obj_set_style_border_color(bd, lv_color_hex(A_CARAMEL), 0);
    lv_obj_add_flag(bd, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(bd, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *bdtx = mk_label(bd, s_zh_ok ? "恢复默认" : "DEFAULT",
                              s_zh_ok ? s_f20 : &lv_font_montserrat_20,
                              lv_color_hex(A_CARAMEL));
    lv_obj_align(bdtx, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(bd, btn_default_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *bs = lv_obj_create(left);
    lv_obj_remove_style_all(bs);
    lv_obj_set_pos(bs, 470 - 18 - 170, 396);
    lv_obj_set_size(bs, 170, 46);
    lv_obj_set_style_radius(bs, 23, 0);
    lv_obj_set_style_bg_color(bs, lv_color_hex(A_SAVE_BG1), 0);
    lv_obj_set_style_bg_grad_color(bs, lv_color_hex(A_SAVE_BG2), 0);
    lv_obj_set_style_bg_grad_dir(bs, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(bs, 3, 0);
    lv_obj_set_style_border_color(bs, lv_color_hex(A_SAVE_EDGE), 0);
    lv_obj_set_style_border_opa(bs, LV_OPA_COVER, 0);
    lv_obj_set_style_translate_y(bs, 2, LV_STATE_PRESSED);
    lv_obj_add_flag(bs, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(bs, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *bstx = mk_label(bs, s_zh_ok ? "保存价格" : "SAVE",
                              s_zh_ok ? s_f20 : &lv_font_montserrat_20,
                              lv_color_hex(A_SAVE_TXT));
    lv_obj_align(bstx, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(bs, btn_save_cb, LV_EVENT_CLICKED, NULL);

    s_msg_lbl = mk_label(left, "", s_zh_ok ? s_f16 : &lv_font_montserrat_16,
                         lv_color_hex(A_GREEN_TX));
    lv_obj_set_pos(s_msg_lbl, 18, 452);
    set_status("正在读取价格文件…", "loading price file...");
    admin_load_prices();
    for (i = 0; i < ITEM_NUM; i++)
        price_row_refresh((uint8_t)i);

    /* ============ 右面板:点餐信息 ============ */
    lv_obj_t *right = mk_panel(root, 518, 100, 482, 486);

    lv_obj_t *rt = mk_label(right, s_zh_ok ? "点餐信息" : "ORDERS",
                            s_zh_ok ? s_f24 : &lv_font_montserrat_24,
                            lv_color_hex(A_COCOA));
    lv_obj_set_pos(rt, 18, 16);

    lv_obj_t *rs = mk_label(right,
                            s_zh_ok ? "order_list.txt · 每 2 秒自动检测新订单"
                                    : "order_list.txt · auto refresh 2s",
                            s_zh_ok ? s_f16 : &lv_font_montserrat_16,
                            lv_color_hex(A_LIGHT_BR));
    lv_obj_set_pos(rs, 18, 52);

    lv_obj_t *btn = lv_obj_create(right);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, 120, 40);
    lv_obj_set_style_radius(btn, 20, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(A_CARAMEL), 0);
    lv_obj_set_style_bg_grad_color(btn, lv_color_hex(A_CARAMEL2), 0);
    lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_translate_y(btn, 2, LV_STATE_PRESSED);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -16, 12);
    lv_obj_t *btx = mk_label(btn, s_zh_ok ? "立即刷新" : "REFRESH",
                             s_zh_ok ? s_f16 : &lv_font_montserrat_16,
                             lv_color_hex(0xFFFFFF));
    lv_obj_align(btx, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn, btn_refresh_cb, LV_EVENT_CLICKED, NULL);

    /* 可滚动订单区 */
    lv_obj_t *sc = lv_obj_create(right);
    lv_obj_remove_style_all(sc);
    lv_obj_set_pos(sc, 18, 92);
    lv_obj_set_size(sc, 482 - 36, 486 - 92 - 18);
    lv_obj_set_style_bg_color(sc, lv_color_hex(0xFFFBF2), 0);
    lv_obj_set_style_radius(sc, 16, 0);
    lv_obj_set_style_border_width(sc, 1, 0);
    lv_obj_set_style_border_color(sc, lv_color_hex(0xEFD9BA), 0);
    lv_obj_set_style_pad_all(sc, 12, 0);
    lv_obj_set_scroll_dir(sc, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(sc, LV_SCROLLBAR_MODE_AUTO);
    /* 明确可滚动:鼠标滚轮 / 按住拖动 都能看全部点单信息 */
    lv_obj_add_flag(sc, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(sc, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_add_flag(sc, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    s_ord_sc = sc;

    s_ord_lbl = lv_label_create(sc);
    lv_label_set_long_mode(s_ord_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ord_lbl, 482 - 36 - 26);
    lv_obj_set_style_text_font(s_ord_lbl,
                               s_zh_ok ? s_f16 : &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s_ord_lbl, lv_color_hex(0x55371F), 0);
    lv_obj_set_style_text_line_space(s_ord_lbl, 2, 0);
    lv_label_set_text(s_ord_lbl, "(loading order_list.txt ...)");
    orders_load(1);
    lv_obj_update_layout(sc);

    /* 每 2 秒检测新订单 */
    lv_timer_create(order_timer_cb, 2000, NULL);
}

/* ======================== 公共接口 ======================== */

int ui_admin_price_get(uint8_t idx)
{
    if (idx >= ITEM_NUM)
        return 0;
    return s_price[idx];
}

void ui_admin_price_set(uint8_t idx, int v)
{
    if (idx >= ITEM_NUM)
        return;
    if (v < 1)
        v = 1;
    if (v > 99)
        v = 99;
    s_price[idx] = v;
    price_row_refresh(idx);
}
