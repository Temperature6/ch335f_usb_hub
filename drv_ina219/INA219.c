/*
 * INA219.c
 *
 *  Created on: Dec 30, 2020
 *       Author: Piotr Smolen <komuch@gmail.com>
 */
#include "INA219.h"
#ifdef __cplusplus
extern "C" {
#endif

uint16_t ina219_calibrationValue;
int16_t ina219_currentDivider_mA;
int16_t ina219_powerMultiplier_mW;

uint16_t Read16(INA219_t *ina219, uint8_t Register)
{
	uint8_t Value[2] = { 0 };

	// HAL_I2C_Mem_Read(ina219->ina219_i2c, (INA219_ADDRESS<<1), Register, 1, Value, 2, 1000);
	i2c_write_blocking(INA219_I2C_HANDLE, ina219->Address, &Register, 1, true);
	i2c_read_blocking(INA219_I2C_HANDLE, ina219->Address, Value, 2, false);

	return ((Value[0] << 8) | Value[1]);
}


void Write16(INA219_t *ina219, uint8_t Register, uint16_t Value)
{
	uint8_t addr[2];
	addr[0] = (Value >> 8) & 0xff;  // upper byte
	addr[1] = (Value >> 0) & 0xff; // lower byte
	//HAL_I2C_Mem_Write(ina219->ina219_i2c, (INA219_ADDRESS<<1), Register, 1, (uint8_t*)addr, 2, 1000);
	i2c_write_blocking(INA219_I2C_HANDLE, ina219->Address, &Register, 1, true);
	i2c_write_blocking(INA219_I2C_HANDLE, ina219->Address, addr, 2, false);
}

uint16_t INA219_ReadBusVoltage(INA219_t *ina219)
{
	uint16_t result = Read16(ina219, INA219_REG_BUSVOLTAGE);

	return ((result >> 3  ) * 4);

}

int16_t INA219_ReadCurrent_raw(INA219_t *ina219)
{
	int16_t result = Read16(ina219, INA219_REG_CURRENT);

	return (result );
}

int16_t INA219_ReadCurrent(INA219_t *ina219)
{
	int16_t result = INA219_ReadCurrent_raw(ina219);

	return (result / ina219_currentDivider_mA );
}

uint16_t INA219_ReadShuntVolage(INA219_t *ina219)
{
	uint16_t result = Read16(ina219, INA219_REG_SHUNTVOLTAGE);
	//整套系统电流不应该超过2A
	result = result > 20000 ? 0 : result;
	return (result / 10);
}

void INA219_Reset(INA219_t *ina219)
{
	Write16(ina219, INA219_REG_CONFIG, INA219_CONFIG_RESET);
	sleep_ms(1);
}

void INA219_setCalibration(INA219_t *ina219, uint16_t CalibrationData)
{
	Write16(ina219, INA219_REG_CALIBRATION, CalibrationData);
}

uint16_t INA219_getConfig(INA219_t *ina219)
{
	uint16_t result = Read16(ina219, INA219_REG_CONFIG);
	return result;
}

void INA219_setConfig(INA219_t *ina219, uint16_t Config)
{
	Write16(ina219, INA219_REG_CONFIG, Config);
}

void INA219_setCalibration_32V_2A(INA219_t *ina219)
{
	// uint16_t config = INA219_CONFIG_BVOLTAGERANGE_32V |
	// 			 INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT |
	// 			 INA219_CONFIG_SADCRES_12BIT_1S_532US |
	// 			 INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
	uint16_t config = INA219_CONFIG_BVOLTAGERANGE_32V |
	             INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT_128S_69MS |
	             INA219_CONFIG_SADCRES_12BIT_128S_69MS |
	             INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;

	ina219_calibrationValue = 4096;
	ina219_currentDivider_mA = 10; // Current LSB = 100uA per bit (1000/100 = 10)
	ina219_powerMultiplier_mW = 2; // Power LSB = 1mW per bit (2/1)

	INA219_setCalibration(ina219, ina219_calibrationValue);
	INA219_setConfig(ina219, config);
}

void INA219_setCalibration_32V_1A(INA219_t *ina219)
{
	uint16_t config = INA219_CONFIG_BVOLTAGERANGE_32V |
	                    INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT |
	                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
	                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;

	ina219_calibrationValue = 10240;
	ina219_currentDivider_mA = 25;    // Current LSB = 40uA per bit (1000/40 = 25)
	ina219_powerMultiplier_mW = 0.8f; // Power LSB = 800uW per bit

	INA219_setCalibration(ina219, ina219_calibrationValue);
	INA219_setConfig(ina219, config);
}

void INA219_setCalibration_16V_400mA(INA219_t *ina219)
{
	uint16_t config = INA219_CONFIG_BVOLTAGERANGE_16V |
	                    INA219_CONFIG_GAIN_1_40MV | INA219_CONFIG_BADCRES_12BIT |
	                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
	                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;

	ina219_calibrationValue = 8192;
	ina219_currentDivider_mA = 20;    // Current LSB = 50uA per bit (1000/50 = 20)
	ina219_powerMultiplier_mW = 1.0f; // Power LSB = 1mW per bit

	INA219_setCalibration(ina219, ina219_calibrationValue);
	INA219_setConfig(ina219, config);
}

void INA219_setPowerMode(INA219_t *ina219, uint8_t Mode)
{
	uint16_t config = INA219_getConfig(ina219);

	switch (Mode) {
		case INA219_CONFIG_MODE_POWERDOWN:
			config = (config & ~INA219_CONFIG_MODE_MASK) | (INA219_CONFIG_MODE_POWERDOWN & INA219_CONFIG_MODE_MASK);
			INA219_setConfig(ina219, config);
			break;

		case INA219_CONFIG_MODE_SANDBVOLT_TRIGGERED:
			config = (config & ~INA219_CONFIG_MODE_MASK) | (INA219_CONFIG_MODE_SANDBVOLT_TRIGGERED & INA219_CONFIG_MODE_MASK);
			INA219_setConfig(ina219, config);
			break;

		case INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS:
			config = (config & ~INA219_CONFIG_MODE_MASK) | (INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS & INA219_CONFIG_MODE_MASK);
			INA219_setConfig(ina219, config);
			break;

		case INA219_CONFIG_MODE_ADCOFF:
			config = (config & ~INA219_CONFIG_MODE_MASK) | (INA219_CONFIG_MODE_ADCOFF & INA219_CONFIG_MODE_MASK);
			INA219_setConfig(ina219, config);
			break;
	}
}

uint8_t INA219_Init(INA219_t *ina219, i2c_inst_t *i2c, uint8_t Address)
{
	ina219->ina219_i2c = i2c;
	ina219->Address = Address;

	i2c_init(ina219->ina219_i2c, 100000);
	gpio_set_function(INA219_I2C_SDA, GPIO_FUNC_I2C);
	gpio_set_function(INA219_I2C_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(INA219_I2C_SDA);
	gpio_pull_up(INA219_I2C_SCL);

	ina219_currentDivider_mA = 0;
	ina219_powerMultiplier_mW = 0;

	//INA219_Reset(ina219);
	// INA219_setCalibration_32V_1A(ina219);
	//INA219_setCalibration_32V_2A(ina219);

	return 1;
}

void ina219_test(INA219_t *ina219, uint8_t index) {
	const uint16_t vbus = INA219_ReadBusVoltage(ina219);
	const uint16_t vshunt = INA219_ReadShuntVolage(ina219);

	printf("[%6.3f]INA219_%02d vbus:%.3f  curr:%3.3fmA  power:%.2fW\n",
		((float)to_ms_since_boot(get_absolute_time()) / 1000), index,
		vbus / 1000.0, (float)vshunt / 100 * 100, vbus / 1000.0 * (float)vshunt / 1000);
}

#ifdef __cplusplus
}
#endif
