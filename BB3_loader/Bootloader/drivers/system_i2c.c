
#include "system_i2c.h"

#define I2C_TIMEOUT 100

bool system_i2c_test_device(uint8_t addr)
{
    return HAL_I2C_IsDeviceReady(sys_i2c, addr, 1, I2C_TIMEOUT) == HAL_OK;
}

void system_i2c_cmd8(uint8_t adr, uint8_t cmd)
{
	HAL_I2C_Master_Transmit(sys_i2c, adr, &cmd, 1, I2C_TIMEOUT);
}

uint8_t system_i2c_read8(uint8_t adr, uint8_t reg)
{
	uint8_t tmp;

	HAL_I2C_Mem_Read(sys_i2c, adr, reg, I2C_MEMADD_SIZE_8BIT, &tmp, 1, I2C_TIMEOUT);

	return tmp;
}

void system_i2c_write8(uint8_t adr, uint8_t reg, uint8_t val)
{
    HAL_I2C_Mem_Write(sys_i2c, adr, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, I2C_TIMEOUT);
}

uint16_t system_i2c_read16(uint8_t adr, uint8_t reg)
{
	uint16_t tmp;

	HAL_I2C_Mem_Read(sys_i2c, adr, reg, I2C_MEMADD_SIZE_8BIT, (uint8_t *)&tmp, 2, I2C_TIMEOUT);

	return tmp;
}

void system_i2c_write16(uint8_t adr, uint8_t reg, uint16_t val)
{
    HAL_I2C_Mem_Write(sys_i2c, adr, reg, I2C_MEMADD_SIZE_8BIT, (uint8_t *)&val, 2, I2C_TIMEOUT);
}

uint32_t system_i2c_read24(uint8_t adr, uint8_t reg)
{
	uint32_t tmp;

	HAL_I2C_Mem_Read(sys_i2c, adr, reg, I2C_MEMADD_SIZE_8BIT, (uint8_t *)&tmp, 3, I2C_TIMEOUT);

	return tmp;
}
