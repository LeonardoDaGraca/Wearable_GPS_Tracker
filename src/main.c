/*
File: main.c
Author: Leonardo DaGraca

References: Carl's library for FAT filesystem with SPI driver for SD card on Raspberry Pi Pico
https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico?tab=readme-ov-file

Description: 
GPS Wearable Tracker - 
Hardware: Raspberry Pi Pico, gt-u7 gps module, micro sd card adapter module, and mpu6050 IMU

This program is able to obtain key movement data of the wearer and log the data
onto a MicroSD card. Using the power and high frequency updates of the GPS module,
we are able to capture movements accurately. Leveraging the GPS module with the IMU allows
us to capture sudden rapid movements that are key to calculate more advanced metrics.

The IMU samples at a higher frequency than the GPS. To remedy this a circular buffer is used to 
maintain the last 5 IMU readings for each GPS reading.

This program assumes the following hardware configuration:
GPS Module
| GPS   | UART1 | GPIO  | Pin   | 
| ----- | ----  | ----- | ---   | 
| RX    | TX    | 4     | 6     | 
| TX    | RX    | 5     | 7     | 
| GND   |       |       | 38    | 
| 3v3   |       |       | 36    | 

MPU6050 Module
| MPU   | I2C0  | GPIO  | Pin   | 
| ----- | ----  | ----- | ---   | 
| SCL   | SCL   | 1     | 2     | 
| SDA   | SDA   | 0     | 1     | 
| GND   |       |       | 28    | 
| VSYS  |       |       | 39    | 

MicroSD Module
|       | SPI0  | GPIO  | Pin   | SPI       | MicroSD   |  
| ----- | ----  | ----- | ---   | --------  | --------- |
| MISO  | RX    | 16    | 21    | DO        | DO        |
| MOSI  | TX    | 19    | 25    | DI        | DI        | 
| SCK   | SCK   | 18    | 24    | SCLK      | CLK       | 
| CS0   | CSn   | 17    | 22    | SS or CS  | CS        | 
| GND   |       |       | 18,33 |           | GND       | 
| VBUS  |       |       | 40    |           | VBUS      |

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpu6050.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "ff.h"
#include "sd_card.h"
#include <inttypes.h> 


#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define GPS_DIR "gps_logs"
#define IMU_DIR "imu_logs"

FATFS fs;
FIL gps_file;
FIL imu_file;
FRESULT fr;

void create_log_directory();
int get_session_counter();
void get_unique_filename(char *filename, int session, int is_imu);
uint64_t generate_timestamp();


int main() {
    stdio_init_all();

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    mpu6050_init();
    calibrate_mpu6050();

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    printf("Initializing SD card....\n");

    if (!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\n");
        return -1;
    }

    printf("SD card initiliazed...\n");

    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        printf("Error mounting the filesystem: %d\n", fr);
        return -1;
    }
    
    //init directory and filename
    create_log_directory();
    char gps_filename[50];
    char imu_filename[50];
    int session = get_session_counter();
    get_unique_filename(gps_filename, session, 0);
    get_unique_filename(imu_filename, session, 1);

    //open log files
    fr = f_open(&gps_file, gps_filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        printf("Error opening the file: %d\n", fr);
        return -1;
    }

    fr = f_open(&imu_file, imu_filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        printf("Error opening IMU log file: %d\n", fr);
        f_close(&gps_file);
        return -1;
    }

    //write CSV headers
    f_write(&gps_file, "Timestamp,NMEA\n", strlen("Timestamp,NMEA\n"), NULL);
    f_write(&imu_file, "Timestamp,IMU_Readings\n", strlen("Timestamp,IMU_Readings\n"), NULL);

    printf("Logging to file %s\n", gps_filename);
    printf("Logging to file %s\n", imu_filename);

    //buffer to read sentences from UART GPS signal
    char gprmc_buff[100] = {0};
    char gpvtg_buff[100] = {0};
    unsigned char is_gprmc_received = 0;
    unsigned char is_gpvtg_received = 0;

    printf("GPS Test: Waiting for data...\n");

    while (true) {
        process_imu_buffer();

        //read gps data
        if (uart_is_readable(UART_ID)) {
            char dat = uart_getc(UART_ID);
            
            static char sentence_code[5] = {0};
            static char gps_buff[100] = {0};
            static int gps_index = 0;

            if (dat == '$') {
                //reset for new sentence
                memset(sentence_code, 0, sizeof(sentence_code));
                memset(gps_buff, 0, sizeof(gps_buff));
                gps_index = 0;
            } else if (dat == '\r') {
                gps_buff[gps_index] = '\0';
                if (strncmp(sentence_code, "RMC", 3) == 0) {
                    strncpy(gprmc_buff, gps_buff, sizeof(gprmc_buff) - 1);
                    gprmc_buff[sizeof(gprmc_buff) - 1] = '\0';
                    is_gprmc_received = 1;
                }
                else if (strncmp(sentence_code, "VTG", 3) == 0) {
                    strncpy(gpvtg_buff, gps_buff, sizeof(gpvtg_buff) - 1);
                    gpvtg_buff[sizeof(gpvtg_buff) - 1] = '\0';
                    is_gpvtg_received = 1;
                }
            } else if (gps_index < sizeof(gps_buff) - 1) {
                //collect sentence data
                gps_buff[gps_index++] = dat;

                //update sentence code (only the first 3 characters after "$")
                if (gps_index >= 3 && gps_index <= 6) {
                    sentence_code[gps_index - 3] = dat;
                }
            }
        }
        if (is_gprmc_received || is_gpvtg_received) {
            printf("Writing to SD card...\n");
            uint64_t curr_timestamp = generate_timestamp();

            if (is_gprmc_received) {
                printf("GPRMC: %s\n", gprmc_buff);
                char time_stamp_rms[200];
                snprintf(time_stamp_rms, sizeof(time_stamp_rms),
                        "%" PRIu64 ",%s\n", curr_timestamp, gprmc_buff);

                UINT bytes_written;
                fr = f_write(&gps_file, time_stamp_rms, strlen(time_stamp_rms), &bytes_written);
            }
            if (is_gpvtg_received) {
                printf("GPVTG: %s\n", gpvtg_buff);
                char time_stamp_vtg[200];
                snprintf(time_stamp_vtg, sizeof(time_stamp_vtg),
                        "%" PRIu64 ",%s\n", curr_timestamp, gpvtg_buff);
                UINT bytes_written;
                fr = f_write(&gps_file, time_stamp_vtg, strlen(time_stamp_vtg), &bytes_written);
            }

            printf("Data written to the SD card.\n");
            write_imu_buffer(&imu_file, curr_timestamp);
            f_sync(&imu_file);
            f_sync(&gps_file); //ensure data is flushed to the SD card

            is_gprmc_received = 0;
            is_gpvtg_received = 0;
        }
    }
    f_close(&gps_file);
    f_close(&imu_file);
    f_unmount("0:");
    return 0;
}

void create_log_directory() {
    fr = f_mkdir(GPS_DIR);
    if (fr == FR_OK || fr == FR_EXIST) {
        printf("Directory '%s' is ready\n", GPS_DIR);
    }
    else {
        printf("Error with creating directory: %d\n", fr);
    }

    fr = f_mkdir(IMU_DIR);
    if (fr == FR_OK || fr == FR_EXIST) {
        printf("Directory '%s' is ready\n", IMU_DIR);
    }
    else {
        printf("Error with creating directory: %d\n", fr);
    }
}

int get_session_counter() {
    FIL counter_file;
    char counter_str[10] = {0};
    int counter = 1;

    if (f_open(&counter_file, "session_counter.txt", FA_READ) == FR_OK) {
        f_gets(counter_str, sizeof(counter_str), &counter_file);
        counter = atoi(counter_str);
        f_close(&counter_file);
    }

    counter++;
    if (f_open(&counter_file, "session_counter.txt", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
        sprintf(counter_str, "%d", counter);
        f_write(&counter_file, counter_str, strlen(counter_str), NULL);
        f_sync(&counter_file);
        f_close(&counter_file);
    }

    return counter;
}

void get_unique_filename(char *filename, int session, int is_imu) {
    if (is_imu) {
        sprintf(filename, "%s/imu_log_%d.csv", IMU_DIR, session);
    }
    else {
        sprintf(filename, "%s/gps_log_%d.csv", GPS_DIR, session);
    }
}

uint64_t generate_timestamp() {
    return time_us_64();
}