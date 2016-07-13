#include <string>
#include <iostream>
#include "image.h"
#include "stdio.h"

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

int main(int argc, char** argv) {
    const char* subcommand = NULL;

    if (argc <= 1)
        goto bad_subcommand;

    subcommand = argv[1];

    if      (cstr_eq(subcommand, "composite")) return perform_composite(argc - 1, argv + 1);
    else if (cstr_eq(subcommand, "thumbnail")) return perform_thumbnail(argc - 1, argv + 1);

bad_subcommand:
    std::cerr << "Please specify a subcommand: \"composite\" or \"thumbnail\"\n";
    return 1;
}
