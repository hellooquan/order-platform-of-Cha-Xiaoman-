/**
 * ============================================================
 *  茶小满 · 服务端后台管理 UI(LVGL 9.2 / 1024x600 / SDL)
 * ------------------------------------------------------------
 *  配套文件: ui_admin.c / main_admin.c
 *  功能:
 *   1. 奶茶价格设置:5 款饮品逐项 [+]/[-] 调价,保存到
 *      /home/hyq/project/order_platrorm/server/milktea_prices.txt
 *   2. 查看点餐信息:读取
 *      /home/hyq/project/order_platrorm/server/order_list.txt,
 *      每 2 秒自动检测新内容 + 手动"立即刷新"按钮
 *
 *  该程序是独立于 server 进程的"后台窗口",不改动 server.c。
 * ============================================================
 */
#ifndef UI_ADMIN_H
#define UI_ADMIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* 饮品数量(与点餐机一致) */
#define UI_ADMIN_PRODUCT_NUM 5

/**
 * 创建后台管理界面(lv_init() 与显示器初始化后调用一次)
 */
void ui_admin_create(void);

/**
 * 读取第 idx 款饮品当前显示价格
 */
int ui_admin_price_get(uint8_t idx);

/**
 * 设置第 idx 款饮品价格并刷新界面显示(1..99)
 */
void ui_admin_price_set(uint8_t idx, int v);

#ifdef __cplusplus
}
#endif

#endif /* UI_ADMIN_H */
