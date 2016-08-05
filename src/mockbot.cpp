#include <string>
#include <iostream>
#include "image.h"
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <memory>

using namespace mockbot;

bool magick_has_been_initialized = false;
static void init_magick() {
    if (!magick_has_been_initialized) {
        Magick::InitializeMagick(NULL);
        magick_has_been_initialized = true;
    }
}

static bool cstr_eq(const char* s1, const char* s2) {
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

static int positive_int_arg(char* arg) {
    if (cstr_eq(arg, "--"))
        return -1;
    else
        return atoi(arg);
}

CompositeOver     composite_over;
CompositeMultiply composite_multiply;
Compositor*       last_compositor;

std::unique_ptr<Image>        letter_atlas;
std::unique_ptr<CharacterSet> charset;
std::unique_ptr<Image>        last_canvas;

Compositor* load_compositor(char* type) {
    if (cstr_eq(type, "over"))
        last_compositor = &composite_over;
    else if (cstr_eq(type, "multiply"))
        last_compositor = &composite_multiply;

    return last_compositor;
}

Image* load_letter_atlas(char* filename) {
    if (cstr_eq(filename, "--"))
        return letter_atlas.get();

    letter_atlas.reset(new Image);
    FILE* file = fopen(filename, "rb");
    bool success = letter_atlas->load_file(file);
    if (file) fclose(file);

    if (!success)
        return NULL;

    return letter_atlas.get();
}

CharacterSet* load_charset(char* filename) {
    if (cstr_eq(filename, "--"))
        return charset.get();

    charset.reset(new CharacterSet);
    FILE* file = fopen(filename, "r");
    bool success = charset->load_json(file);
    if (file) fclose(file);

    if (!success)
        return NULL;

    return charset.get();
}

Image* load_canvas(char* filename) {
    if (cstr_eq(filename, "--"))
        return last_canvas.get();

    last_canvas.reset(new Image);
    FILE* file = fopen(filename, "rb");
    bool success = last_canvas->load_file(file);
    if (file) fclose(file);

    if (!success)
        return NULL;

    return last_canvas.get();
}

int perform_composite(int argc, char** argv, int* args_used) {
    *args_used = 9;

    // If '--' is passed for img2, we have img2 point to the previously used canvas,
    // otherwise, we load img2 right here and need it to be disposed at the end of the
    // function, so we have it point to this local variable in that case.
    Image local_img1;
    Image local_img2;

    int x = std::stoi(std::string(argv[3]));
    int y = std::stoi(std::string(argv[4]));
    int w = std::stoi(std::string(argv[5]));
    int h = std::stoi(std::string(argv[6]));

    // TODO please clean up the code for laoding the images here it's awful

    Image* img2;
    if (cstr_eq(argv[2], "--")) {
        if (cstr_eq(argv[1], "--")) {
            std::cerr << "Can't use last canvas for both images in a composite\n";
            return 10;
        }

        img2 = last_canvas.get();
    }
    else {
        img2 = &local_img2;

        FILE* f2 = fopen(argv[2], "rb");
        bool success = img2->load_file(f2);
        if (f2) fclose(f2);
        if (!success) {
            std::cerr << "Failed decode img2: " << argv[2] << '\n';
            return 2;
        }
    }
    if (!img2) {
        std::cerr << "Failed to acquire img2 from " << argv[2] << '\n';
        return 2;
    }

    Image* img1;
    if (cstr_eq(argv[1], "--")) {
        img1 = load_canvas(argv[1]);
    }
    else {
        img1 = &local_img1;

        FILE* f1 = fopen(argv[1], "rb");
        bool success = img1->load_file(f1);
        if (f1) fclose(f1);
        if (!success) {
            std::cerr << "Failed decode img1: " << argv[1] << '\n';
            return 2;
        }
    }
    if (!img1) {
        std::cerr << "Failed acquire img1: " << argv[1] << '\n';
        return 2;
    }

    img2->make_background_transparent();

    CompositeOver over;
    CompositeMultiply multiply;
    Compositor* comp;

    std::string operation = argv[7];
    if (operation == "multiply")
        comp = &multiply;
    else
        comp = &over;

    if (!img1->composite(*img2, x, y, w, h, comp)) {
        std::cerr << "Failed to composite " << argv[2] << " onto " << argv[1] << '\n';
        return 2;
    }

    if (!cstr_eq(argv[8], "--")) {
        FILE* f3 = fopen(argv[8], "wb");
        if (!f3) {
            std::cerr << "Couldn't reopen " << argv[9] << " for writing\n";
            return 2;
        }

        if (!img1->save(f3)) {
            std::cerr << "Failed to encode result to " << argv[9] << '\n';
            return 2;
        }
        fclose(f3);
    }
    return 0;
}

// Syntax:
//           (0)        (1)     (2)             (3)             (4)           (5) (6) (7)   (8)  (9) (10) (11) (12)
//   mockbot thumbnail  #00FF00 img/heather.png img/artwork.png img/swash.png 500 10  over  1081 501 2265 3513 img/result.png
//
// (0):  subcommand - always 'thumbnail'
// (1):  background color, set to "--" to use the first pixel of the artwork image
// (2):  heather file path, set to "--" to not use heather
// (3):  artwork file path, set to "--" to use the canvas from the previous command
// (4):  swash file path
// (5):  canvas width and height (it's always square)
// (6):  padding
// (7):  artwork composition method
// (8):  artwork x (rel. to the image's topleft)
// (9):  artwork y
// (10): artwork width (within its image - not the image dimensions)
// (11): artwork height
// (12): output file name
int perform_thumbnail(int argc, char** argv, int* args_used) {
    *args_used = 13;
    if (argc < 13) {
        std::cerr << "Expected exactly 12 arguments after 'thumbnail'\n";
        return 1;
    }

    CompositeOver     composite_over;
    CompositeMultiply composite_multiply;

    Image canvas;
    Image* artwork;
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

    artwork = load_canvas(argv[3]);
    if (!artwork) {
        std::cerr << "Failed to load " << argv[3] << '\n';
        return 4;
    }
    LOAD_IMAGE(swash, argv[4]);

    if (color_hexcode == "--")
        canvas.fill_blank(artwork->pixel(0, 0), canvas_dim, canvas_dim);
    else
        canvas.fill_blank(color_hexcode, canvas_dim, canvas_dim);

    artwork->make_background_transparent();

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

    img_width  = (width  / art_width)  * double(artwork->width);
    img_height = (height / art_height) * double(artwork->height);

    // Center gravity
    double offset_x = (canvas_width - width) / 2.0;
    double offset_y = (canvas_height - height) / 2.0;

    Compositor* comp;
    std::string operation = argv[7];
    if (operation == "multiply")
        comp = &composite_multiply;
    else
        comp = &composite_over;

    canvas.composite(*artwork, offset_x - x, offset_y - y, img_width, img_height, comp);

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
//           (0)  (1)     (2)            (3)           (4)              (5) (6) (7) (8) (9)  (10)
//   mockbot text "Hello" img/canvas.png img/chars.png img/charset.json 100 100 300 120 over img/output.png
//
// (0):  subcommand - always 'text'
// (1):  the text to print onto the image
// (2):  image on which to print the text
// (3):  image containing the letters or "--" for the last used letter atlas
// (4):  json document describing where each letter is on (2) or "--" for the last used character set
// (5):  x coordinate on (2) of the center of the text
// (6):  y coordinate on (2) of the center of the text
// (7):  width of text region (can be "--" to be any width)
// (8):  height of text region (can be "--" to be any height)
// (9):  composite method
// (10): file to save the result to
int perform_text(int argc, char** argv, int* args_used) {
    *args_used = 11;

    Image* canvas = load_canvas(argv[2]);
    if (!canvas) {
        std::cout << "Failed to load canvas at " << argv[2] << '\n';
        return 1;
    }

    Image* atlas = load_letter_atlas(argv[3]);
    if (!atlas) {
        std::cout << "Failed to load letter atlas " << argv[3] << '\n';
        return 1;
    }

    CharacterSet* charset = load_charset(argv[4]);
    if (!charset) {
        std::cout << "Failed to load charset JSON " << argv[4] << '\n';
        return 2;
    }

    // const int padding = 5; // Space between each letter ???
    char* input_string = argv[1];
    const int region_x = positive_int_arg(argv[5]);
    const int region_y = positive_int_arg(argv[6]);
    const int region_width  = positive_int_arg(argv[7]);
    const int region_height = positive_int_arg(argv[8]);

    if (region_width < 0 && region_height < 0) {
        std::cout << "Please specify at least a width or a height\n";
        return 6;
    }

    int total_text_width;
    int total_text_height;
    charset->get_dimensions(input_string, &total_text_width, &total_text_height);

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

    int char_x = region_x; // This will increment by character width each character
    int char_y = region_y; // This will stay the same I suppose

    Compositor* comp = load_compositor(argv[9]);
    for (char* c = input_string; *c != '\0'; c++) {
        CharacterOffsets* offsets = (*charset)[*c];
        if (offsets->x < 0 || offsets->y < 0) {
            continue;
        }
        int char_width  = int((double)offsets->width  * text_scale);
        int char_height = int((double)offsets->height * text_scale);

        canvas->composite(*atlas, offsets, char_x, char_y, char_width, char_height, comp);
        char_x += char_width;
    }

    if (!cstr_eq(argv[10], "--")) {
        FILE* output = fopen(argv[10], "wb");
        bool success = canvas->save(output);
        fclose(output);
        if (!success) {
            std::cerr << "Couldn't save to " << output << '\n';
            return 1;
        }
    }

    return 0;
}

#define DEGREES(x) (double(x) * (M_PI / 180.0))

// Syntax:
//           (0)     (1)     (2)            (3)           (4)              (5) (6)  (7) (8) (9)  (10)
//   mockbot arctext "Hello" img/canvas.png img/chars.png img/charset.json 100 100  200 110 over img/output.png
//
// (0):  subcommand - always 'arctext'
// (1):  the text to print onto the image
// (2):  image on which to print the text
// (3):  image containing the letters
// (4):  json document describing where each letter is on (2)
// (5):  x coordinate of circle center (or -- for center of image)
// (6):  y coordinate of circle center (or -- for center of image)
// (7):  radius of circle
// (8):  max height of text
// (9):  composition method ("over" or "multiply")
// (10): output file
int perform_arctext(int argc, char** argv, int* args_used) {
    *args_used = 11;
    init_magick();

    char* input_string = argv[1];

    Image* atlas = load_letter_atlas(argv[3]);
    if (!atlas) {
        std::cout << "Failed to load letter atlas\n";
        return 1;
    }

    CharacterSet* charset = load_charset(argv[4]);
    if (!charset) {
        std::cout << "Failed to load charset JSON\n";
        return 2;
    }

    Image* canvas = load_canvas(argv[2]);
    if (!canvas) {
        std::cerr << "Failed to load canvas at " << argv[2] << '\n';
        return 3;
    }

    int center_x    = atoi(argv[5]);
    int center_y    = atoi(argv[6]);
    int radius      = atoi(argv[7]);
    int text_height = atoi(argv[8]);

    int total_text_width;
    int total_text_height;
    int char_count;
    charset->get_dimensions(input_string, &total_text_width, &total_text_height, &char_count);
    double text_scale = (double)text_height / (double)total_text_height;

    int actual_text_width = int((double)total_text_width * text_scale);
    int actual_text_height = text_height;

    double actual_text_angle = (double)actual_text_width / (double)radius;

    double start_angle = (DEGREES(180) - actual_text_angle) / 2.0;
    double avg_angle_per_char = actual_text_angle / double(char_count);

    double char_angle = DEGREES(180) + start_angle + avg_angle_per_char / 2.0;

    Compositor* comp = load_compositor(argv[9]);
    for (char* c = input_string; *c != '\0'; c++) {
        CharacterOffsets* offsets = (*charset)[*c];
        Image letter = atlas->rotated(offsets, char_angle);

        int char_width  = int((double)letter.width  * text_scale);
        int char_height = int((double)letter.height * text_scale);

        int char_x = int(double(radius) * cos(char_angle))
          + center_x
          - char_width / 2;  // origin of char at letter center
        int char_y = int(double(radius) * sin(char_angle))
          + center_y
          - char_height / 2;

        canvas->composite(letter, char_x, char_y, char_width, char_height, comp);
        char_angle += (double)offsets->width * text_scale / (double)radius;
    }

    if (!cstr_eq(argv[10], "--")) {
        FILE* output_file = fopen(argv[10], "wb");
        bool success = canvas->save(output_file);
        if (output_file) fclose(output_file);
        if (!success) {
            std::cerr << "Failed to save!!!\n";
            return 2;
        }
    }

    return 0;
}

// Syntax:
//           (0)  (1)     (2)            (3)             (4)     (5)     (6) (7) (8) (9) (10) (11) (12)
//   mockbot text "Hello" img/canvas.png @font/ariel.ttf #FFFFFF #000000 2.5 120 120 700 100  over img/output.png
//
// (0):  subcommand - always 'ftext'
// (1):  the text to print onto the image
// (2):  image on which to print the text
// (3):  font to use
// (4):  font color
// (5):  font outline color or "--" for no outline
// (6):  font outline width
// (7):  rectangle x
// (8):  rectangle y
// (9):  rectangle width
// (10): rectangle height
// (11): composition method
// (12): output file
int perform_ftext(int argc, char** argv, int* args_used) {
    using Magick::Quantum;
    using Magick::Color;
    using Magick::Geometry;
    using Magick::PixelPacket;

    *args_used = 13;
    init_magick();

    char* input_string = argv[1];

    Image* canvas = load_canvas(argv[2]);
    if (!canvas) {
        std::cerr << "Failed to load canvas at " << argv[2] << '\n';
        return 3;
    }

    int dest_x      = atoi(argv[7]);
    int dest_y      = atoi(argv[8]);
    int dest_width  = atoi(argv[9]);
    int dest_height = atoi(argv[10]);

    auto transparent = Magick::Color(0, 0, 0, MaxRGB);
    Magick::Image text_magick(Geometry(0, 0), transparent);
    text_magick.magick("png");
    text_magick.font(argv[3]);
    text_magick.fontPointsize(dest_height + dest_height / 2);

    Magick::TypeMetric metrics;
    text_magick.fontTypeMetrics(input_string, &metrics);
    text_magick.extent(Geometry(metrics.textWidth(), metrics.textHeight()), transparent);

    auto color = Image::magick_color(argv[4]);
    text_magick.fillColor(color);

    bool overdraw;
    if (cstr_eq(argv[5], "--")) {
        text_magick.strokeColor(color);
        overdraw = false;
    }
    else {
        text_magick.strokeColor(Image::magick_color(argv[5]));
        text_magick.strokeWidth(atof(argv[6]));
        overdraw = true;
    }

    text_magick.annotate(input_string, Magick::NorthWestGravity);
    if (overdraw) {
        text_magick.strokeColor(color);
        text_magick.strokeWidth(0.5);
        text_magick.annotate(input_string, Magick::NorthWestGravity);
    }

    int text_width  = 0;
    int text_height = 0;

    // Find the actual width and height of the text on the image
    {
        Magick::Pixels pixel_view(text_magick);
        int textimg_width  = text_magick.columns();
        int textimg_height = text_magick.rows();

        PixelPacket* text_pixel = pixel_view.get(0, 0, textimg_width, textimg_height);
        int area = textimg_width * textimg_height;
        for (int i = 0; i < area; i++)
            ++text_pixel;

        for (int y = textimg_height - 1; y >= 0; y--) {
            for (int x = textimg_width - 1; x >= 0; x--) {
                --text_pixel;

                if (text_pixel->opacity != MaxRGB) {
                    int w = x + 1;
                    int h = y + 1;
                    if (w > text_width)
                        text_width = w;
                    if (h > text_height)
                        text_height = h;
                }
            }
        }
    }

    std::cout << "Text turned out to be " << text_width << 'x' << text_height << '\n';

    Image text_image(text_magick);

    Compositor* comp = load_compositor(argv[11]);
    canvas->composite(text_image, 0, 0, text_width, text_height, dest_x, dest_y, dest_width, dest_height, comp);

    if (!cstr_eq(argv[12], "--")) {
        FILE* output_file = fopen(argv[12], "wb");
        bool success = canvas->save(output_file);
        if (output_file) fclose(output_file);
        if (!success) {
            std::cerr << "Failed to save!!!\n";
            return 2;
        }
    }

    return 0;
}

// We'll make this print a single letter onto the center of a blank image I guess
int perform_letter(int argc, char** argv, int* args_used) {
    *args_used = 0;
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

bool run(int* pargc, char*** pargv) {
    if ((*pargc) < 1)
        return false;

    const char* subcommand = (*pargv)[0];
    int return_code;
    int args_used;

    if      (cstr_eq(subcommand, "composite")) return_code = perform_composite((*pargc), (*pargv), &args_used);
    else if (cstr_eq(subcommand, "thumbnail")) return_code = perform_thumbnail((*pargc), (*pargv), &args_used);
    else if (cstr_eq(subcommand, "text"))      return_code = perform_text((*pargc), (*pargv), &args_used);
    else if (cstr_eq(subcommand, "ftext"))     return_code = perform_ftext((*pargc), (*pargv), &args_used);
    else if (cstr_eq(subcommand, "arctext"))   return_code = perform_arctext((*pargc), (*pargv), &args_used);
    else {
        std::cerr << "Invalid subcommand \"" << subcommand << "\"\n";
        return false;
    }

    if (return_code != 0) {
        std::cerr << "Subcommand \"" << subcommand << "\" returned " << return_code << '\n';
        return false;
    }

    *pargc -= args_used;
    *pargv += args_used;

    return true;
}

int main(int argc, char** argv) {
    // First arg is application path
    argc -= 1;
    argv += 1;
    while (run(&argc, &argv)) {}

    return 0;
}
