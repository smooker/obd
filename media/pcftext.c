/*
 * pcftext — render a UTF-8 string with a PCF (or any FreeType-readable)
 * bitmap font to a PBM (P4) file. Pixel-perfect, no anti-aliasing,
 * no hinting. Designed to be called from a layout driver (Python now,
 * Qt later — same FreeType under the hood).
 *
 * Build: cc -O2 -Wall -o pcftext pcftext.c $(pkg-config --cflags --libs freetype2)
 *
 * Usage: pcftext <font.pcf[.gz]> <pixel_size> <text> <out.pbm>
 *
 * Output PBM is a tight bounding box around the rendered text. The
 * driver knows the font ascent and can position the result on a canvas.
 *
 * On stdout we print: "<width> <height> <baseline_y>\n" so the driver
 * doesn't need to re-parse the PBM header.
 */

#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void die(const char *msg) {
    fprintf(stderr, "pcftext: %s\n", msg);
    exit(1);
}

/* Decode one UTF-8 codepoint, advance *p. Returns 0 on end. */
static unsigned utf8_next(const char **p) {
    const unsigned char *s = (const unsigned char *)*p;
    if (!*s) return 0;
    unsigned cp;
    int extra;
    if      (*s < 0x80) { cp = *s;        extra = 0; }
    else if ((*s & 0xE0) == 0xC0) { cp = *s & 0x1F; extra = 1; }
    else if ((*s & 0xF0) == 0xE0) { cp = *s & 0x0F; extra = 2; }
    else if ((*s & 0xF8) == 0xF0) { cp = *s & 0x07; extra = 3; }
    else { cp = '?'; extra = 0; }
    s++;
    while (extra-- > 0 && (*s & 0xC0) == 0x80) {
        cp = (cp << 6) | (*s & 0x3F);
        s++;
    }
    *p = (const char *)s;
    return cp;
}

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr,
                "usage: %s <font> <pixel_size> <text> <out.pbm>\n", argv[0]);
        return 2;
    }
    const char *font_path = argv[1];
    int px = atoi(argv[2]);
    const char *text = argv[3];
    const char *out_path = argv[4];

    FT_Library lib;
    if (FT_Init_FreeType(&lib)) die("FT_Init_FreeType");

    FT_Face face;
    if (FT_New_Face(lib, font_path, 0, &face)) die("FT_New_Face");

    /* For PCF / embedded bitmaps: select an exact strike. */
    int strike = -1;
    for (int i = 0; i < face->num_fixed_sizes; i++) {
        if (face->available_sizes[i].height == px ||
            face->available_sizes[i].y_ppem >> 6 == px) {
            strike = i;
            break;
        }
    }
    if (strike < 0 && face->num_fixed_sizes > 0) {
        /* Fall back to nearest, but warn — caller asked for the wrong size. */
        int best = 0, bd = 1 << 30;
        for (int i = 0; i < face->num_fixed_sizes; i++) {
            int d = face->available_sizes[i].height - px;
            if (d < 0) d = -d;
            if (d < bd) { bd = d; best = i; }
        }
        strike = best;
        fprintf(stderr, "pcftext: warn: no strike for %dpx, using %dpx\n",
                px, face->available_sizes[strike].height);
    }
    if (strike >= 0) {
        if (FT_Select_Size(face, strike)) die("FT_Select_Size");
    } else {
        if (FT_Set_Pixel_Sizes(face, 0, px)) die("FT_Set_Pixel_Sizes");
    }

    /* First pass: measure. */
    int pen_x = 0;
    int min_y = 0, max_y = 0;
    const char *p = text;
    unsigned cp;
    while ((cp = utf8_next(&p))) {
        if (FT_Load_Char(face, cp,
                         FT_LOAD_RENDER | FT_LOAD_MONOCHROME |
                         FT_LOAD_NO_HINTING | FT_LOAD_TARGET_MONO)) continue;
        FT_GlyphSlot g = face->glyph;
        int top = -g->bitmap_top;
        int bot = top + (int)g->bitmap.rows;
        if (top < min_y) min_y = top;
        if (bot > max_y) max_y = bot;
        pen_x += g->advance.x >> 6;
    }
    int width = pen_x;
    int height = max_y - min_y;
    int baseline = -min_y;
    if (width <= 0 || height <= 0) die("empty text");

    /* Allocate 1-bit canvas, MSB-first rows, padded to byte. */
    int row_bytes = (width + 7) / 8;
    unsigned char *bits = calloc(row_bytes * height, 1);
    if (!bits) die("oom");

    /* Second pass: rasterize. */
    pen_x = 0;
    p = text;
    while ((cp = utf8_next(&p))) {
        if (FT_Load_Char(face, cp,
                         FT_LOAD_RENDER | FT_LOAD_MONOCHROME |
                         FT_LOAD_NO_HINTING | FT_LOAD_TARGET_MONO)) continue;
        FT_GlyphSlot g = face->glyph;
        FT_Bitmap *bm = &g->bitmap;
        int x0 = pen_x + g->bitmap_left;
        int y0 = baseline - g->bitmap_top;
        for (unsigned r = 0; r < bm->rows; r++) {
            int dy = y0 + (int)r;
            if (dy < 0 || dy >= height) continue;
            unsigned char *src = bm->buffer + r * bm->pitch;
            for (unsigned c = 0; c < bm->width; c++) {
                int bit = (src[c >> 3] >> (7 - (c & 7))) & 1;
                if (!bit) continue;
                int dx = x0 + (int)c;
                if (dx < 0 || dx >= width) continue;
                bits[dy * row_bytes + (dx >> 3)] |= 0x80 >> (dx & 7);
            }
        }
        pen_x += g->advance.x >> 6;
    }

    /* Write PBM P4 (binary, 1=black). We want 1=set-pixel, so caller can
     * tint as desired — we keep the natural "1 = ink" convention. */
    FILE *f = fopen(out_path, "wb");
    if (!f) die("fopen out");
    fprintf(f, "P4\n%d %d\n", width, height);
    fwrite(bits, 1, row_bytes * height, f);
    fclose(f);

    /* Layout info to stdout. */
    printf("%d %d %d\n", width, height, baseline);

    free(bits);
    FT_Done_Face(face);
    FT_Done_FreeType(lib);
    return 0;
}
