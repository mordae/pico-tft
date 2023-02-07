# pico-tft

To use these drivers with `pico-sdk`, modify your `CMakeLists.txt`:

```cmake
add_subdirectory(vendor/pico-tft)
target_link_libraries(your_target pico_tft ...)
```

See `include/tft.h` for interface.

Highlights:

- There is a basic 8×16 ready-to-use font embedded.
- Double buffering is in place to improve performance.
- 16 color palette is used to save space.
