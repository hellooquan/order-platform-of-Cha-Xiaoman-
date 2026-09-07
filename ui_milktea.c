/**
 * ============================================================
 *  茶小满 · 奶茶点餐 UI   (LVGL 9.2.x / 1024x600 / SDL)
 * ------------------------------------------------------------
 *  功能:
 *    - 温暖奶茶色系界面,顶部品牌栏 + 5 个招牌饮品卡片按钮
 *      (每杯饮品卡片上带"已点数量"角标)+ 底部汇总条
 *    - 5 个饮品按钮 + 底部"提交订单"按钮,均注册 LV_EVENT_CLICKED 回调,
 *      回调函数体 drink_click_cb() / submit_order_cb() 留给你自己填写
 *      (文件里各有一段默认示例,可整体替换)
 *
 *  中文字体:
 *    - 运行时用 TinyTTF(LV_USE_TINY_TTF,你 lv_conf.h 已开启)
 *      从 Noto CJK 字体文件加载,无需改 lv_conf、无需装开发库;
 *    - 若加载失败(比如换机器没有该字体),自动退回蒙提塞拉特字体,
 *      界面中文会留空,西文/数字仍可显示。
 *
 *  用法(在 lv_init() 和显示器初始化之后):
 *      ui_milktea_create();
 *  在你的回调里调用:
 *      ui_milktea_set_count(idx, n);  刷新角标 + 底部汇总
 *      ui_milktea_get_count(idx);
 *      ui_milktea_price_of(idx);      单价
 *      ui_milktea_order_cups();       总杯数(提交订单时用)
 *      ui_milktea_order_total();      合计金额(提交订单时用)
 * ============================================================
 */
#include "lvgl/lvgl.h"
#include "ui_milktea.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client/client.h"

#if LV_USE_TINY_TTF
#include "lvgl/src/libs/tiny_ttf/lv_tiny_ttf.h"
#endif

/* ======================== 可配置项 ======================== */

/* 屏幕尺寸(与模拟器保持一致) */
#define MLK_SCREEN_W 1024
#define MLK_SCREEN_H 600

/* Noto CJK 字体路径(运行环境内优先找第 1 个,找不到试第 2 个) */
#define UI_MLK_FONT_P1 "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
#define UI_MLK_FONT_P2 "/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc"

/* ======================== 配色(温暖奶茶色系) ======================== */
#define C_BG1 0xFDF3E3      /* 页面背景(奶油白,顶) */
#define C_BG2 0xF6E2C8      /* 页面背景渐变(下)    */
#define C_COCOA 0x5A3619    /* 深可可:主标题        */
#define C_BROWN 0x8C6A48    /* 正文棕               */
#define C_LIGHT_BR 0xB08A5F /* 浅棕:小字点缀        */
#define C_CARAMEL 0xB5763A  /* 焦糖主色             */
#define C_CARAMEL2 0xDEA45F /* 焦糖浅色(渐变)       */
#define C_CARD 0xFFFFFF     /* 卡片白               */
#define C_PINK_BG1 0xF6C3CB /* 粉色特惠条           */
#define C_PINK_BG2 0xF0AEB9
#define C_PINK_TX 0x9E4055
#define C_BADGE 0xE2604C    /* 数量角标红           */
#define C_GREEN_TX 0x3E7C4F /* “营业中”绿           */
#define C_SUBMIT 0xFFCE3D       /* 提交订单按钮:醒目金黄(上) */
#define C_SUBMIT2 0xF5911B      /* 提交订单按钮:渐变橙(下)   */
#define C_SUBMIT_TXT 0x3F2410   /* 按钮文字:深咖啡色(不是白) */
#define C_SUBMIT_EDGE 0xC2410C  /* 按钮描边:深橘红(框醒目)   */

/* 布局:5 张卡片横向排布 */
#define CARD_W 183
#define CARD_H 396
#define CARD_X0 26  /* 第一张卡片 x         */
#define CARD_GAP 14 /* 卡片间距             */
#define CARD_Y 112

/* 饮品按钮元数据(标签/价格你随时可以改) */
typedef struct
{
    const char *name; /* 中文名 */
    const char *note; /* 中文卖点 */
    const char *en;   /* 英文小字 */
    uint16_t price;   /* 单价(元) */
    uint32_t accent;  /* 每款主题色(圆环/价格/杯盖) */
} mlk_prod_t;

static const mlk_prod_t PRODS[UI_MLK_PRODUCT_NUM] = {
    {"招牌珍珠奶茶", "现煮珍珠 · 醇厚红茶", "Pearl Milk Tea", 15, 0xB5763A},
    {"芋泥啵啵鲜奶", "绵密芋泥 · 每日现蒸", "Taro Boba Milk Tea", 18, 0xA67AC4},
    {"杨枝甘露", "芒果西柚 · 椰乳满满", "Mango Pomelo Sago Tea", 20, 0xE8A33D},
    {"葡萄冻冻", "整颗葡萄 · 清爽茶冻", "Grape Jelly Tea", 16, 0x6FA3C9},
    {"四季春柠檬茶", "高山春茶 · 鲜切柠檬", "Four Seasons Lemon Tea", 12, 0xA9BE5B},
};

/* ======================== 全局状态 ======================== */

static lv_obj_t *s_badges[UI_MLK_PRODUCT_NUM];  /* 每杯的数量角标 label */
static int32_t s_counts[UI_MLK_PRODUCT_NUM];    /* 每杯已点数量         */
static lv_obj_t *s_summary_lbl = NULL;          /* 底部汇总条 label     */
static lv_obj_t *s_minus_lbls[UI_MLK_PRODUCT_NUM]; /* 每张卡“−”文字,0 杯时置灰 */

/* ---------- 中文字体(TinyTTF 运行时加载) ---------- */
static int s_zh_ok = 0;
static uint8_t *s_ttf_buf = NULL;
static size_t s_ttf_len = 0;
static const lv_font_t *s_f16 = NULL;
static const lv_font_t *s_f20 = NULL;
static const lv_font_t *s_f24 = NULL;
static const lv_font_t *s_f28 = NULL;
static const lv_font_t *s_f36 = NULL;

/* 加载 Noto CJK,成功则 s_zh_ok = 1 */
static void zh_fonts_init(void)
{
    const lv_font_t *f16 = &lv_font_montserrat_16;
    const lv_font_t *f20 = &lv_font_montserrat_20;
    const lv_font_t *f24 = &lv_font_montserrat_24;
    const lv_font_t *f28 = &lv_font_montserrat_28;
    const lv_font_t *f36 = &lv_font_montserrat_36;

#if LV_USE_TINY_TTF
    const char *paths[] = {UI_MLK_FONT_P1, UI_MLK_FONT_P2, NULL};
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

        /* 同一个数据缓冲创建多个字号,缓冲需一直存活(static) */
        lv_font_t *t16 = lv_tiny_ttf_create_data(s_ttf_buf, s_ttf_len, 16);
        lv_font_t *t20 = lv_tiny_ttf_create_data(s_ttf_buf, s_ttf_len, 20);
        lv_font_t *t24 = lv_tiny_ttf_create_data(s_ttf_buf, s_ttf_len, 24);
        lv_font_t *t28 = lv_tiny_ttf_create_data(s_ttf_buf, s_ttf_len, 28);
        lv_font_t *t36 = lv_tiny_ttf_create_data(s_ttf_buf, s_ttf_len, 36);

        if (t16 && t20 && t24 && t28 && t36)
        {
            f16 = t16;
            f20 = t20;
            f24 = t24;
            f28 = t28;
            f36 = t36;
            s_zh_ok = 1;
        }
        break;
    }
#endif

    s_f16 = f16;
    s_f20 = f20;
    s_f24 = f24;
    s_f28 = f28;
    s_f36 = f36;
}

/* ======================== 小工具 ======================== */

/* 建一个文本 label */
static lv_obj_t *mk_label(lv_obj_t *parent, const char *text,
                          const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    return l;
}

/* 画一杯小奶茶(纯 LVGL 形状拼成,无图片依赖)。
 * disc: 圆形底色容器(128x128),art 坐标按 128 尺寸排布 */
static void draw_mini_cup(lv_obj_t *disc, lv_color_t accent)
{
    /* 吸管:白色带描边,绕杯底微微倾斜,像真的插进杯里 */
    lv_obj_t *straw = lv_obj_create(disc);
    lv_obj_remove_style_all(straw);
    lv_obj_set_pos(straw, 60, 6);
    lv_obj_set_size(straw, 9, 44);
    lv_obj_set_style_bg_color(straw, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(straw, 2, 0);
    lv_obj_set_style_border_color(straw, accent, 0);
    lv_obj_set_style_border_opa(straw, LV_OPA_70, 0);
    lv_obj_set_style_radius(straw, 4, 0);
    lv_obj_set_style_transform_pivot_x(straw, 4, 0);
    lv_obj_set_style_transform_pivot_y(straw, 42, 0);
    lv_obj_set_style_transform_angle(straw, 140, 0); /* 约 14°,更自然 */

    /* 杯身:白色圆角杯 + 主题色描边(内部子对象裁成圆角) */
    lv_obj_t *cup = lv_obj_create(disc);
    lv_obj_remove_style_all(cup);
    lv_obj_set_pos(cup, 34, 46);
    lv_obj_set_size(cup, 60, 46);
    lv_obj_set_style_bg_color(cup, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(cup, 16, 0);
    lv_obj_set_style_border_width(cup, 3, 0);
    lv_obj_set_style_border_color(cup, accent, 0);
    lv_obj_set_style_border_opa(cup, LV_OPA_80, 0);
    lv_obj_set_style_shadow_width(cup, 10, 0);
    lv_obj_set_style_shadow_color(cup, accent, 0);
    lv_obj_set_style_shadow_opa(cup, LV_OPA_40, 0);
    lv_obj_set_style_shadow_ofs_y(cup, 3, 0);
    lv_obj_set_style_clip_corner(cup, true, 0);

    /* 奶茶液面:半透明主题色,透出"喝的是什么" */
    lv_obj_t *tea = lv_obj_create(cup);
    lv_obj_remove_style_all(tea);
    lv_obj_set_pos(tea, 0, 14);
    lv_obj_set_size(tea, 60, 32);
    lv_obj_set_style_bg_color(tea, accent, 0);
    lv_obj_set_style_bg_opa(tea, 140, 0); /* 约 55% 透明 */

    /* 杯底珍珠/果粒(深棕色小圆,更有奶茶感) */
    lv_color_t pearl = lv_color_hex(0x7A4E2E);
    lv_obj_t *p1 = lv_obj_create(cup);
    lv_obj_remove_style_all(p1);
    lv_obj_set_pos(p1, 12, 30);
    lv_obj_set_size(p1, 10, 10);
    lv_obj_set_style_bg_color(p1, pearl, 0);
    lv_obj_set_style_radius(p1, 999, 0);
    lv_obj_set_style_shadow_width(p1, 4, 0);
    lv_obj_set_style_shadow_color(p1, lv_color_hex(0x5C3A21), 0);
    lv_obj_set_style_shadow_opa(p1, LV_OPA_40, 0);

    lv_obj_t *p2 = lv_obj_create(cup);
    lv_obj_remove_style_all(p2);
    lv_obj_set_pos(p2, 30, 35);
    lv_obj_set_size(p2, 10, 10);
    lv_obj_set_style_bg_color(p2, pearl, 0);
    lv_obj_set_style_radius(p2, 999, 0);
    lv_obj_set_style_shadow_width(p2, 4, 0);
    lv_obj_set_style_shadow_color(p2, lv_color_hex(0x5C3A21), 0);
    lv_obj_set_style_shadow_opa(p2, LV_OPA_40, 0);

    lv_obj_t *p3 = lv_obj_create(cup);
    lv_obj_remove_style_all(p3);
    lv_obj_set_pos(p3, 44, 29);
    lv_obj_set_size(p3, 10, 10);
    lv_obj_set_style_bg_color(p3, pearl, 0);
    lv_obj_set_style_radius(p3, 999, 0);
    lv_obj_set_style_shadow_width(p3, 4, 0);
    lv_obj_set_style_shadow_color(p3, lv_color_hex(0x5C3A21), 0);
    lv_obj_set_style_shadow_opa(p3, LV_OPA_40, 0);

    /* 圆顶杯盖(主题色),压在杯口上 */
    lv_obj_t *lid = lv_obj_create(disc);
    lv_obj_remove_style_all(lid);
    lv_obj_set_pos(lid, 32, 38);
    lv_obj_set_size(lid, 64, 14);
    lv_obj_set_style_bg_color(lid, accent, 0);
    lv_obj_set_style_radius(lid, 7, 0);

    /* 盖顶吸管口:小圆洞 */
    lv_obj_t *hole = lv_obj_create(disc);
    lv_obj_remove_style_all(hole);
    lv_obj_set_pos(hole, 58, 42);
    lv_obj_set_size(hole, 8, 8);
    lv_obj_set_style_bg_color(hole, lv_color_hex(0xFFF3E0), 0);
    lv_obj_set_style_radius(hole, 999, 0);
}

/* ======================== 底部汇总 ======================== */

/* 计算当前订单:总杯数 + 合计金额 */
static void calc_order(int32_t *cups_out, uint32_t *total_out)
{
    int32_t cups = 0;
    uint32_t total = 0;
    int i;
    for (i = 0; i < UI_MLK_PRODUCT_NUM; i++)
    {
        cups += s_counts[i];
        total += (uint32_t)(s_counts[i] * PRODS[i].price);
    }
    *cups_out = cups;
    *total_out = total;
}

static void update_summary(void)
{
    if (!s_summary_lbl)
        return;
    int32_t cups = 0;
    uint32_t total = 0;
    calc_order(&cups, &total);
    char tmp[64];
    if (s_zh_ok)
        snprintf(tmp, sizeof(tmp), "已选 %d 杯   ·   合计 ¥%u",
                 (int)cups, (unsigned)total);
    else
        snprintf(tmp, sizeof(tmp), "CUPS %d   |   TOTAL %u",
                 (int)cups, (unsigned)total);
    lv_label_set_text(s_summary_lbl, tmp);
}

/* ======================== 点击回调(给你填!) ======================== */

static void drink_click_cb(lv_event_t *e)
{
    /* 触发事件的按钮索引 0..4 */
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    (void)idx; /* 若你暂时用不到就留着这行 */
    (void)e;

    /* ================================================================
     * >>> 你的点单回调逻辑写在这里 <<<
     * 可用接口:
     *   ui_milktea_get_count(idx)         当前已选杯数
     *   ui_milktea_set_count(idx, n)      设置杯数并刷新角标/汇总
     *   ui_milktea_price_of(idx)          该饮品单价(元)
     *   ui_milktea_name_of(idx)           该饮品名字(中文)
     *
     * 下面是一段【默认示例】:点击一次 +1 杯。
     * 你可以整体替换成自己的逻辑,比如发消息给 server.c、跳转页面等。
     * ================================================================ */
    {
        int32_t cnt = ui_milktea_get_count(idx);
        ui_milktea_set_count(idx, cnt + 1);
    }
}

/* ======================== 减杯按钮回调 ======================== */

static void minus_click_cb(lv_event_t *e)
{
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    int32_t n = ui_milktea_get_count(idx);
    if (n > 0)
    {
        ui_milktea_set_count(idx, n - 1); /* 减 1 杯,最少 0 */
    }
}

/* ======================== 提交订单回调(给你填!) ======================== */

static void submit_order_cb(lv_event_t *e)
{
    (void)e;
    /* ================================================================
     * >>> 你的"提交点单"逻辑写在这里 <<<
     * 现成数据:
     *   ui_milktea_order_cups()        总共几杯
     *   ui_milktea_order_total()       合计金额(元)
     *   ui_milktea_get_count(i)        第 i 杯几杯
     *   ui_milktea_name_of(i)          第 i 杯名字
     *   ui_milktea_price_of(i)         第 i 杯单价
     *
     * 下面是一段【默认示例】:没点东西直接返回;有点东西就打印一行
     * 订单日志再清空购物车。可整体替换成你的逻辑,
     * 比如把订单(名字 x 数量)发给你的 server.c。
     * ================================================================ */
    {
        int32_t cups = ui_milktea_order_cups();
        if (cups <= 0)
            return; /* 购物车是空的,不提交 */

        LV_LOG_USER("submit order: %d cup(s), total %u yuan",
                    (int)cups,
                    (unsigned)ui_milktea_order_total());

        /* TODO: 在这里把你的订单发出去(如 socket 发送到 server.c) */
        {
            for (int i = 0; i < UI_MLK_PRODUCT_NUM; i++)
            {
                set_milktea_idx_count(i, ui_milktea_get_count(i));
            }
            printf("[line:%d]:%s\n", __LINE__, __FUNCTION__);
            total_price(ui_milktea_order_total());
            client_send();
        }

        int i; /* 示例:提交成功后清空购物车 */
        for (i = 0; i < UI_MLK_PRODUCT_NUM; i++)
        {
            ui_milktea_set_count((uint8_t)i, 0);
        }
    }
}

/* ======================== 单张饮品卡片 ======================== */

static void create_drink_card(lv_obj_t *root, int idx)
{
    const mlk_prod_t *p = &PRODS[idx];
    lv_color_t accent = lv_color_hex(p->accent);

    /* ---- 卡片(整体可点,即一个“按钮”) ---- */
    lv_obj_t *card = lv_obj_create(root);
    lv_obj_remove_style_all(card);
    lv_obj_set_pos(card, CARD_X0 + idx * (CARD_W + CARD_GAP), CARD_Y);
    lv_obj_set_size(card, CARD_W, CARD_H);

    lv_obj_set_style_bg_color(card, lv_color_hex(C_CARD), 0);
    lv_obj_set_style_radius(card, 26, 0);
    lv_obj_set_style_shadow_width(card, 22, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x6B4626), 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_30, 0);
    lv_obj_set_style_shadow_ofs_y(card, 4, 0);

    /* 按下反馈:卡片轻微下移 + 描主题色边 */
    lv_obj_set_style_translate_y(card, 3, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(card, 2, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(card, accent, LV_STATE_PRESSED);

    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* 内部纵向居中对齐 */
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    /* ---- 顶部圆形“杯贴”(主题色) ---- */
    lv_obj_t *disc = lv_obj_create(card);
    lv_obj_remove_style_all(disc);
    lv_obj_set_size(disc, 128, 128);
    lv_obj_set_style_radius(disc, 999, 0);
    lv_obj_set_style_bg_color(disc, accent, 0);
    lv_obj_set_style_bg_opa(disc, LV_OPA_40, 0); /* 浅色底 */
    lv_obj_set_style_border_width(disc, 3, 0);
    lv_obj_set_style_border_color(disc, accent, 0);
    lv_obj_set_style_border_opa(disc, LV_OPA_70, 0);
    lv_obj_set_style_margin_top(disc, 14, 0);
    draw_mini_cup(disc, accent);

    /* ---- 中文名(无中文环境时自动换成英文名) ---- */
    lv_obj_t *name;
    if (s_zh_ok)
    {
        name = mk_label(card, p->name, s_f20, lv_color_hex(C_COCOA));
    }
    else
    {
        name = mk_label(card, p->en, &lv_font_montserrat_16,
                        lv_color_hex(C_COCOA));
    }
    lv_obj_set_style_margin_top(name, 12, 0);

    /* ---- 英文小字(仅中文环境显示,作装饰) ---- */
    if (s_zh_ok)
    {
        lv_obj_t *en = mk_label(card, p->en, &lv_font_montserrat_12,
                                lv_color_hex(C_LIGHT_BR));
        lv_obj_set_style_text_letter_space(en, 1, 0);
        lv_obj_set_style_margin_top(en, 4, 0);
    }

    /* ---- 一句话卖点 ---- */
    if (s_zh_ok)
    {
        lv_obj_t *note = mk_label(card, p->note, s_f16,
                                  lv_color_hex(C_BROWN));
        lv_obj_set_style_margin_top(note, 8, 0);
    }

    /* ---- 价格 ---- */
    {
        char pr[16];
        const lv_font_t *pf = s_zh_ok ? s_f28 : &lv_font_montserrat_28;
        snprintf(pr, sizeof(pr), s_zh_ok ? "¥%u" : "%u",
                 (unsigned)p->price);
        lv_obj_t *price = mk_label(card, pr, pf, accent);
        lv_obj_set_style_margin_top(price, 12, 0);
    }

    /* ---- 右上角“已点数量”角标(挂在根上避免被卡片布局影响) ---- */
    lv_obj_t *badge = lv_obj_create(root);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, 36, 36);
    lv_obj_set_style_radius(badge, 999, 0);
    lv_obj_set_style_bg_color(badge, lv_color_hex(C_BADGE), 0);
    lv_obj_set_style_shadow_width(badge, 8, 0);
    lv_obj_set_style_shadow_color(badge, lv_color_hex(0xE2604C), 0);
    lv_obj_set_style_shadow_opa(badge, LV_OPA_40, 0);
    lv_obj_align_to(badge, card, LV_ALIGN_TOP_RIGHT, -8, 12);

    lv_obj_t *badge_lbl = mk_label(badge, "0", &lv_font_montserrat_20,
                                   lv_color_hex(0xFFFFFF));
    lv_obj_align(badge_lbl, LV_ALIGN_CENTER, 0, 1);
    s_badges[idx] = badge_lbl;

    /* ---- 底部“−”减杯按钮(点错了可以减回去,0 杯时自动置灰) ---- */
    lv_obj_t *minus = lv_obj_create(card);
    lv_obj_remove_style_all(minus);
    lv_obj_set_size(minus, 42, 42);
    lv_obj_set_style_radius(minus, 999, 0);
    lv_obj_set_style_bg_color(minus, lv_color_hex(0xFFF1DE), 0);
    lv_obj_set_style_border_width(minus, 2, 0);
    lv_obj_set_style_border_color(minus, accent, 0);
    lv_obj_set_style_border_opa(minus, LV_OPA_60, 0);
    lv_obj_set_style_shadow_width(minus, 6, 0);
    lv_obj_set_style_shadow_color(minus, lv_color_hex(0x6B4626), 0);
    lv_obj_set_style_shadow_opa(minus, 64, 0); /* 约 25% */
    lv_obj_set_style_translate_y(minus, 2, LV_STATE_PRESSED);
    lv_obj_set_style_margin_top(minus, 10, 0);
    lv_obj_add_flag(minus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(minus, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *minus_lbl = mk_label(minus, "-", &lv_font_montserrat_28,
                                   accent);
    lv_obj_align(minus_lbl, LV_ALIGN_CENTER, 0, -1);
    s_minus_lbls[idx] = minus_lbl;

    lv_obj_add_event_cb(minus, minus_click_cb, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)idx);

    /* 注册点击回调,user_data 携带按钮索引 */
    lv_obj_add_event_cb(card, drink_click_cb, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)idx);
}

/* ======================== 主入口 ======================== */

void ui_milktea_create(void)
{
    zh_fonts_init();

    lv_obj_t *scr = lv_scr_act();

    /* 页面根容器:奶油渐变底 */
    lv_obj_t *root = lv_obj_create(scr);
    lv_obj_remove_style_all(root);
    lv_obj_set_pos(root, 0, 0);
    lv_obj_set_size(root, MLK_SCREEN_W, MLK_SCREEN_H);
    lv_obj_set_style_bg_color(root, lv_color_hex(C_BG1), 0);
    lv_obj_set_style_bg_grad_color(root, lv_color_hex(C_BG2), 0);
    lv_obj_set_style_bg_grad_dir(root, LV_GRAD_DIR_VER, 0);

    /* 装饰性柔光大圆(制造层次,可删) */
    lv_obj_t *dec1 = lv_obj_create(root);
    lv_obj_remove_style_all(dec1);
    lv_obj_set_size(dec1, 340, 340);
    lv_obj_set_style_radius(dec1, 999, 0);
    lv_obj_set_style_bg_color(dec1, lv_color_hex(0xE7A766), 0);
    lv_obj_set_style_bg_opa(dec1, 16, 0);
    lv_obj_align(dec1, LV_ALIGN_TOP_LEFT, -90, -150);

    lv_obj_t *dec2 = lv_obj_create(root);
    lv_obj_remove_style_all(dec2);
    lv_obj_set_size(dec2, 320, 320);
    lv_obj_set_style_radius(dec2, 999, 0);
    lv_obj_set_style_bg_color(dec2, lv_color_hex(0xE89AA6), 0);
    lv_obj_set_style_bg_opa(dec2, 12, 0);
    lv_obj_align(dec2, LV_ALIGN_BOTTOM_RIGHT, 40, 130);

    /* ============ 顶部品牌栏 ============ */
    /* logo 小方块:奶油底 + 焦糖描边 + 深可可“茶”字(更清晰好看) */
    lv_obj_t *logo = lv_obj_create(root);
    lv_obj_remove_style_all(logo);
    lv_obj_set_pos(logo, 26, 14);
    lv_obj_set_size(logo, 66, 66);
    lv_obj_set_style_radius(logo, 20, 0);
    lv_obj_set_style_bg_color(logo, lv_color_hex(0xFFF7EA), 0);
    lv_obj_set_style_bg_grad_color(logo, lv_color_hex(0xF4DCB6), 0);
    lv_obj_set_style_bg_grad_dir(logo, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(logo, 3, 0);
    lv_obj_set_style_border_color(logo, lv_color_hex(C_CARAMEL), 0);
    lv_obj_set_style_border_opa(logo, LV_OPA_90, 0);
    lv_obj_set_style_shadow_width(logo, 12, 0);
    lv_obj_set_style_shadow_color(logo, lv_color_hex(0xB5763A), 0);
    lv_obj_set_style_shadow_opa(logo, 90, 0); /* 约 35% */

    lv_obj_t *logotx = mk_label(logo, s_zh_ok ? "茶" : "T",
                                s_zh_ok ? s_f36 : &lv_font_montserrat_36,
                                lv_color_hex(0x7A3B12));
    lv_obj_align(logotx, LV_ALIGN_CENTER, 0, -2);

    /* 品牌名 */
    lv_obj_t *brand = mk_label(root, s_zh_ok ? "茶小满" : "CHA XIAO MAN",
                               s_zh_ok ? s_f36 : &lv_font_montserrat_36,
                               lv_color_hex(C_COCOA));
    lv_obj_set_pos(brand, 110, 6);

    lv_obj_t *branden = mk_label(root, "CHA XIAO MAN  -  MILK TEA",
                                 &lv_font_montserrat_16,
                                 lv_color_hex(C_CARAMEL));
    lv_obj_set_style_text_letter_space(branden, 2, 0);
    lv_obj_set_pos(branden, 112, 62);

    /* 右侧:粉色特惠条(最右) */
    lv_obj_t *promo = lv_obj_create(root);
    lv_obj_remove_style_all(promo);
    lv_obj_set_size(promo, 248, 40);
    lv_obj_set_style_radius(promo, 20, 0);
    lv_obj_set_style_bg_color(promo, lv_color_hex(C_PINK_BG1), 0);
    lv_obj_set_style_bg_grad_color(promo, lv_color_hex(C_PINK_BG2), 0);
    lv_obj_set_style_bg_grad_dir(promo, LV_GRAD_DIR_HOR, 0);
    lv_obj_align(promo, LV_ALIGN_TOP_RIGHT, -22, 24);
    lv_obj_t *promotx = mk_label(promo,
                                 s_zh_ok ? "第二杯半价 · 今日特惠"
                                         : "2ND CUP HALF PRICE",
                                 s_zh_ok ? s_f16 : &lv_font_montserrat_16,
                                 lv_color_hex(C_PINK_TX));
    lv_obj_align(promotx, LV_ALIGN_CENTER, 0, 0);

    /* ============ 5 个饮品按钮卡片 ============ */
    int i;
    for (i = 0; i < UI_MLK_PRODUCT_NUM; i++)
    {
        s_counts[i] = 0;
        create_drink_card(root, i);
    }

    /* ============ 底部汇总条 ============ */
    lv_obj_t *footer = lv_obj_create(root);
    lv_obj_remove_style_all(footer);
    lv_obj_set_pos(footer, 0, 520);
    lv_obj_set_size(footer, MLK_SCREEN_W, 80);
    lv_obj_set_style_radius(footer, 26, 0);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_shadow_width(footer, 26, 0);
    lv_obj_set_style_shadow_color(footer, lv_color_hex(0x8A5A2B), 0);
    lv_obj_set_style_shadow_opa(footer, LV_OPA_30, 0);
    lv_obj_set_style_shadow_ofs_y(footer, -6, 0);

    /* 左侧:小字 */
    lv_obj_t *fl = mk_label(footer, s_zh_ok ? "茶小满 · 点餐台" : "CHA XIAO MAN",
                            s_zh_ok ? s_f16 : &lv_font_montserrat_16,
                            lv_color_hex(C_LIGHT_BR));
    lv_obj_align(fl, LV_ALIGN_LEFT_MID, 30, 0);

    /* 右侧:提交订单按钮(金黄渐变 + 深橘红描边 + 深咖啡字,醒目) */
    lv_obj_t *submit = lv_obj_create(footer);
    lv_obj_remove_style_all(submit);
    lv_obj_set_size(submit, 216, 58);
    lv_obj_set_style_radius(submit, 29, 0);
    lv_obj_set_style_bg_color(submit, lv_color_hex(C_SUBMIT), 0);
    lv_obj_set_style_bg_grad_color(submit, lv_color_hex(C_SUBMIT2), 0);
    lv_obj_set_style_bg_grad_dir(submit, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(submit, 3, 0);
    lv_obj_set_style_border_color(submit, lv_color_hex(C_SUBMIT_EDGE), 0);
    lv_obj_set_style_border_opa(submit, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(submit, 16, 0);
    lv_obj_set_style_shadow_color(submit, lv_color_hex(0x7A3B00), 0);
    lv_obj_set_style_shadow_opa(submit, 140, 0); /* 约 55% */
    lv_obj_set_style_translate_y(submit, 2, LV_STATE_PRESSED);
    lv_obj_add_flag(submit, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(submit, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(submit, LV_ALIGN_RIGHT_MID, -26, 0);

    lv_obj_t *subtx = mk_label(submit, s_zh_ok ? "提交订单" : "SUBMIT",
                               s_zh_ok ? s_f28 : &lv_font_montserrat_28,
                               lv_color_hex(C_SUBMIT_TXT));
    lv_obj_align(subtx, LV_ALIGN_CENTER, 0, 0);

    lv_obj_add_event_cb(submit, submit_order_cb, LV_EVENT_CLICKED, NULL);

    /* 中间:汇总(角标变化时自动刷新) */
    s_summary_lbl = mk_label(footer, "", s_zh_ok ? s_f24 : &lv_font_montserrat_24,
                             lv_color_hex(C_COCOA));
    lv_obj_align(s_summary_lbl, LV_ALIGN_CENTER, 0, 0);

    update_summary();

    /* 把角标初始化为 0 */
    for (i = 0; i < UI_MLK_PRODUCT_NUM; i++)
    {
        ui_milktea_set_count((uint8_t)i, 0);
    }
}

/* ======================== 公共接口 ======================== */

void ui_milktea_set_count(uint8_t idx, int32_t n)
{
    if (idx >= UI_MLK_PRODUCT_NUM)
        return;
    if (n < 0)
        n = 0;
    s_counts[idx] = n;
    if (s_badges[idx])
    {
        char t[8];
        snprintf(t, sizeof(t), "%d", (int)n);
        lv_label_set_text(s_badges[idx], t);
    }
    /* 0 杯时把“−”置灰,一眼看出不能再减 */
    if (s_minus_lbls[idx])
    {
        lv_obj_set_style_text_color(s_minus_lbls[idx],
                                    (n > 0) ? lv_color_hex(PRODS[idx].accent)
                                            : lv_color_hex(0xC9B49C),
                                    0);
    }
    update_summary();
}

int32_t ui_milktea_get_count(uint8_t idx)
{
    if (idx >= UI_MLK_PRODUCT_NUM)
        return 0;
    return s_counts[idx];
}

uint16_t ui_milktea_price_of(uint8_t idx)
{
    if (idx >= UI_MLK_PRODUCT_NUM)
        return 0;
    return PRODS[idx].price;
}

const char *ui_milktea_name_of(uint8_t idx)
{
    if (idx >= UI_MLK_PRODUCT_NUM)
        return "";
    return PRODS[idx].en;
}

int32_t ui_milktea_order_cups(void)
{
    int32_t cups;
    uint32_t total;
    calc_order(&cups, &total);
    return cups;
}

uint32_t ui_milktea_order_total(void)
{
    int32_t cups;
    uint32_t total;
    calc_order(&cups, &total);
    return total;
}
