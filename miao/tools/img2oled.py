#!/usr/bin/env python3
"""img2oled.py - 把任意图片转成 STM32 OLED 能直接用的 C 数组（默认深色像素点亮）。

用法:
    python img2oled.py cat.png out.h [--w 128] [--h 64] [--name logo]
                      [--threshold 128] [--invert] [--dither]

输出格式与这块板子的 OLED_DrawBMP 完全匹配：
    - 按页组织（每页 8 行像素）
    - 每页里按列，从第 0 列到最后一列
    - 每个字节的 bit0 是最上面那一行
所以可以直接：
    OLED_DrawBMP(x, y, W, H, (unsigned char *)logo);
"""
import sys
import argparse
from PIL import Image


def to_page_bytes(img, threshold, invert, dither):
    img = img.convert("L")
    if dither:
        img = img.convert("1")          # Floyd-Steinberg
        bits = img.convert("L")
    else:
        bits = img.point(lambda v: 255 if v < threshold else 0)   # dark = lit

    w, h = bits.size
    out = []
    pages = (h + 7) // 8
    px = bits.load()
    for page in range(pages):
        for col in range(w):
            b = 0
            for bit in range(8):
                row = page * 8 + bit
                if row >= h:
                    continue
                on = px[col, row] > 0
                if invert:
                    on = not on
                if on:
                    b |= 1 << bit
            out.append(b)
    return out, w, h, pages


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("--w", type=int, default=128)
    ap.add_argument("--h", type=int, default=64)
    ap.add_argument("--name", default="img")
    ap.add_argument("--threshold", type=int, default=128)
    ap.add_argument("--invert", action="store_true")
    ap.add_argument("--dither", action="store_true")
    args = ap.parse_args()

    img = Image.open(args.src).convert("L").resize((args.w, args.h), Image.LANCZOS)
    data, w, h, pages = to_page_bytes(img, args.threshold, args.invert, args.dither)

    with open(args.dst, "w", encoding="utf-8") as f:
        guard = "__%s_H" % args.name.upper()
        f.write("/* %s : %dx%d, page layout, LSB=top */\n" % (args.name, w, h))
        f.write("#ifndef %s\n#define %s\n\n" % (guard, guard))
        f.write("static const unsigned char %s[%d] = {\n" % (args.name, len(data)))
        for i in range(0, len(data), 16):
            f.write("  " + ",".join("0x%02X" % b for b in data[i:i + 16]) + ",\n")
        f.write("};\n\n#endif\n")

    print("written %s  (%dx%d, %d bytes)" % (args.dst, w, h, len(data)))


if __name__ == "__main__":
    main()
