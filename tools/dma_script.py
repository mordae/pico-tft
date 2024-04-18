print('static const __attribute__((__section__(".flashdata.tft_dma_script"))) void *')
print("dma_row_script[2][TFT_RAW_HEIGHT + 1] = {")
print("#define TFT_SCRIPT_ROW(x) ((TFT_RAW_WIDTH / TFT_SCALE) * ((x) / TFT_SCALE))")

for b in range(2):
    print("\t{")

    for i in range(320):
        print(f"#if TFT_RAW_HEIGHT >= {i}")
        print(f"\t\t&buffer[{b}][TFT_SCRIPT_ROW({i})],")
        print("#endif")

    print("\t\tNULL,")
    print("\t},")

print("};")
