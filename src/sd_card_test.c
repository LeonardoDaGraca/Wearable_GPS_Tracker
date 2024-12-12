/*
File: sd_card_test.c
Author: Leonardo DaGraca

Testing file to mount sd card and then create, write, and read from file
*/
#include <stdio.h>
#include "pico/stdlib.h"
#include "ff.h"
#include "sd_card.h"


int main() {
    FATFS fs;
    FRESULT fr;
    FIL fil;
    int ret;
    char buf[100];
    char filename[] = "test.txt";

    stdio_init_all();
    printf("Entered program.\r\n");
    printf("\r\nSD card test. Press 'enter' to start.\r\n");
    while (true) {
        buf[0] = getchar();
        if ((buf[0] == '\r') || (buf[0] == '\n')) {
            break;
        }
    }

    //initialize SD card
    if (!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\r\n");
        while (true);
    }
    else {
        printf("SD card driver initialized successfully\r\n");
    }

    //mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
        while (true);
    }

    //open file for writing
    fr = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        while (true);
    }

    //write something to file
    ret = f_printf(&fil, "This is another test\r\n");
    if (ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
        while (true);
    }
    ret = f_printf(&fil, "of writing to an SD card.\r\n");
    if (ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
        while (true);
    }

    //close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        while (true);
    }

    //open file for reading
    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        while (true);
    }

    //print every line in file over serial
    printf("Reading from file '%s':\r\n", filename);
    printf("---\r\n");
    while (f_gets(buf, sizeof(buf), &fil)) {
        printf(buf);
    }
    printf("\r\n---\r\n");

    //close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        while (true);
    }

    //unmount drive
    f_unmount("0:");

    //loop forever doing nothing
    while (true) {
        sleep_ms(1000);
    }
}