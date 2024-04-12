if (NOT TARGET pico_tft)
  pico_add_library(pico_tft)

  target_include_directories(
    pico_tft
    INTERFACE
      ${CMAKE_CURRENT_LIST_DIR}/include
  )

  target_sources(
    pico_tft
    INTERFACE
      ${CMAKE_CURRENT_LIST_DIR}/tft.c
      ${CMAKE_CURRENT_LIST_DIR}/font.c
      ${CMAKE_CURRENT_LIST_DIR}/st7735.c
      ${CMAKE_CURRENT_LIST_DIR}/ili9225.c
      ${CMAKE_CURRENT_LIST_DIR}/ili9341.c
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
