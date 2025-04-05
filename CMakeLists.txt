if (NOT TARGET pico_tft)
  pico_add_library(pico_tft)
  pico_generate_pio_header(pico_tft ${CMAKE_CURRENT_LIST_DIR}/lut.pio)

  target_include_directories(
    pico_tft
    INTERFACE
      ${CMAKE_CURRENT_LIST_DIR}/include
  )

  target_sources(
    pico_tft
    INTERFACE
      ${CMAKE_CURRENT_LIST_DIR}/tft.c
      ${CMAKE_CURRENT_LIST_DIR}/tft_dma_script.c
      ${CMAKE_CURRENT_LIST_DIR}/font.c
      ${CMAKE_CURRENT_LIST_DIR}/st7789v.c
  )

  target_link_libraries(
    pico_tft
    INTERFACE
      pico_stdlib
      pico_util
      pico_divider
      hardware_spi
      hardware_timer
      hardware_dma
  )
endif()
