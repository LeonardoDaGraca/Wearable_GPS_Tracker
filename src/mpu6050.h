#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include "hardware/i2c.h"
#include "ff.h"

//I2C pins for Raspberry Pi Pico
#define I2C_PORT i2c0
#define SDA_PIN 0
#define SCL_PIN 1

//MPU6050 I2C address
#define MPU6050_ADDR 0x68

//MPU6050 registers
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H 0x43
#define MAX_IMU_READINGS 5

typedef struct {
    int ax, ay, az, gx, gy, gz;
} IMU_Reading;

extern IMU_Reading imu_buffer[MAX_IMU_READINGS];
extern int imu_buffer_index;

void mpu6050_init();
int16_t read_raw_data(uint8_t reg);
void calibrate_mpu6050();
void read_sensor_data_corrected(int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                                int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z);
IMU_Reading read_imu();
void process_imu_buffer();
void write_imu_buffer(FIL *file, uint64_t curr_time);

#endif