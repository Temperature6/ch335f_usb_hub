//
// Created by AQin on 2025/6/10.
//

#ifndef INFOLABEL_H
#define INFOLABEL_H
#include <string>
#include "lvgl.h"
#include "INA219.h"

class info_label
{
	void ina219_get_volt_cur_power(float *volt_v, float *cur_mA, float *power_mW) const;
	static int get_digit_count(int num);
public:
	float voltage_v{};
	float current_ma{};
	float power_mw{};
	INA219_t *ina219;
	lv_obj_t *panel;
	lv_obj_t *label;
	std::string active_color;
	std::string non_act_color;
	float threshhold_voltage{};
	bool panel_enabled = true;
	info_label(INA219_t *ina219, lv_obj_t *panel, lv_obj_t *label, std::string active_color, std::string non_act_color, float thsh_volt);
	~info_label();

	void set_enable(bool enabled);
	[[nodiscard]] std::string fmt_info_str() const;
	void refresh_sensor_data();
	void set_label_text(const std::string& str) const;
	[[nodiscard]] bool check_voltage() const;
};


#endif //INFOLABEL_H
