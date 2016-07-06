#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>
#include <png.h>
#include <zlib.h>
#include <pngconf.h>

#define COLOR_TYPE_RGBA 6
#define COLOR_TYPE_RGB  2

#ifdef DEBUG
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) do {} while (0)
#endif

const uint8_t empty_pixel[] = { 0xFF, 0xFF, 0xFF, 0x00 };

int pixel_eq(uint8_t* p1, uint8_t* p2, int bytes_per_pixel) {
    return p1[0] == p2[0] &&
           p1[1] == p2[1] &&
           p1[2] == p2[2] &&
           (bytes_per_pixel == 3 || p1[3] == p2[3]);
}

int transform(FILE* input, FILE* output) {
    // ======================
    // Check signature to make sure it's actually a PNG
    // ======================

    uint8_t sig[8];
    fread(sig, 1, 8, input);
    if (!png_check_sig(sig, 8))
        return 1;

    // ======================
    // Initialize PNG file
    // ======================
    png_structp png;
    png_infop   info;

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
        return 1; // out of memory!

    info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        return 1;
    }

    if (setjmp(png_jmpbuf(png))) {
    duck_out:
        png_destroy_read_struct(&png, &info, NULL);
        return 1;
    }

    png_init_io(png, input);
    png_set_sig_bytes(png, 8);
    png_read_info(png, info);

    // ======================
    // Set up the image in memory
    // ======================

    uint32_t in_width;
    uint32_t in_height;
    int bit_depth;
    int color_type;
    int bytes_per_pixel;

    png_get_IHDR(png, info, &in_width, &in_height, &bit_depth, &color_type, NULL, NULL, NULL);
    // printf("Width: %i\nHeight: %i\nBit Depth: %i\nColor Type: %i\n", in_width, in_height, bit_depth, color_type);

    switch (color_type) {
    case COLOR_TYPE_RGB:  bytes_per_pixel = 3; break;
    case COLOR_TYPE_RGBA: bytes_per_pixel = 4; break;
    default:
        // Only accept RGB or RGBA
        goto duck_out;
    }

    if (bit_depth == 16) {
        png_set_strip_16(png);
        png_read_update_info(png, info);
    }

    uint32_t rowbytes = png_get_rowbytes(png, info);
    uint8_t* image_data = (uint8_t*)malloc(rowbytes * in_height);
    if (image_data == NULL) {
        // malloc didn't work so we're out of memory
        goto duck_out;
    }

    {
        uint8_t** row_pointers = (uint8_t**)malloc(in_height * sizeof(uint8_t*));
        for (int i = 0; i < in_height; ++i) {
            // Libpng wants these "2d array" style row pointers
            row_pointers[i] = image_data + i * rowbytes;
        }
        png_read_image(png, row_pointers);
        free(row_pointers);
    }

    // ======================
    // Add transparency
    // ======================

    if (color_type == COLOR_TYPE_RGBA && image_data[3] == 0) {
        // This means we're already transparent. (top left pixel's alpha channel is 0)
        dprintf("Already transparent - skipping removing transparency");
    }
    else {
        uint8_t* transparent_image_data = (uint8_t*)malloc(in_width * 4 * in_height);
        if (transparent_image_data == NULL) {
            // Outta effin' memory
            free(image_data);
            goto duck_out;
        }

        uint8_t* bg_color = image_data + 0;// first pixel (`+ 0` not necessary)
        size_t in_area = in_width * in_height;

        for (size_t i = 0; i < in_area; i++) {
            uint8_t* src_pixel  = image_data + i * bytes_per_pixel;
            uint8_t* dest_pixel = transparent_image_data + i * 4;

            if (pixel_eq(src_pixel, bg_color, bytes_per_pixel)) {
                memcpy(dest_pixel, empty_pixel, sizeof(empty_pixel));
            }
            else {
                memcpy(dest_pixel, src_pixel, bytes_per_pixel);
                if (bytes_per_pixel == 3)
                    dest_pixel[3] = 0xFF;
            }
        }

        free(image_data);
        image_data = transparent_image_data;
    }
    // Now image_data has transparency
    bytes_per_pixel = 4;

    // ======================
    // Resize and reposition
    // ======================

    // NOTE these could become adjustable in the future if we want to make this script more flexible
    uint32_t from_dpi = 150;
    uint32_t to_dpi = 300;
    uint32_t scale_factor = to_dpi / from_dpi;

    uint32_t out_width = 15 * to_dpi;
    uint32_t out_height = 18 * to_dpi;

    uint8_t* resized_image_data = (uint8_t*)malloc(out_width * 4 * out_height);
    uint8_t** resized_rows = (uint8_t**)malloc(out_height * sizeof(uint8_t*));

    for (int y = 0; y < out_height; ++y) {
        resized_rows[y] = resized_image_data + y * out_width * 4;

        for (int x = 0; x < out_width; ++x) {
            uint8_t* dest_pixel = &resized_rows[y][x * 4];

            // Center horizontally
            int src_x = x - (out_width - in_width * scale_factor) / 2;
            // Keep at top verticaly
            int src_y = y;

            src_x /= scale_factor;
            src_y /= scale_factor;

            if (src_x < 0 || src_x >= in_width || src_y >= in_height) {
                memcpy(dest_pixel, empty_pixel, sizeof(empty_pixel));
            }
            else {
                uint8_t* src_pixel = image_data + (src_y * 4 * in_width + src_x * 4);
                memcpy(dest_pixel, src_pixel, 4);
            }
        }
    }

    free(image_data);
    image_data = resized_image_data;

    // ======================
    // Save results
    // ======================

    png_structp out_png;
    png_infop   out_info = NULL;
    out_png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!out_png)
        return 2; // out of memory!

    out_info = png_create_info_struct(out_png);
    if (!info) {
        png_destroy_write_struct(&out_png, NULL);
        free(resized_rows);
        free(image_data);
        goto duck_out;
    }

    if (setjmp(png_jmpbuf(out_png))) {
    write_failed:
        png_destroy_write_struct(&out_png, (out_info ? &out_info : NULL));
        png_destroy_read_struct(&png, &info, NULL);
        free(resized_rows);
        free(image_data);
        return 2;
    }

    png_init_io(out_png, output);
    png_set_compression_level(out_png, Z_BEST_SPEED);
    png_set_IHDR(
        out_png, out_info,
        out_width, out_height,
        bit_depth, COLOR_TYPE_RGBA,
        PNG_INTERLACE_ADAM7,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    png_time modtime;
    png_convert_from_time_t(&modtime, time(NULL));
    png_set_tIME(out_png, out_info, &modtime);

    png_write_info(out_png, out_info);
    png_write_image(out_png, resized_rows);
    png_write_end(out_png, NULL);

    // ======================
    // Clean up
    // ======================
    free(resized_rows);
    free(image_data);
    png_destroy_read_struct(&png, &info, NULL);
    png_destroy_write_struct(&out_png, &out_info);
    return 0;
}

char* change_14x16_to_15x18(char* in) {
    char* sub = strstr(in, "14x16.png");

    if (!sub)
        return NULL;

    size_t substr_pos = sub - in;

    size_t in_size = strlen(in) + 1; // <- +1 for terminating 0
    char* out = (char*)malloc(in_size);
    memcpy(out, in, in_size);
    memcpy(out + substr_pos, "15x18.png", strlen("15x18.png"));

    return out;
}

int main(int argc, char** argv) {
    // Read arguments for filenames to transform
    for (int i = 1; i < argc; ++i) {
        char* in_filename = argv[i];
        FILE* input = fopen(in_filename, "rb");

        if (!input) {
            fprintf(stderr, "Couldn't read file \"%s\"\n", in_filename);
            return 1;
        }

        char* out_filename = change_14x16_to_15x18(in_filename);

        if (!out_filename) {
            fprintf(stderr, "Expected to find \"14x16\" in the filename \"%s\"\n", in_filename);
            return 1;
        }

        FILE* output = fopen(out_filename, "wb");

        if (!output) {
            fprintf(stderr, "Couldn't open file for writing \"%s\"\n", out_filename);
            return 1;
        }

        transform(input, output);

        // Since out_filename is malloced in change_14x16_to_15x18:
        free(out_filename);
        fclose(input);
        fclose(output);
    }

    return 0;
}
