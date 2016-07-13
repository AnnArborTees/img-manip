#include "image.h"
#include <memory>
#include <stdlib.h>
#include <cmath>

#define PNG_CATCH(png) if (setjmp(png_jmpbuf(png)))

namespace mockbot {
    //
    // These are some static helper methods used throughout
    //

    static const uint8_t empty_pixel[] = { 0xFF, 0xFF, 0xFF, 0x00 };

    static bool pixel_eq(uint8_t* p1, uint8_t* p2, int bytes_per_pixel) {
        return p1[0] == p2[0] &&
               p1[1] == p2[1] &&
               p1[2] == p2[2] &&
               (bytes_per_pixel == 3 || p1[3] == p2[3]);
    }

    static double px_to_real(uint8_t color) {
        return double(color) / 255.0;
    }

    static uint8_t real_to_px(double color) {
        // printf("COLOR: %f ||| ", color);
        return uint8_t(color * 255.0);
    }

    static double decimal(double n) {
        return n - floor(n);
    }

    static uint8_t px_alpha(uint8_t* pixel, int bytes_per_pixel) {
        if (bytes_per_pixel == 4) return pixel[3];
        else return 0xFF;
    }

    static double real_alpha(double* pixel, int bytes_per_pixel) {
        if (bytes_per_pixel == 4) return pixel[3];
        else return 1.0;
    }

    static void hexcode_to_pixel(uint8_t* pixel, std::string& str) {
        char component[3];
        component[2] = '\0';

        for (int i = 0, p = 0; i < 6; i += 2, p++) {
            component[0] = str[i];
            component[1] = str[i + 1];

            pixel[p] = (uint8_t)std::stoi(component, nullptr, 16);
        }
        pixel[3] = 0xFF;
    }

    //
    // ImageWrite and ImageRead wrappers for destroying libpng resources using RAII
    //

    ImageWrite::ImageWrite() : png(NULL), info(NULL) {
    }
    ImageWrite::~ImageWrite() {
        if (png)
            png_destroy_write_struct(&png, info ? &info : NULL);
    }

    ImageRead::ImageRead() : png(NULL), info(NULL) {
    }
    ImageRead::~ImageRead() {
        if (png)
            png_destroy_read_struct(&png, info ? &info : NULL, NULL);
    }

    //
    // Image class definitions
    //

    Image::Image() : image_data(NULL) {
    }
    Image::~Image() {
        cleanup();
    }

    void Image::cleanup() {
        if (image_data)
            free(image_data);
    }

    bool Image::load_file(FILE* input) {
        ImageRead r;
        //
        // Validate signature and create structures
        //
        uint8_t sig[8];
        fread(sig, 1, 8, input);
        if (!png_check_sig(sig, 8)) {
            error = "Invalid PNG signature";
            return false;
        }

        cleanup();

        r.png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!r.png) {
            error = "Failed to create PNG read structure";
            return false;
        }

        r.info = png_create_info_struct(r.png);
        if (!r.info) {
            error = "Failed to create PNG info structure";
            return false;
        }

        PNG_CATCH(r.png) {
            error = "Libpng exception was raised";
            return false;
        }

        //
        // Actually load image into memory
        //

        png_init_io(r.png, input);
        png_set_sig_bytes(r.png, 8);
        png_read_info(r.png, r.info);
        png_get_IHDR(r.png, r.info, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

        switch (color_type) {
        case COLOR_TYPE_RGB:  bytes_per_pixel = 3; break;
        case COLOR_TYPE_RGBA: bytes_per_pixel = 4; break;
        default:
            error = "Unsupported color type: " + std::to_string(color_type);
            return false;
        }

        // This probably won't ever show up in our case.
        if (bit_depth == 16) {
            png_set_strip_16(r.png);
            png_read_update_info(r.png, r.info);
        }

        uint32_t rowbytes = png_get_rowbytes(r.png, r.info);
        image_data = (uint8_t*)malloc(rowbytes * height);
        if (!image_data) {
            error = "Not enough memory available to allocate image data";
            return false;
        }
        else {
            // Libpng wants these "2d array" style row pointers
            uint8_t** row_pointers = (uint8_t**)malloc(height * sizeof(uint8_t*));
            for (int i = 0; i < height; ++i)
                row_pointers[i] = image_data + i * rowbytes;
            png_read_image(r.png, row_pointers);
            free(row_pointers);
        }

        return true;
    }

    bool Image::save(FILE* output) {
        ImageWrite w;

        w.png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!w.png)
            return false; // out of memory!

        w.info = png_create_info_struct(w.png);
        if (!w.info) return false;

        PNG_CATCH(w.png) return false;

        png_init_io(w.png, output);
        png_set_compression_level(w.png, Z_BEST_SPEED);
        png_set_IHDR(
            w.png, w.info,
            width, height,
            bit_depth, bytes_per_pixel == 4 ? COLOR_TYPE_RGBA : COLOR_TYPE_RGB,
            PNG_INTERLACE_ADAM7,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT
        );

        png_time modtime;
        png_convert_from_time_t(&modtime, time(NULL));
        png_set_tIME(w.png, w.info, &modtime);

        png_write_info(w.png, w.info);
        {
            int rowbytes = width * bytes_per_pixel;
            // Libpng wants these "2d array" style row pointers
            uint8_t** row_pointers = (uint8_t**)malloc(height * sizeof(uint8_t*));
            for (int i = 0; i < height; ++i)
                row_pointers[i] = image_data + i * rowbytes;

            png_write_image(w.png, row_pointers);
            free(row_pointers);
        }
        png_write_end(w.png, NULL);

        return true;
    }

    bool Image::fill_blank(uint8_t* bg_pixel, int new_width, int new_height) {
        cleanup();

        width           = new_width;
        height          = new_height;
        bit_depth       = 8;
        color_type      = COLOR_TYPE_RGBA;
        bytes_per_pixel = 3;

        image_data = (uint8_t*)malloc(width * height * bytes_per_pixel);

        size_t area = (size_t)width * (size_t)height;
        for (size_t i = 0; i < area; i++)
            memcpy(&image_data[i * bytes_per_pixel], bg_pixel, bytes_per_pixel);

        return true;
    }

    bool Image::fill_blank(std::string hexcode, int new_width, int new_height) {
        if (hexcode.size() != 7) return false;

        uint8_t bg_pixel[4];
        hexcode.erase(0, 1); // Chop off the '#'
        hexcode_to_pixel(bg_pixel, hexcode);

        return fill_blank(bg_pixel, new_width, new_height);
    }

    bool Image::loaded() {
        return image_data != NULL;
    }

    bool Image::make_background_transparent() {
        if (color_type == COLOR_TYPE_RGBA && image_data[3] == 0) {
            // This means we're already transparent. (top left pixel's alpha channel is 0)
            return true;
        }
        else {
            uint8_t* transparent_image_data = (uint8_t*)malloc(width * 4 * height);
            if (transparent_image_data == NULL)
                return false;

            uint8_t* bg_color = image_data + 0;// first pixel (`+ 0` not necessary)
            size_t in_area = width * height;

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
            color_type = COLOR_TYPE_RGBA;
            bytes_per_pixel = 4;
            return true;
        }
    }

    //
    // Gives a pointer to the red component at the given coordinates
    //
    uint8_t* Image::pixel(int x, int y) {
        int rowbytes = width * bytes_per_pixel;
        return &image_data[(x * bytes_per_pixel) + (y * rowbytes)];
    }

    bool Image::set_background_to(std::string hexcode, Compositor* comp) {
        if (bytes_per_pixel == 3) return false;
        if (hexcode.size() != 7) return false;

        uint8_t pixel[4];
        hexcode.erase(0, 1); // Chop off the '#'
        hexcode_to_pixel(pixel, hexcode);

        double real_pixel[4];
        for (int c = 0; c < 4; c++)
            real_pixel[c] = px_to_real(pixel[c]);

        const double dest_alpha = real_pixel[3]; // Should always be 0xFF

        size_t area = (size_t)width * (size_t)height;
        for (int i = 0; i < area; i++) {
            uint8_t* top_pixel = &image_data[i * bytes_per_pixel];

            double src_alpha = px_to_real(top_pixel[3]);
            double one_minus_src_alpha = 1.0 - src_alpha;

            for (int c = 0; c < 3; c++) {
                double dest_color = real_pixel[c];
                double src_color  = px_to_real(top_pixel[c]);

                double new_color = comp->blend_color(src_color, src_alpha, dest_color, dest_alpha, one_minus_src_alpha);
                if (new_color > 1.0) new_color = 1.0;

                top_pixel[c] = real_to_px(new_color);
            }
            double alpha = comp->blend_alpha(src_alpha, dest_alpha);
            if (alpha > 1.0) alpha = 1.0;
            top_pixel[3] = real_to_px(alpha);
        }

        return true;
    }

    //
    // Copy other's pixels onto this's pixels at the given coordinates and size
    //
    bool Image::composite(Image& other, int other_x, int other_y, int other_new_width, int other_new_height, Compositor* comp) {
        int rowbytes = width * bytes_per_pixel;

        // Number of big-image pixels per little-image pixel
        // (big-image being unmodified `other`, little-image being `other` resized to `other_new_width` by `other_new_height`)
        double x_old_per_new = double(other.width)  / double(other_new_width);
        double y_old_per_new = double(other.height) / double(other_new_height);

        int max_x = other_x + other_new_width;
        int max_y = other_y + other_new_height;

        // Used to store the pixel average when scaling down.
        double new_pixel[4];

        // NOTE: x and y are in this's image space.
        for (int y = other_y; y < max_y; y++) {
            if (y < 0 || y >= height) continue;

            for (int x = other_x; x < max_x; x++) {
                if (x < 0 || x >= width) continue;

                uint8_t* dest_pixel = &image_data[(x * bytes_per_pixel) + (y * rowbytes)];

                // a "new src" pixel probably refers to multiple "old src" pixels, so we want
                // the average of those "old src" pixels.
                double oldsrc_x_begin = double(x - other_x) * double(x_old_per_new);
                double oldsrc_y_begin = double(y - other_y) * double(y_old_per_new);
                double oldsrc_x_end = oldsrc_x_begin + x_old_per_new;
                double oldsrc_y_end = oldsrc_y_begin + y_old_per_new;

                // Gather sum for average:
                double total_count = 0.0;
                int start_x = (int)oldsrc_x_begin;
                int start_y = (int)oldsrc_y_begin;
                int finish_x = (int)oldsrc_x_end;
                int finish_y = (int)oldsrc_y_end;

                for (int i = 0; i < other.bytes_per_pixel; i++)
                    new_pixel[i] = 0.0;

                for (int v = start_y; v < finish_y; v++) {
                    for (int u = start_x; u < finish_x; u++) {
                        total_count += 1.0;
                        uint8_t* oldsrc_pixel = other.pixel(u, v);

                        for (int i = 0; i < other.bytes_per_pixel; i++)
                            new_pixel[i] += px_to_real(oldsrc_pixel[i]);
                    }
                }

                // Compute average and convert back to 1byte colors
                double dest_alpha = px_to_real(px_alpha(dest_pixel, bytes_per_pixel));
                double src_alpha;
                if (other.bytes_per_pixel == 4)
                    src_alpha = new_pixel[3] / total_count;
                else
                    src_alpha = 1.0;

                double one_minus_src_alpha = 1.0 - src_alpha;
                // NOTE This (inv_alpha) only has an effect when the image is blending into empty space (dest_alpha = 0)
                // which won't happen in our use case as far as I know.
                //
                //   double inv_alpha = 1.0 / (src_alpha + dest_alpha * one_minus_src_alpha);
                //
                // Multiplying the result of comp->blend_color by inv_alpha will fix that weirdness.

                for (int i = 0; i < 3; i++) {
                    double dest_color = px_to_real(dest_pixel[i]);
                    double src_color  = new_pixel[i] / total_count;

                    double new_color = comp->blend_color(src_color, src_alpha, dest_color, dest_alpha, one_minus_src_alpha);
                    if (new_color > 1.0) new_color = 1.0;

                    dest_pixel[i] = real_to_px(new_color);
                }
                if (bytes_per_pixel == 4) {
                    double alpha = comp->blend_alpha(src_alpha, dest_alpha);
                    if (alpha > 1.0) alpha = 1.0;
                    dest_pixel[3] = real_to_px(alpha);
                }
            }
        }

        return true;
    }

    std::string Image::last_error() {
        return error;
    }

    double CompositeOver::blend_color(double srcc, double srca, double dstc, double dsta, double omsrca) {
        return srcc * srca + dstc * dsta * omsrca;
    }
    double CompositeOver::blend_alpha(double srca, double dsta) {
        return dsta + srca;
    }

    double CompositeMultiply::blend_color(double srcc, double srca, double dstc, double dsta, double omsrca) {
        return (srcc * srca * dstc) + (dstc * dsta * omsrca);
    }
    double CompositeMultiply::blend_alpha(double srca, double dsta) {
        return dsta + srca;
    }
}
