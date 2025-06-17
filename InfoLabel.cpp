//
// Created by AQin on 2025/6/10.
//

#include "InfoLabel.h"
#include <utility>
#include <cmath>
#include <iomanip>
#include <string>
#include <sstream>
#include <algorithm>
#include "ui.h"

/**
 * @param ina219 用于测量总线电压电流的INA219句柄
 * @param panel lv panel控件
 * @param label 显示功率的label控件
 * @param label_mask 功率占比遮罩层
 * @param active_color 有效数字颜色
 * @param non_act_color 无效数字（前导0）颜色
 * @param thsh_volt 电压阈值，低于此电压认为无效
 * @param max_current 最大电流，用于计算功率占比
 */
info_label::info_label(INA219_t *ina219, lv_obj_t *panel, lv_obj_t *label, lv_obj_t *label_mask,
		std::string active_color, std::string non_act_color,
		const float thsh_volt, const float max_current):
	ina219(ina219), panel(panel), label(label), label_mask(label_mask),
	active_color(std::move(active_color)), non_act_color(std::move(non_act_color)),
	threshhold_voltage(thsh_volt), max_current(max_current)
{

}

/**
 * @brief 刷新传感器数据，读取数据之前必须调用此函数刷新传感器数据
 */
void info_label::refresh_sensor_data() {
	ina219_get_volt_cur_power(&voltage_v, &current_ma, &power_mw);
}

/**
 * @brief 读取传感器数据
 * @param volt_v 存放读出的电压(V)的指针
 * @param cur_mA 存放读出的电流(mA)的指针
 * @param power_mW 存放读出的功率(mW)的指针
 */
void info_label::ina219_get_volt_cur_power(float *volt_v, float *cur_mA, float *power_mW) const {
	const uint16_t vbus = INA219_ReadBusVoltage(ina219);
	const uint16_t vshunt = INA219_ReadShuntVolage(ina219);

	if (volt_v) *volt_v = static_cast<float>(vbus) / 1000.0f;
	if (cur_mA) *cur_mA = static_cast<float>(vshunt);
	if (power_mW) *power_mW = static_cast<float>(vbus) / 1000.0f * static_cast<float>(vshunt);
}

/**
 * @brief 设置panel是否使能，失能则消失
 * @param enabled 是否使能
 */
void info_label::set_enable(const bool enabled) {
	if (panel_enabled == enabled) {
		return;
	}
	panel_enabled = enabled;
	if (enabled) {
		lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN);
		// plenable_Animation(panel, 0);
	} else {
		lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
		// pldisable_Animation(panel, 0);
	}
}

/**
 * @brief 将电流和功率数据格式化成lvgl recolor风格化的字符串
 * @return 返回格式化后的字符串
 */
std::string info_label::fmt_info_str() const {
	/*
	电流mA最大四位数 功率mW最大四位数 格式
	0000mA 0015mW，前导0为灰色
	#A09896 00#  #F33C0D 00mA# 0015mW，前导0为灰色
	*/
	int dig_count = get_digit_count(static_cast<int>(current_ma));
	std::ostringstream oss_ma;
	oss_ma << std::setw(4) << std::setfill('0') << static_cast<int>(current_ma) << "mA#";	//先格式化出来不带颜色的
	std::string str_ma = oss_ma.str();
	str_ma.insert(4 - dig_count, "##" + active_color + " ");
	str_ma.insert(0, "#" + non_act_color + " ");

	dig_count = get_digit_count(static_cast<int>(power_mw));
	std::ostringstream oss_mw;
	oss_mw << std::setw(4) << std::setfill('0') << static_cast<int>(power_mw) << "mW#";	//先格式化出来不带颜色的
	std::string str_mw = oss_mw.str();
	str_mw.insert(4 - dig_count, "##" + active_color + " ");
	str_mw.insert(0, "#" + non_act_color + " ");

	return str_ma + " " + str_mw;
}

/**
 * @brief 计算一个十进制整数的位数
 * @param num 整数
 * @return 位数
 */
int info_label::get_digit_count(const int num) {
	if (num == 0) return 1; // 0 有 1 位数字
	const int abs_num = std::abs(num);
	return static_cast<int>(std::log10(static_cast<double>(abs_num))) + 1;
}

/**
 * @brief 设置label的字符串
 * @param str 要设置的字符串
 */
void info_label::set_label_text(const std::string& str) const {
	lv_label_set_text(label, str.c_str());
}

/**
 * @brief 检查当前电压是否为有效电压
 * @return false: 应当失能, true: 应当使能
 */
bool info_label::check_voltage() const {
	if (voltage_v < threshhold_voltage) {
		return false;
	}
	return true;
}

/**
 * @brief 设置label遮罩层在整个panel中的显示占比
 * @param pos_percent 百分比，范围0.0 ~ 1.0
 */
void info_label::set_label_mask_pos(const float pos_percent) const {
	lv_obj_set_x(label_mask, static_cast<short>(map(pos_percent, 0, 1, GRAY_MASK_BEG, GARY_MASK_END)));
}

void info_label::update_label_mask() const {
	set_label_mask_pos(current_ma / max_current);
}

float info_label::map(float val, const float old_min, const float old_max, const float new_min, const float new_max) {
	val = std::clamp(val, old_min, old_max);
	return (new_max - new_min) * (val - old_min) / (old_max - old_min) + new_min;
}




