#include <cstdio>
#include <pico/stdlib.h>
#include <cstdlib>
#include <hardware/i2c.h>
#include <lvgl.h>
#include <lv_port_disp.h>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <benchmark/lv_demo_benchmark.h>
#include "ui.h"
#include "InfoLabel.h"
#include "INA219.h"
#include "st7789.h"

#define INA219_ADDR_PORT1	0x40
#define INA219_ADDR_PORT2	0x41
#define INA219_ADDR_PORT3	0x44
#define INA219_ADDR_PORT4	0x45

#define DATA_REFRESH_INTER	(500)	//数据刷新间隔（ms)
/*
 *认为端口被关闭的门限电压，低于此电压则认为端口被关闭
 * 2.7V为CH217K手册中规定的欠压保护电压
 */
#define THRESHOLD_VOLTAGE	(2.7f)
#define MAX_CURRENT_MA		(1500)

#if 0
#define LB_PORT1_ACT_COLOR	"7A0DF3"
#define LB_PORT2_ACT_COLOR	"F30D8F"
#define LB_PORT3_ACT_COLOR	"0DF36C"
#define LB_PORT4_ACT_COLOR	"F7F8E1"
#define LB_ZERO_COLOR		"BBBBBB"
#else
#define LB_PORTx_ACT_COLOR	"e8e8e8"
#define LB_PORT1_ACT_COLOR	LB_PORTx_ACT_COLOR
#define LB_PORT2_ACT_COLOR	LB_PORTx_ACT_COLOR
#define LB_PORT3_ACT_COLOR	LB_PORTx_ACT_COLOR
#define LB_PORT4_ACT_COLOR	LB_PORTx_ACT_COLOR
#define LB_ZERO_COLOR		"BBBBBB"
#endif

INA219_t ina219_1_h;
INA219_t ina219_2_h;
INA219_t ina219_3_h;
INA219_t ina219_4_h;

lv_timer_t *refresh_timer;
lv_timer_t *backlight_on_timer;

info_label *info_lb1, *info_lb2, *info_lb3, *info_lb4;
std::vector<info_label*> arr_info_label;

bool alarm_1ms_callback(struct repeating_timer *);
static void refresh_data_cb(lv_timer_t * timer);
static void backlight_on_cb(lv_timer_t * timer);

int main() {
	set_sys_clock_khz(130000, true);

	stdio_init_all();
	repeating_timer timer_1ms = { 0 };
	add_repeating_timer_ms(-1, alarm_1ms_callback, nullptr, &timer_1ms);

	printf("CH335F UBS HUB Begin\n");

	lv_init();
	lv_port_disp_init();
	ui_init();

	// lv_demo_benchmark();
	/*
	 * TODO: 检测到所有口失效一分钟之后关闭屏幕显示，停止lvgl刷新，进入低功耗模式（？）
	 * TODO：每隔1s检测一次四口电压，有效时恢复显示
	 */

	INA219_Init(&ina219_1_h, INA219_I2C_HANDLE, INA219_ADDR_PORT1);
	INA219_Init(&ina219_2_h, INA219_I2C_HANDLE, INA219_ADDR_PORT2);
	INA219_Init(&ina219_3_h, INA219_I2C_HANDLE, INA219_ADDR_PORT3);
	INA219_Init(&ina219_4_h, INA219_I2C_HANDLE, INA219_ADDR_PORT4);

	info_lb1 = new info_label(&ina219_1_h, uic_pl_port1, uic_lb_port1, uic_pl_shade_1, LB_PORT1_ACT_COLOR, LB_ZERO_COLOR, THRESHOLD_VOLTAGE, MAX_CURRENT_MA);
	info_lb2 = new info_label(&ina219_2_h, uic_pl_port2, uic_lb_port2, uic_pl_shade_2, LB_PORT2_ACT_COLOR, LB_ZERO_COLOR, THRESHOLD_VOLTAGE, MAX_CURRENT_MA);
	info_lb3 = new info_label(&ina219_3_h, uic_pl_port3, uic_lb_port3, uic_pl_shade_3, LB_PORT3_ACT_COLOR, LB_ZERO_COLOR, THRESHOLD_VOLTAGE, MAX_CURRENT_MA);
	info_lb4 = new info_label(&ina219_4_h, uic_pl_port4, uic_lb_port4, uic_pl_shade_4, LB_PORT1_ACT_COLOR, LB_ZERO_COLOR, THRESHOLD_VOLTAGE, MAX_CURRENT_MA);
	arr_info_label.push_back(info_lb1);
	arr_info_label.push_back(info_lb2);
	arr_info_label.push_back(info_lb3);
	arr_info_label.push_back(info_lb4);

	//info_lb1->set_label_mask_pos(0.5);

	//数据刷新定时器
	refresh_timer = lv_timer_create(refresh_data_cb, DATA_REFRESH_INTER, nullptr);
	lv_timer_set_repeat_count(refresh_timer, -1);
	//延时260ms之后才打开背光，不展示初始化时的一些缓存
	backlight_on_timer = lv_timer_create(backlight_on_cb, 270, nullptr);
	lv_timer_set_repeat_count(backlight_on_timer, 1);

	while (true) {
		lv_task_handler();
	}
}

static void refresh_data_cb(lv_timer_t * timer) {
	float power_total = 0.0f, volt_v = info_lb1->voltage_v;

	//总线上的电压相差不大，电压label显示最后一个有效电压，如果全部失效，显示第一个电压
	for (const auto info_label: arr_info_label) {
		info_label->refresh_sensor_data();
		info_label->set_label_text(info_label->fmt_info_str());
		info_label->update_label_mask();
		power_total += info_label->power_mw;

		//当该接口的总线电压低于设定值时，隐藏其显示
		info_label->set_enable(info_label->check_voltage());
		if (info_label->check_voltage()) {
			volt_v = info_label->voltage_v;
		}
	}

	//格式化：00.000 V
	std::ostringstream oss_v;
	oss_v << std::fixed << std::setprecision(3)
			  << std::setfill('0') << std::setw(6)
			  << volt_v
			  << " V";
	lv_label_set_text(uic_lb_volt, oss_v.str().c_str());
	//格式化：00.000 W
	std::ostringstream oss_tot_power;
	oss_tot_power << std::fixed << std::setprecision(3)
			  << std::setfill('0') << std::setw(6)
			  << (power_total / 1000.0f)
			  << " W";
	lv_label_set_text(uic_lb_tot_power, oss_tot_power.str().c_str());
}

static void backlight_on_cb(lv_timer_t * timer) {
	ST7789_SetBacklight(1);
}

bool alarm_1ms_callback(struct repeating_timer * timer) {
	lv_tick_inc(1);
	return true;
}
