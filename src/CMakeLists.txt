cmake_minimum_required(VERSION 3.13)

# Include the Pico SDK
include(pico_sdk_import.cmake)

# Set the project name and programming languages
project(test_project C CXX ASM)

# Set the C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Add the executable for gps_test.c
# add_executable(gps_test
#   gps_test.c
#   lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI/hw_config.c
# )

# add_executable(sd_card_test
#   sd_card_test.c
#   lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI/hw_config.c
# )

# Add the executable for main.c
# add_executable(mpu6050_i2c
#   mpu6050_i2c.c
# )

# Add the executable for main.c
add_executable(main
  main.c
  mpu6050_i2c.c
  ../lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI/hw_config.c
)


# Add FatFs source files from no-OS-FatFS-SD-SPI-RPi-Pico
add_subdirectory(../lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)


# Set the include directories for gps_test (current directory)
# target_include_directories(gps_test PRIVATE
#     ${CMAKE_CURRENT_LIST_DIR}
#     lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI
# )

# target_include_directories(sd_card_test PRIVATE
#     ${CMAKE_CURRENT_LIST_DIR}
#     lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI
# )

# Set the include directories for gps_test (current directory)
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ../lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI
)

# target_include_directories(mpu6050_i2c PRIVATE
#     ${CMAKE_CURRENT_LIST_DIR}
#     lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI
# )


# # Enable USB output if desired
# pico_enable_stdio_usb(gps_test 1)
# # Enable UART output if desired
# pico_enable_stdio_uart(gps_test 1)


# pico_enable_stdio_uart(sd_card_test 1)
# pico_enable_stdio_usb(sd_card_test 1)

# Enable USB output if desired
pico_enable_stdio_usb(main 1)
# Enable UART output if desired
pico_enable_stdio_uart(main 1)

# Enable USB output if desired
# pico_enable_stdio_usb(mpu6050_i2c 1)
# # Enable UART output if desired
# pico_enable_stdio_uart(mpu6050_i2c 1)

# Add extra outputs for UF2, bin, etc.
# pico_add_extra_outputs(gps_test)
# pico_add_extra_outputs(sd_card_test)
pico_add_extra_outputs(main)

# pico_add_extra_outputs(mpu6050_i2c)

# Link the necessary libraries
# target_link_libraries(gps_test
#   pico_stdlib
#   hardware_uart
#   hardware_spi
#   FatFs_SPI
# )

# target_link_libraries(sd_card_test
#   pico_stdlib
#   hardware_uart
#   hardware_spi
#   FatFs_SPI
# )

target_link_libraries(main
  pico_stdlib
  hardware_uart
  hardware_spi
  FatFs_SPI
  hardware_i2c
)

# target_link_libraries(mpu6050_i2c
#   pico_stdlib
#   hardware_uart
#   hardware_i2c
# )