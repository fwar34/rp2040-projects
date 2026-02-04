# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(myblink C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(myblink myblink.c
                       board/src/bsp.c
                       board/src/spinlock.c
                       drivers/src/led.c
                       drivers/src/logic.c
                    #    drivers/src/dht11.c
                       drivers/src/dht11_it.c
                    #    drivers/src/ec11.c
                       drivers/src/ec11_it.c
                    #    drivers/src/ec11_pio.c
                    #    drivers/src/lcd.c
                       drivers/src/st7735s.c
                       app/src/util_print.c
                       app/src/irq_handler.c
                       app/src/uart1.c
                       app/src/session.c
                       app/src/receive_buffer.c
                       app/src/input_process.c)

pico_set_program_name(myblink "myblink")
pico_set_program_version(myblink "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(myblink 1)
pico_enable_stdio_usb(myblink 1)

# Add the standard library to the build
target_link_libraries(myblink
        pico_stdlib
        hardware_gpio
        hardware_spi
        hardware_pwm
        hardware_timer
        hardware_dma
        hardware_uart
        hardware_irq
        hardware_pio
        # hardware_spin_lock
        qpc)

# Add the standard include files to the build
target_include_directories(myblink PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        PRIVATE board/include
        PRIVATE app/include
        PRIVATE drivers/include
)

pico_add_extra_outputs(myblink)

# my modify
add_subdirectory(components/qpc)
target_compile_definitions(myblink PRIVATE LOGIC_TEST)