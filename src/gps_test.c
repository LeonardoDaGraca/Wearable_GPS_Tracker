/*
File: gps_test.c
Author: Leonardo DaGraca

Testing file to read GPS nmea sentences
*/
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 4
#define UART_RX_PIN 5

int main() {
    stdio_init_all();

    //initialize UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    char dat, buff[100], GGA_code[3] = {0};
    unsigned char IsitGGAstring = 0;
    unsigned char GGA_index = 0;
    unsigned char is_GGA_received_completely = 0;

    printf("GPS Test: Waiting for GGA data...\n");

    while (true) {
        if (uart_is_readable(UART_ID)) { //Check for available data on UART
            dat = uart_getc(UART_ID);   //Read character from UART

            if (dat == '$') {
                IsitGGAstring = 0;
                GGA_index = 0;
            } else if (IsitGGAstring == 1) {
                buff[GGA_index++] = dat;
                if (dat == '\r') {
                    is_GGA_received_completely = 1;
                }
            } else if (GGA_code[0] == 'G' && GGA_code[1] == 'G' && GGA_code[2] == 'A') {
                IsitGGAstring = 1;
                GGA_code[0] = 0;
                GGA_code[1] = 0;
                GGA_code[2] = 0;
            } else {
                GGA_code[0] = GGA_code[1];
                GGA_code[1] = GGA_code[2];
                GGA_code[2] = dat;
            }
        }

        if (is_GGA_received_completely == 1) {
            buff[GGA_index] = '\0'; //Null-terminate the buffer
            printf("GGA: %s\n", buff);

            is_GGA_received_completely = 0;
        }
    }
    
    return 0;
}
