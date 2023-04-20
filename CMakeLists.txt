set(TFT_DRIVER "st7735" CACHE STRING "Display driver")
set_property(CACHE TFT_DRIVER PROPERTY STRINGS st7735 ili9225)
set(TFT_DRIVERS st7735 ili9225 ili9341)

if(NOT TFT_DRIVER)
    set(TFT_DRIVER st7735)
endif()

if(NOT TFT_DRIVER IN_LIST TFT_DRIVERS)
    message(FATAL_ERROR "Invalid TFT_DRIVER=${TFT_DRIVER} (Allowed: st7735, ili9225, ili9341)")
endif()

message("TFT_DRIVER is ${TFT_DRIVER}")

if (NOT TARGET pico_tft)
    pico_add_library(pico_tft)

    target_include_directories(pico_tft INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_sources(pico_tft INTERFACE
                   ${CMAKE_CURRENT_LIST_DIR}/tft.c
                   ${CMAKE_CURRENT_LIST_DIR}/font.c
                   ${CMAKE_CURRENT_LIST_DIR}/${TFT_DRIVER}.c)

    target_link_libraries(pico_tft INTERFACE
                          pico_stdlib
                          pico_util
                          pico_divider
                          hardware_spi
                          hardware_timer
                          hardware_dma)
endif()
