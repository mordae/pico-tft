# pico-tft

To use these drivers with `pico-sdk`, modify your `CMakeLists.txt`:

```cmake
set(TFT_DRIVER ili9225 CACHE STRING "TFT Driver")
add_subdirectory(vendor/pico-tft)
target_link_libraries(your_target pico_tft ...)
```

If you do not have your board header file, create `include/boards/myboard.h` and use it like this:

```cmake
set(PICO_BOARD myboard)
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/include/boards)
```

Your header file may include the original board file, but it must define following:

```c
/* Pins to use: */
#define TFT_CS_PIN 21
#define TFT_SCK_PIN 22
#define TFT_MOSI_PIN 23
#define TFT_RS_PIN 24
#define TFT_RST_PIN 25

/* SPI device that corresponds with those pins: */
#define TFT_SPI_DEV spi0

/* Maximum speed to use: */
#define TFT_BAUDRATE (64 * 1000 * 1000)

/* Swap axes, or perhaps flip them: */
#define TFT_SWAP_XY 1
#define TFT_FLIP_X 0
#define TFT_FLIP_Y 0
```

See `include/tft.h` for interface.

Highlights:

- There is a basic 8×16 ready-to-use font embedded.
- Double buffering is in place to improve performance.
- 256 color palette is used to save space.
