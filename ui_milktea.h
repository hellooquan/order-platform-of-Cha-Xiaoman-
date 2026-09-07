/**
 * ============================================================
 *  茶小满 · 奶茶点餐 UI (LVGL 9.2) — 公共接口
 * ============================================================
 *  配套文件: ui_milktea.c
 *  只做界面与 5 个饮品按钮 + 1 个"提交订单"按钮的点击回调空壳,
 *  回调内容由你自己填写:
 *   - 饮品按钮 -> ui_milktea.c 的 drink_click_cb()
 *   - 提交订单 -> ui_milktea.c 的 submit_order_cb()
 *
 *  屏幕尺寸按 1024 x 600 (SDL 模拟器)设计。
 * ============================================================
 */
#ifndef UI_MILKTEA_H
#define UI_MILKTEA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 饮品数量(5 个按钮) */
#define UI_MLK_PRODUCT_NUM  5

/* 按钮(饮品)索引,方便你在回调里 switch 使用 */
typedef enum {
    UI_MLK_IDX_PEARL_MILK_TEA = 0,  /* 招牌珍珠奶茶 ¥15 */
    UI_MLK_IDX_TARO_MILK       = 1,  /* 芋泥啵啵鲜奶   ¥18 */
    UI_MLK_IDX_MANGO_SAGO      = 2,  /* 杨枝甘露       ¥20 */
    UI_MLK_IDX_GRAPE_JELLY     = 3,  /* 葡萄冻冻       ¥16 */
    UI_MLK_IDX_LEMON_TEA       = 4,  /* 四季春柠檬茶   ¥12 */
} ui_mlk_drink_t;

/**
 * 创建整个点餐界面(请在 lv_init() 与显示器初始化之后调用一次)
 */
void ui_milktea_create(void);

/**
 * 设置第 idx 杯饮品的已点数量(会同步刷新角标与底部汇总条)
 * @param idx 0..UI_MLK_PRODUCT_NUM-1
 * @param n   新的数量,小于等于 0 视为 0
 */
void ui_milktea_set_count(uint8_t idx, int32_t n);

/**
 * 读取第 idx 杯饮品的当前数量
 */
int32_t ui_milktea_get_count(uint8_t idx);

/**
 * 读取第 idx 杯饮品的单价(元)
 */
uint16_t ui_milktea_price_of(uint8_t idx);

/**
 * 读取第 idx 杯饮品的名字(中文,仅供你显示/发送用)
 */
const char * ui_milktea_name_of(uint8_t idx);

/**
 * 当前订单总共几杯(所有饮品数量之和)
 */
int32_t ui_milktea_order_cups(void);

/**
 * 当前订单合计金额(元)
 */
uint32_t ui_milktea_order_total(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_MILKTEA_H */
