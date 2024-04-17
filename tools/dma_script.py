print(
    'static const __attribute__((__section__(".flashdata.tft_dma_script"))) void *dma_row_script[2][TFT_RAW_HEIGHT + 1] = {'
)

for b in range(2):
    print("\t{")

    for i in range(512):
        print(f"#if TFT_RAW_HEIGHT > {i}")
        print(f"\t\t&buffer[{b}][TFT_RAW_WIDTH / TFT_SCALE * {i}],")
        print("#endif")

    print("\t\tNULL,")
    print("\t},")

print("};")
