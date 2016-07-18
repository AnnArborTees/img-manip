#include <string>
#include <iostream>
#include "image.h"
#include <stdio.h>

using namespace mockbot;

bool cstr_eq(const char* s1, const char* s2) {
    size_t i = 0;
    char c1;
    char c2;

    while (true) {
        c1 = s1[i];
        c2 = s2[i];

        if (c1 != c2)
            return false;

        if (c1 == '\0')
            return true;

        i += 1;
    }
}

int int_arg(char* arg) {
    if (cstr_eq(arg, "--"))
        return -1;
    else
        return atoi(arg);
}

int perform_composite(int argc, char** argv) {
    if (argc == 9) {
        Image img1;
        Image img2;

        FILE* f1 = fopen(argv[1], "rb");
        if (!f1) {
            std::cerr << "Couldn't open " << argv[1] << '\n';
            return 2;
        }

        FILE* f2 = fopen(argv[2], "rb");
        if (!f2) {
            std::cerr << "Couldn't open " << argv[2] << '\n';
            return 2;
        }

        int x = std::stoi(std::string(argv[3]));
        int y = std::stoi(std::string(argv[4]));
        int w = std::stoi(std::string(argv[5]));
        int h = std::stoi(std::string(argv[6]));

        if (!img1.load_file(f1)) {
            std::cerr << "Failed decode PNG: " << argv[1] << '\n';
            return 2;
        }
        if (!img2.load_file(f2)) {
            std::cerr << "Failed decode PNG: " << argv[2] << '\n';
            return 2;
        }

        img2.make_background_transparent();

        CompositeOver over;
        CompositeMultiply multiply;
        Compositor* comp;

        std::string operation = argv[7];
        if (operation == "multiply")
            comp = &multiply;
        else
            comp = &over;

        if (!img1.composite(img2, x, y, w, h, comp)) {
            std::cerr << "Failed to composite " << argv[2] << " onto " << argv[1] << '\n';
            return 2;
        }

        fclose(f1);
        fclose(f2);

        FILE* f3 = fopen(argv[8], "wb");
        if (!f3) {
            std::cerr << "Couldn't reopen " << argv[9] << " for writing\n";
            return 2;
        }

        if (!img1.save(f3)) {
            std::cerr << "Failed to encode result to " << argv[9] << '\n';
            return 2;
        }
        fclose(f3);
        return 0;
    }
    else {
        std::cout << "Do `" << argv[0] << " file1 file2 x y width height (over|multiply) outputfile` to comp file2 onto file1 at the given position/dimensions, then save into outputfile\n";
        return 1;
    }
}

// Syntax:
//           (0)        (1)     (2)             (3)             (4)           (5) (6) (7)   (8)  (9) (10) (11) (12)
//   mockbot thumbnail  #00FF00 img/heather.png img/artwork.png img/swash.png 500 10  over  1081 501 2265 3513 img/result.png
//
// (0):  subcommand - always 'thumbnail'
// (1):  background color, set to "--" to use the first pixel of the artwork image
// (2):  heather file path, set to "--" to not use heather
// (3):  artwork file path
// (4):  swash file path
// (5):  canvas width and height
// (6):  padding
// (7):  artwork composition method
// (8):  artwork x (rel. to the image's topleft)
// (9):  artwork y
// (10): artwork width (within its image - not the image dimensions)
// (11): artwork height
// (12): output file name
int perform_thumbnail(int argc, char** argv) {
    if (argc != 13) {
        std::cerr << "Expected exactly 12 arguments after 'thumbnail'\n";
        return 1;
    }

    CompositeOver     composite_over;
    CompositeMultiply composite_multiply;

    Image canvas;
    Image artwork;
    Image swash;
    Image heather;

    std::string color_hexcode = argv[1];

    int canvas_dim = std::stoi(std::string(argv[5]));
    int padding    = std::stoi(std::string(argv[6]));

    // Common routine to load an image with error checking.
#define LOAD_IMAGE(image, filename)                                                                                       \
do {                                                                                                                       \
    FILE* f = fopen(filename, "rb");                                                                                        \
    if (!f) { std::cerr << "Failed to open file " << (filename) << '\n'; return 2; }                                         \
    bool did_load = (image).load_file(f);                                                                                     \
    fclose(f);                                                                                                                 \
    if (!did_load) { std::cerr << "Failed to decode file " << (filename) << " -- " << (image).last_error() << '\n'; return 3; } \
} while (0)

    LOAD_IMAGE(artwork, argv[3]);
    LOAD_IMAGE(swash,   argv[4]);

    if (color_hexcode == "--")
        canvas.fill_blank(artwork.pixel(0, 0), canvas_dim, canvas_dim);
    else
        canvas.fill_blank(color_hexcode, canvas_dim, canvas_dim);

    artwork.make_background_transparent();

    const char* heather_filename = argv[2];
    if (!cstr_eq(heather_filename, "--")) {
        LOAD_IMAGE(heather, heather_filename);
        canvas.composite(heather, 0, 0, canvas_dim, canvas_dim, &composite_multiply);
    }

    // Here we calculate the "cropped" dimensions and offsets for the artwork.
    double canvas_width  = (double)canvas_dim;
    double canvas_height = (double)canvas_dim;
    double art_x      = (double)std::stoi(std::string(argv[8])); // art_* variables are in fullsize-art-image space
    double art_y      = (double)std::stoi(std::string(argv[9]));
    double art_width  = (double)std::stoi(std::string(argv[10]));
    double art_height = (double)std::stoi(std::string(argv[11]));
    double dpadding   = (double)padding;
    double target_box = double(canvas_dim) - (2.0 * dpadding);
    double img_width, img_height;
    double width, height; // these width,height,x,y variables are in copped&resized image space
    double x, y;

    // Fit the image into the target_box x target_box square
    if (art_width > art_height) {
        width = target_box;
        height = width * (art_height / art_width);
    }
    else {
        height = target_box;
        width = height * (art_width / art_height);
    }
    x = (width / art_width) * art_x;
    y = (height / art_height) * art_y;

    img_width  = (width  / art_width)  * double(artwork.width);
    img_height = (height / art_height) * double(artwork.height);

    // Center gravity
    double offset_x = (canvas_width - width) / 2.0;
    double offset_y = (canvas_height - height) / 2.0;

    Compositor* comp;
    std::string operation = argv[7];
    if (operation == "multiply")
        comp = &composite_multiply;
    else
        comp = &composite_over;

    canvas.composite(artwork, offset_x - x, offset_y - y, img_width, img_height, comp);

    // Now we add the swash, and the operation is done.
    canvas.composite(swash, 0, 0, canvas_dim, canvas_dim, &composite_multiply);

    FILE* result = fopen(argv[12], "wb");
    if (!result) {
        std::cerr << "Failed to open result file " << argv[12] << '\n';
        return 4;
    }
    bool saved = canvas.save(result);
    fclose(result);

    if (!saved) {
        std::cerr << "Failed to encode result file " << argv[12] << '\n';
        return 5;
    }
    else
        return 0;
}

// Syntax:
//           (0)  (1)     (2)            (3)           (4)              (5) (6) (7) (8) (9)
//   mockbot text "Hello" img/canvas.png img/chars.png img/charset.json 100 100 300 120 ing/output.png
//
// (0):  subcommand - always 'text'
// (1):  the text to print onto the image
// (2):  image on which to print the text
// (3):  image containing the letters
// (4):  json document describing where each letter is on (2)
// (5):  x coordinate on (2) of the center of the text
// (6):  y coordinate on (2) of the center of the text
// (7):  width of text region (can be "--" to be any width)
// (8):  height of text region (can be "--" to be any height)
// (9):  file to save the result to
int perform_text(int argc, char** argv) {
    Image canvas;
    FILE* canvas_file = fopen(argv[2], "rb");
    bool success = canvas.load_file(canvas_file);
    if (canvas_file) fclose(canvas_file);

    if (!success) {
        std::cout << "Failed to load canvas\n";
        return 1;
    }

    Image atlas;
    FILE* atlas_file = fopen(argv[3], "rb");
    success = atlas.load_file(atlas_file);
    if (atlas_file) fclose(atlas_file);

    if (!success) {
        std::cout << "Failed to load atlas\n";
        return 1;
    }

    CharacterSet charset;
    FILE* charset_file = fopen(argv[4], "r");
    success = charset.load_json(charset_file);
    if (charset_file) fclose(charset_file);

    if (!success) {
        std::cout << "Failed to load charset JSON\n";
        return 2;
    }

    // const int padding = 5; // Space between each pixel ???
    char* input_string = argv[1];
    const int center_x = int_arg(argv[5]);
    const int center_y = int_arg(argv[6]);
    const int region_width  = int_arg(argv[7]);
    const int region_height = int_arg(argv[8]);

    if (region_width < 0 && region_height < 0) {
        std::cout << "Please specify at least a width or a height\n";
        return 6;
    }

    int total_text_width  = 0;
    int total_text_height = 0;
    // TODO do we need this?
    CharacterOffsets* tallest_offset;

    for (char* c = input_string; *c != '\0'; c++) {
        CharacterOffsets* offsets = charset[*c];

        if (offsets->width >= 0)
            total_text_width += offsets->width;
        if (offsets->height >= 0) {
            if (offsets->height > total_text_height) {
                tallest_offset = offsets;
                total_text_height = offsets->height;
            }
        }
    }

    int actual_region_width, actual_region_height;

    // We take our "total text" width/height and scale it down to fit within the given region
    if (region_width < 0) {
        // Scale total height to match region height
        double total_w_by_h = (double)total_text_width / (double)total_text_height;
        actual_region_height = region_height;
        actual_region_width  = int((double)actual_region_height * total_w_by_h);
        std::cout << "Width not supplied - scaling down by height\n";
    }
    else if (region_height < 0) {
        double total_h_by_w = (double)total_text_height / (double)total_text_width;
        actual_region_width  = region_width;
        actual_region_height = int((double)actual_region_width * total_h_by_w);
        std::cout << "Height not supplied - scaling down by width\n";
    }
    else {
        double total_w_by_h  = (double)total_text_width / (double)total_text_height;
        double region_w_by_h = (double)region_width     / (double)region_height;

        if (total_w_by_h > region_w_by_h) {
            // We need to scale down by width
            actual_region_width  = region_width;
            actual_region_height = int((double)actual_region_width / total_w_by_h);
            std::cout << "Scaling down by width\n";
        }
        else {
            // We scale down by height
            actual_region_height = region_height;
            actual_region_width  = int((double)actual_region_height * total_w_by_h);
            std::cout << "Scaling down by height\n";
        }
    }

    double text_scale = (double)actual_region_width / (double)total_text_width;

    int char_x = center_x - actual_region_width / 2; // This will increment by character width each character
    int char_y = center_y - actual_region_height / 2; // This will stay the same I suppose
    CompositeOver comp;

    for (char* c = input_string; *c != '\0'; c++) {
        CharacterOffsets* offsets = charset[*c];
        if (offsets->x < 0 || offsets->y < 0) {
            continue;
        }
        int char_width  = int((double)offsets->width  * text_scale);
        int char_height = int((double)offsets->height * text_scale);

        canvas.composite(atlas, offsets, char_x, char_y, char_width, char_height, &comp);
        char_x += char_width;
    }

    FILE* output = fopen(argv[9], "wb");
    success = canvas.save(output);
    fclose(output);
    if (!success) {
        std::cerr << "Couldn't save to " << output << '\n';
        return 1;
    }

    return 0;
}

// We'll make this print a single letter onto the center of a blank image I guess
int perform_letter(int argc, char** argv) {
    CompositeOver comp;

    CharacterSet charset;
    FILE* json_file = fopen("test/charset.json", "r");
    if (!json_file) {
        std::cerr << "no json\n";
        return 2;
    }

    bool loaded_successfully = charset.load_json(json_file);
    fclose(json_file);

    if (!loaded_successfully) {
        std::cerr << "invalid characterset JSON\n";
        return 3;
    }

    Image atlas;
    FILE* image_file = fopen("test/charset.png", "rb");
    if (!image_file) {
        std::cerr << "no image\n";
        return 2;
    }

    loaded_successfully = atlas.load_file(image_file);
    fclose(image_file);

    if (!loaded_successfully) {
        std::cerr << "failed to load test/charset.png\n";
        return 4;
    }

    Image canvas;
    canvas.fill_blank("#AAAAAA", 600, 600);

    auto offsets = charset['a'];
    if (offsets->x < 0 || offsets->y < 0) {
        std::cerr << "bad offset\n";
        return 7;
    }
    std::cout << "x: " << offsets->x << " y: " << offsets->y << '\n';
    if (!canvas.composite(atlas, offsets, 50, 50, offsets->width, offsets->height, &comp)) {
        std::cerr << "failed to composite\n";
        return 5;
    }

    FILE* outFile = fopen("test/letter-result.png", "wb");
    bool saved = canvas.save(outFile);
    fclose(outFile);

    if (!saved) {
        std::cerr << "failed to encode\n";
        return 6;
    }

    return 0;
};

int main(int argc, char** argv) {
    const char* subcommand = NULL;

    if (argc <= 1)
        goto bad_subcommand;

    subcommand = argv[1];

    if      (cstr_eq(subcommand, "composite")) return perform_composite(argc - 1, argv + 1);
    else if (cstr_eq(subcommand, "thumbnail")) return perform_thumbnail(argc - 1, argv + 1);
    else if (cstr_eq(subcommand, "text"))      return perform_text(argc - 1, argv + 1);
    // NOTE "letter" is just for testing
    else if (cstr_eq(subcommand, "letter"))    return perform_letter(argc - 1, argv + 1);

bad_subcommand:
    std::cerr << "Invalid subcommand \"" << subcommand << "\"\n";
    return 1;
}
