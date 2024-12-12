/*
File: mpu6050_i2c.c
Author: Leonardo DaGraca

Implementation of IMU functions 
*/
#include <stdio.h>
#include <string.h>
#include "mpu6050.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

IMU_Reading imu_buffer[MAX_IMU_READINGS];
int imu_buffer_index = 0;

void mpu6050_init() {
    uint8_t buf[2];

    //Wake up the MPU6050
    buf[0] = MPU6050_REG_PWR_MGMT_1;
    buf[1] = 0x00;
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);

    //Set accelerometer sensitivity to +/- 2g (default)
    buf[0] = 0x1C;
    buf[1] = 0x00;
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);

    //Set gyroscope sensitivity to +/- 250°/s (default)
    buf[0] = 0x1B;
    buf[1] = 0x00;
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);
}

//function to read raw data from a register
int16_t read_raw_data(uint8_t reg) {
    uint8_t buf[2];

    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);

    //Combine high and low bytes
    return (int16_t)(buf[0] << 8 | buf[1]);
}

//offsets
int16_t accel_x_offset = 0, accel_y_offset = 0, accel_z_offset = 0;
int16_t gyro_x_offset = 0, gyro_y_offset = 0, gyro_z_offset = 0;

//calibrating IMU offsets using 2000 samples
void calibrate_mpu6050() {
    int32_t accel_x_sum = 0, accel_y_sum = 0, accel_z_sum = 0;
    int32_t gyro_x_sum = 0, gyro_y_sum = 0, gyro_z_sum = 0;
    int num_samples = 2000;

    printf("Calibrating MPU6050. Please keep the device stationary...\n");

    for (int i = 0; i < num_samples; i++) {
        //Read raw accelerometer data
        int16_t accel_x = read_raw_data(MPU6050_REG_ACCEL_XOUT_H);
        int16_t accel_y = read_raw_data(MPU6050_REG_ACCEL_XOUT_H + 2);
        int16_t accel_z = read_raw_data(MPU6050_REG_ACCEL_XOUT_H + 4);

        //Read raw gyroscope data
        int16_t gyro_x = read_raw_data(MPU6050_REG_GYRO_XOUT_H);
        int16_t gyro_y = read_raw_data(MPU6050_REG_GYRO_XOUT_H + 2);
        int16_t gyro_z = read_raw_data(MPU6050_REG_GYRO_XOUT_H + 4);

        accel_x_sum += accel_x;
        accel_y_sum += accel_y;
        accel_z_sum += accel_z;
        gyro_x_sum += gyro_x;
        gyro_y_sum += gyro_y;
        gyro_z_sum += gyro_z;

        sleep_ms(2); //delay for stability
    }

    //Calculate offsets
    accel_x_offset = accel_x_sum / num_samples;
    accel_y_offset = accel_y_sum / num_samples;
    accel_z_offset = (accel_z_sum / num_samples) - 16384; //Adjust for gravity
    gyro_x_offset = gyro_x_sum / num_samples;
    gyro_y_offset = gyro_y_sum / num_samples;
    gyro_z_offset = gyro_z_sum / num_samples;

    printf("Calibration complete:\n");
    printf("Accel Offsets: X: %d, Y: %d, Z: %d\n", accel_x_offset, accel_y_offset, accel_z_offset);
    printf("Gyro Offsets: X: %d, Y: %d, Z: %d\n", gyro_x_offset, gyro_y_offset, gyro_z_offset);
}

//process the current IMU data and correct it using the offset values
void read_sensor_data_corrected(int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                                int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z) {
    *accel_x = read_raw_data(MPU6050_REG_ACCEL_XOUT_H) - accel_x_offset;
    *accel_y = read_raw_data(MPU6050_REG_ACCEL_XOUT_H + 2) - accel_y_offset;
    *accel_z = read_raw_data(MPU6050_REG_ACCEL_XOUT_H + 4) - accel_z_offset;

    *gyro_x = read_raw_data(MPU6050_REG_GYRO_XOUT_H) - gyro_x_offset;
    *gyro_y = read_raw_data(MPU6050_REG_GYRO_XOUT_H + 2) - gyro_y_offset;
    *gyro_z = read_raw_data(MPU6050_REG_GYRO_XOUT_H + 4) - gyro_z_offset;
}

IMU_Reading read_imu() {
    IMU_Reading read;
    int16_t accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z;

    read_sensor_data_corrected(&accel_x, &accel_y, &accel_z, &gyro_x, &gyro_y, &gyro_z);

    read.ax = accel_x;
    read.ay = accel_y;
    read.az = accel_z;
    read.gx = gyro_x;
    read.gy = gyro_y;
    read.gz = gyro_z;

    return read;
}

void process_imu_buffer() {
    IMU_Reading read = read_imu();
    imu_buffer[imu_buffer_index] = read;
    imu_buffer_index = (imu_buffer_index + 1) % MAX_IMU_READINGS; 
}

void write_imu_buffer(FIL *file) {
    char imu_log[200];

    for (int i = 0; i < MAX_IMU_READINGS; i++) {
      int index = (imu_buffer_index + i) % MAX_IMU_READINGS; //adjust index for circular buffer
        snprintf(imu_log, sizeof(imu_log), 
                 "IMU: %d, %d, %d, %d, %d, %d\n", 
                 imu_buffer[index].ax, imu_buffer[index].ay, imu_buffer[index].az, 
                 imu_buffer[index].gx, imu_buffer[index].gy, imu_buffer[index].gz);
        UINT bytes_written;
        FRESULT fr = f_write(file, imu_log, strlen(imu_log), &bytes_written);
        if (fr != FR_OK) {
            printf("Error writing IMU data to the file: %d\n", fr);
            return;
        }
    }
}