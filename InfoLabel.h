//
// Created by AQin on 2025/6/10.
//

#ifndef INFOLABEL_H
#define INFOLABEL_H
#include <string>
#include "lvgl.h"
#include "INA219.h"

#define GRAY_MASK_LEFT	(-168)
#define GRAY_MASK_RIGHT	(168)
#define GRAY_MASK_MID	(0)

#define GRAY_MASK_BEG	GRAY_MASK_LEFT
#define GARY_MASK_END	GRAY_MASK_MID

class info_label
{
	float max_current{};
    float mask_pos_old{};
    lv_anim_t anim{};       // 动画对象
    const uint32_t duration = 490;
    const uint32_t panel_pos_threshold = 150;

	void ina219_get_volt_cur_power(float *volt_v, float *cur_mA, float *power_mW) const;
	static int get_digit_count(int num);
	static float map(float val, float old_min, float old_max, float new_min, float new_max);
    bool panel_is_enabled() const;
public:
	float voltage_v{};
	float current_ma{};
	float power_mw{};
	INA219_t *ina219;
	lv_obj_t *panel;
	lv_obj_t *label;
	lv_obj_t *label_mask;
	std::string active_color;
	std::string non_act_color;
	float threshold_voltage{};
	info_label(INA219_t *ina219, lv_obj_t *panel, lv_obj_t *label, lv_obj_t *power_label_mask,
		std::string active_color, std::string non_act_color,
		float thsh_volt, float max_current);
	~info_label();

	void set_enable(bool enabled);
	[[nodiscard]] std::string fmt_info_str() const;
	void refresh_sensor_data();
	void set_label_text(const std::string& str) const;
    void set_label_mask_pos(float pos_percent, float pos_before) ;
	void update_label_mask();
	[[nodiscard]] bool check_voltage() const;
};


#endif //INFOLABEL_H
