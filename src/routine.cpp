#include "routine.h"
#include <list>
#define DEGREES(x) (double(x) * (M_PI / 180.0))

namespace mockbot {
//
// =============== ROUTINES ==============
//

class CompositeRoutine : public Routine {
public:
    const char* help_text() const {
        return
            "Syntax:\n"\
            "              (0)       (1)                (2)             (3) (4) (5) (6) (7)      (8) (9)\n"\
            "  bin/mockbot composite img/blankshirt.png img/artwork.png 409 192 665 760 multiply 150 test/img/out1.png\n"\
            "\n"\
            "(0): subcommand - always 'composite'\n"\
            "(1): background image, or \"--\" to use the previous command's canvas\n"\
            "(2): foreground image, or \"--\" to use the previous command's canvas\n"\
            "(3): foreground x\n"\
            "(4): foreground y\n"\
            "(5): foreground width\n"\
            "(6): foreground height\n"\
            "(7): composition method\n"\
            "(8): dpi or \"--\" to use 150\n"\
            "(9): output file name\n";
    };

    int args_used() const { return 10; }

    int perform(Session &session, char** argv) {
        // If '--' is passed for img2, we have img2 point to the previously used canvas,
        // otherwise, we load img2 right here and need it to be disposed at the end of the
        // function, so we have it point to this local variable in that case.
        Image local_img1;
        Image local_img2;
 
        int x = std::stoi(std::string(argv[3]));
        int y = std::stoi(std::string(argv[4]));
        int w = std::stoi(std::string(argv[5]));
        int h = std::stoi(std::string(argv[6]));

        // Check DPI for images
        if (!cstr_eq(argv[8], "--")) {
            int dpi = std::stoi(std::string(argv[8]));
            local_img1.set_dpi(dpi);
            local_img2.set_dpi(dpi);
        }
        
         
        // TODO please clean up the code for loading the images here it's awful
  
        Image* img2;
        if (cstr_eq(argv[2], "--")) {
            if (cstr_eq(argv[1], "--")) {
                std::cerr << "Can't use last canvas for both images in a composite\n";
                return 10;
            }

            img2 = session.get_last_canvas();
        }
        else {
            img2 = &local_img2;

            FILE* f2 = fopen(argv[2], "rb");
            bool success = img2->load_file(f2);
            if (f2) fclose(f2);
            if (!success) {
                std::cerr << "Failed decode img2: " << argv[2] << ": " << img2->last_error() << '\n';
                return 2;
            }
        }
        if (!img2) {
            std::cerr << "Failed to acquire img2 from " << argv[2] << '\n';
            return 2;
        }

        Image* img1;
        if (cstr_eq(argv[1], "--")) {
            img1 = session.load_canvas(argv[1]);
        }
        else {
            img1 = &local_img1;

            FILE* f1 = fopen(argv[1], "rb");
            bool success = img1->load_file(f1);
            if (f1) fclose(f1);
            if (!success) {
                std::cerr << "Failed decode img1: " << argv[1] << ": " << img1->last_error() << '\n';
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

        if (!cstr_eq(argv[9], "--")) {
            FILE* f3 = fopen(argv[9], "wb");
            if (!f3) {
                std::cerr << "Couldn't reopen " << argv[10] << " for writing\n";
                return 2;
            }

            if (!img1->save(f3)) {
                std::cerr << "Failed to encode result to " << argv[10] << '\n';
                return 2;
            }
            fclose(f3);
        }
        return 0;
    }
};

class ThumbnailRoutine : public Routine {
public:
    const char* help_text() const {
        return
            "Syntax:\n"\
            "          (0)        (1)     (2)             (3)             (4)           (5) (6) (7)   (8)  (9) (10) (11) (12) (13)\n"\
            "  mockbot thumbnail  #00FF00 img/heather.png img/artwork.png img/swash.png 500 10  over  1081 501 2265 3513 150  img/result.png\n"\
            "\n"\
            "(0):  subcommand - always 'thumbnail'\n"\
            "(1):  background color, set to \"--\" to use the first pixel of the artwork image\n"\
            "(2):  heather file path, set to \"--\" to not use heather\n"\
            "(3):  artwork file path, set to \"--\" to use the canvas from the previous command\n"\
            "(4):  swash file path\n"\
            "(5):  canvas width and height (it's always square)\n"\
            "(6):  padding\n"\
            "(7):  artwork composition method\n"\
            "(8):  artwork x (rel. to the image's topleft)\n"\
            "(9):  artwork y\n"\
            "(10): artwork width (within its image - not the image dimensions)\n"\
            "(11): artwork height\n"\
            "(12): dpi or \"--\" to use 150\n"\
            "(13): output file name\n";
    }

    int args_used() const { return 14; }

    int perform(Session &session, char** argv) {
        Image canvas;
        Image* artwork;
        Image swash;
        Image heather;

        std::string color_hexcode = argv[1];

        int canvas_dim = std::stoi(std::string(argv[5]));
        int padding    = std::stoi(std::string(argv[6]));

        // Check DPI for images
        if (!cstr_eq(argv[8], "--")) {
            int dpi = std::stoi(std::string(argv[12]));
            canvas.set_dpi(dpi);
            artwork->set_dpi(dpi);
            swash.set_dpi(dpi);
            heather.set_dpi(dpi);
        }
        // Common routine to load an image with error checking.
#define LOAD_IMAGE(image, filename)                                                                                       \
    do {                                                                                                                       \
        FILE* f = fopen(filename, "rb");                                                                                        \
        if (!f) { std::cerr << "Failed to open file " << (filename) << '\n'; return 2; }                                         \
        bool did_load = (image).load_file(f);                                                                                     \
        fclose(f);                                                                                                                 \
        if (!did_load) { std::cerr << "Failed to decode file " << (filename) << " -- " << (image).last_error() << '\n'; return 3; } \
    } while (0)

        artwork = session.load_canvas(argv[3]);
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
            canvas.composite(heather, 0, 0, canvas_dim, canvas_dim, &session.composite_multiply);
        }

        // Here we calculate the "cropped" dimensions and offsets for the artwork.
        double canvas_width  = (double)canvas_dim;
        double canvas_height = (double)canvas_dim;
        double art_x      = (double)std::stoi(std::string(argv[8])); // art_* variables are in fullsize-art-image space
        double art_y      = (double)std::stoi(std::string(argv[9]));
        double art_width  = (double)std::stoi(std::string(argv[10]));
        double art_height = (double)std::stoi(std::string(argv[11]));
        double dpadding   = (double)padding;
        double target_box = (double)canvas_dim - (2.0 * dpadding);
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
            comp = &session.composite_multiply;
        else
            comp = &session.composite_over;

        canvas.composite(*artwork, offset_x - x, offset_y - y, img_width, img_height, comp);

        // Now we add the swash, and the operation is done.
        canvas.composite(swash, 0, 0, canvas_dim, canvas_dim, &session.composite_multiply);

        FILE* result = fopen(argv[13], "wb");
        if (!result) {
            std::cerr << "Failed to open result file " << argv[13] << '\n';
            return 4;
        }
        bool saved = canvas.save(result);
        fclose(result);

        if (!saved) {
            std::cerr << "Failed to encode result file " << argv[13] << '\n';
            return 5;
        }
        else
            return 0;
    }
};


class TextRoutine : public Routine {
public:
    const char* help_text() const {
        return
            "Syntax:\n"\
            "          (0)  (1)     (2)            (3)           (4)              (5) (6) (7) (8) (9)  (10)\n"\
            "  mockbot text \"Hello\" img/canvas.png img/chars.png img/charset.json 100 100 300 120 over img/output.png\n"\
            "\n"\
            "(0):  subcommand - always 'text'\n"\
            "(1):  the text to print onto the image\n"\
            "(2):  image on which to print the text\n"\
            "(3):  image containing the letters or \"--\" for the last used letter atlas\n"\
            "(4):  json document describing where each letter is on (2) or \"--\" for the last used character set\n"\
            "(5):  x coordinate on (2) of the center of the text\n"\
            "(6):  y coordinate on (2) of the center of the text\n"\
            "(7):  width of text region (can be \"--\" to be any width)\n"\
            "(8):  height of text region (can be \"--\" to be any height)\n"\
            "(9):  composite method\n"\
            "(10): file to save the result to\n";
    }

    int args_used() const { return 11; }

    int perform(Session &session, char** argv) {
        Image* canvas = session.load_canvas(argv[2]);
        if (!canvas) {
            std::cerr << "Failed to load canvas at " << argv[2] << '\n';
            return 1;
        }

        Image* atlas = session.load_letter_atlas(argv[3]);
        if (!atlas) {
            std::cerr << "Failed to load letter atlas " << argv[3] << '\n';
            return 1;
        }

        CharacterSet* charset = session.load_charset(argv[4]);
        if (!charset) {
            std::cerr << "Failed to load charset JSON " << argv[4] << '\n';
            return 2;
        }

        char* input_string = argv[1];
        const int region_x = positive_int_arg(argv[5]);
        const int region_y = positive_int_arg(argv[6]);
        const int region_width  = positive_int_arg(argv[7]);
        const int region_height = positive_int_arg(argv[8]);

        if (region_width < 0 && region_height < 0) {
            std::cerr << "Please specify at least a width or a height\n";
            return 6;
        }

        int total_text_width;
        int total_text_height;
        charset->get_dimensions(Linear, input_string, &total_text_width, &total_text_height);

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

        Compositor* comp = session.load_compositor(argv[9]);

        char* letter = input_string;

        while (*letter != '\0') {
            uint32_t codepoint = utf8::unchecked::next(letter);
            CharacterOffsets* offsets = (*charset)[codepoint];

            if (offsets->x < 0 || offsets->y < 0) {
                continue;
            }
            int char_width  = int((double)offsets->width  * text_scale);
            int char_height = int((double)offsets->height * text_scale);

            char_x += offsets->l_pad;
            canvas->composite(*atlas, offsets, char_x, char_y, char_width, char_height, comp);
            char_x += char_width + offsets->r_pad;
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
};

class ArctextRoutine : public Routine {
public:
    const char* help_text() const {
        return
            "Syntax:\n"\
            "          (0)     (1)     (2)            (3)           (4)              (5) (6)  (7)  (8)(9) (10) (11)\n"\
            "  mockbot arctext \"Hello\" img/canvas.png img/chars.png img/charset.json 100 100  200  30 110 over img/output.png\n"\
            "\n"\
            "(0):  subcommand - always 'arctext'\n"\
            "(1):  the text to print onto the image\n"\
            "(2):  image on which to print the text\n"\
            "(3):  image containing the letters\n"\
            "(4):  json document describing where each letter is on (2)\n"\
            "(5):  x coordinate of circle center (or -- for center of image)\n"\
            "(6):  y coordinate of circle center (or -- for center of image)\n"\
            "(7):  radius of circle\n"\
            "(8):  min height of text\n"\
            "(9):  max height of text\n"\
            "(10): composition method (\"over\" or \"multiply\")\n"\
            "(11): output file\n";
    }

    int args_used() const { return 12; }

    int perform(Session &session, char** argv) {
        session.init_magick();

        char* input_string = argv[1];

        Image* atlas = session.load_letter_atlas(argv[3]);
        if (!atlas) {
            std::cerr << "Failed to load letter atlas\n";
            return 1;
        }

        CharacterSet* charset = session.load_charset(argv[4]);
        if (!charset) {
            std::cerr << "Failed to load charset JSON\n";
            return 2;
        }

        Image* canvas = session.load_canvas(argv[2]);
        if (!canvas) {
            std::cerr << "Failed to load canvas at " << argv[2] << '\n';
            return 3;
        }

        int center_x        = atoi(argv[5]);
        int center_y        = atoi(argv[6]);
        int radius          = atoi(argv[7]);
        int text_min_height = atoi(argv[8]);
        int text_height     = atoi(argv[9]);

        int total_text_width;
        int total_text_height;
        int char_count;
        charset->get_dimensions(Angular, input_string, &total_text_width, &total_text_height, &char_count);

        bool retrying = false;
        uint32_t first_codepoint;
        uint32_t last_codepoint = 0;
        {
            char* letter = input_string;
            first_codepoint = utf8::unchecked::next(letter);
            while (*letter != '\0')
                last_codepoint = utf8::unchecked::next(letter);
        }

#define ANGLE(x) (((double)(x)) * (text_scale / (double)radius))
        double text_scale; // text_scale defined here so we can capture its reference

        // === Functions for changing the text height if letters are clipping off the image ===

        // NOTE all the various 90° angle offsets in here have to do with the way Magick++ rotates.

        std::function<int(CharacterOffsets* offsets, vec2, vec2, double)> check_rotated = [&text_height, radius, center_x, &text_scale](CharacterOffsets* offsets, vec2 point1, vec2 point2, double theta) {
            vec2 center(offsets->width / 2.0, offsets->height / 2.0);
            point1.x -= center.x;
            point1.y -= center.y;
            point2.x -= center.x;
            point2.y -= center.y;

            vec2 rotated1(
                point1.x*cos(theta) - point1.y*sin(theta),
                point1.x*sin(theta) + point1.y*cos(theta)
            );

            vec2 rotated2(
                point2.x*cos(theta) - point2.y*sin(theta),
                point2.x*sin(theta) + point2.y*cos(theta)
            );

            point1.x = rotated1.x + center.x;
            point2.x = rotated2.x + center.x;

            double char_width = fmax(offsets->width, point2.x - point1.x) * text_scale;

            int char_x = int(double(radius) * cos(theta - DEGREES(90)))
                + center_x
                - char_width / 2;  // origin of char at glyph center

            return char_x;
        };

        std::function<int(double, double)> check_first = [charset, first_codepoint, radius, &check_rotated, &text_scale](double char_angle, double actual_text_angle) {
            CharacterOffsets* offsets = (*charset)[first_codepoint];

            vec2 point1(0, 0);
            vec2 point2(offsets->width, offsets->height);

            double theta = char_angle + DEGREES(90) + ANGLE(offsets->width / 2 + offsets->ar_pad);

            return check_rotated(offsets, point1, point2, theta);
        };

        std::function<int(double, double)> check_last = [charset, last_codepoint, radius, &check_rotated, &text_scale](double char_angle, double actual_text_angle) {
            CharacterOffsets* offsets = (*charset)[last_codepoint];

            vec2 point1(offsets->width, 0);
            vec2 point2(0, offsets->height);

            double theta = char_angle + actual_text_angle + DEGREES(90) + ANGLE(offsets->width + offsets->al_pad);

            return check_rotated(offsets, point1, point2, theta);
        };

        std::function<int(double, double)>* checker = nullptr;

    ApplyScale:;
        if (retrying && text_height < text_min_height) {
            std::cerr << "text \"" << input_string << "\" is too large to fit the minimum height of " << text_min_height << "px\n";
            return 10;
        }

        /*if (retrying)
            std::cout << "Trying height = " << text_height << "\n";*/

        text_scale = (double)text_height / (double)total_text_height;

        int actual_text_width = int((double)total_text_width * text_scale);
        int actual_text_height = text_height;

        double actual_text_angle = (double)actual_text_width / (double)radius;

        double start_angle = (DEGREES(180) - actual_text_angle) / 2.0;

        double char_angle = DEGREES(180) + start_angle;

        Compositor* comp = session.load_compositor(argv[10]);

        // Only perform overflow check if a min_height > 0 was specified -- otherwise it's assumed
        // we can just let the overflow happen.
        if (text_min_height > 0) {
            int char_x;

            if (checker == nullptr) {
                if (last_codepoint == 0) {
                    checker = &check_first;
                }
                else {
                    // Check both ends -- whichever is further to off the edge will be checked for the rest of the operation.
                    int left_x  = check_first(char_angle, actual_text_angle);
                    int right_x = check_last(char_angle, actual_text_angle);
                    int left_diff  = 0 - left_x;
                    int right_diff = right_x - canvas->width;

                    if (left_diff > 0 && left_diff > right_diff) {
                        checker = &check_first;
                        char_x = left_x;
                    }
                    if (right_diff > 0 && right_diff > left_diff) {
                        checker = &check_last;
                        char_x = right_x;
                    }

                    // Skip running the checker since we already have char_x assigned.
                    goto CheckOverflow;
                }
            }

            char_x = (*checker)(char_angle, actual_text_angle);

        CheckOverflow:;
            if (char_x < 0 || char_x >= canvas->width) {
                text_height -= 1;
                retrying = true;
                goto ApplyScale;
            }
        }

        bool printed_actual = false;
        char* letter = input_string;
        while (*letter != '\0') {
            uint32_t codepoint = utf8::unchecked::next(letter);
            CharacterOffsets* offsets = (*charset)[codepoint];

            char_angle += ANGLE(offsets->width / 2 + offsets->al_pad);

            Image glyph = atlas->rotated(offsets, char_angle);

            int char_width  = int((double)glyph.width  * text_scale);
            int char_height = int((double)glyph.height * text_scale);

            int char_x = int(double(radius) * cos(char_angle))
                + center_x
                - char_width / 2;  // origin of char at glyph center
            int char_y = int(double(radius) * sin(char_angle))
                + center_y
                - char_height / 2;

            if (!printed_actual) {
                printed_actual = true;
            }

            canvas->composite(glyph, char_x, char_y, char_width, char_height, comp);
            char_angle += ANGLE(offsets->width / 2 + offsets->ar_pad);
#undef ANGLE
        }

        if (!cstr_eq(argv[11], "--")) {
            FILE* output_file = fopen(argv[11], "wb");
            bool success = canvas->save(output_file);
            if (output_file) fclose(output_file);
            if (!success) {
                std::cerr << "Failed to save!!!\n";
                return 2;
            }
        }

        return 0;
    }
};


class FTextRoutine : public Routine {
public:
    const char* help_text() const {
        return
            "Syntax:\n"\
            "          (0)   (1)     (2)            (3)             (4)     (5)     (6) (7) (8) (9) (10) (11) (12) (13)\n"\
            "  mockbot ftext \"Hello\" img/canvas.png @font/ariel.ttf #FFFFFF #000000 2.5 0   120 120 700  100  over img/output.png\n"\
            "\n"\
            "(0):  subcommand - always 'ftext'\n"\
            "(1):  the text to print onto the image\n"\
            "(2):  image on which to print the text\n"\
            "(3):  font to use\n"\
            "(4):  font color\n"\
            "(5):  font outline color or \"--\" for no outline\n"\
            "(6):  font outline width\n"\
            "(7):  font canvas padding\n"\
            "(8):  rectangle x\n"\
            "(9):  rectangle y\n"\
            "(10): rectangle width\n"\
            "(11): rectangle height\n"\
            "(12): composition method\n"\
            "(13): output file\n";
    }

    int args_used() const { return 14; }

    int perform(Session &session, char** argv) {
        using Magick::Quantum;
        using Magick::Color;
        using Magick::Geometry;
        using Magick::PixelPacket;

        session.init_magick();

        char* input_string = argv[1];

        Image* canvas = session.load_canvas(argv[2]);
        if (!canvas) {
            std::cerr << "Failed to load canvas at " << argv[2] << '\n';
            return 3;
        }

        int padding     = atoi(argv[7]);
        int dest_x      = atoi(argv[8]);
        int dest_y      = atoi(argv[9]);
        int dest_width  = atoi(argv[10]);
        int dest_height = atoi(argv[11]);

        auto transparent = Magick::Color(0, 0, 0, MaxRGB);
        Magick::Image text_magick(Geometry(0, 0), transparent);
        text_magick.magick("png");
        text_magick.font(argv[3]);
        text_magick.fontPointsize(dest_height + dest_height / 2);

        Magick::TypeMetric metrics;
        text_magick.fontTypeMetrics(input_string, &metrics);

        auto textarea_width = metrics.textWidth();
        auto textarea_height = metrics.textHeight();
        // Some fonts are taller than they claim to be
        textarea_height += textarea_height / 2;
        // And on Linux, some fonts are wider than they claim to be!
        textarea_width += textarea_width / 3;

        text_magick.extent(Geometry(textarea_width, textarea_height), transparent);

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

        auto offset = Geometry(textarea_width - padding, textarea_height, padding, 0);
        text_magick.annotate(input_string, offset);
        if (overdraw) {
            text_magick.strokeColor(color);
            text_magick.strokeWidth(0.5);
            text_magick.annotate(input_string, offset);
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

        // std::cout << "Text turned out to be " << text_width << 'x' << text_height << '\n';

        Image text_image(text_magick);

        Compositor* comp = session.load_compositor(argv[12]);
        canvas->composite(text_image, 0, 0, text_width, text_height, dest_x, dest_y, dest_width, dest_height, comp);

        if (!cstr_eq(argv[13], "--")) {
            FILE* output_file = fopen(argv[13], "wb");
            bool success = canvas->save(output_file);
            if (output_file) fclose(output_file);
            if (!success) {
                std::cerr << "Failed to save!!!\n";
                return 2;
            }
        }

        return 0;
    }
};

class TestRoutine : public Routine {
public:
    const char* help_text() const {
        return "A test command\n";
    }

    int args_used() const { return 1; }

    int perform(Session &session, char** argv) {
        std::cout << "Hello!!! " << argv[0] << "\n";
        return 0;
    }
};

class HelpRoutine : public Routine {
public:
    const char* help_text() const {
        return "Shows this message\n";
    }

    int args_used() const { return 0; }

    int perform(Session &session, char **argv) {
        print_all_help_texts();
        return 0;
    }
};

//
// =============== SESSION ==============
//
std::unordered_map<std::string, Routine*> Routine::subcommands = {
    {
        { "composite", new CompositeRoutine },
        { "thumbnail", new ThumbnailRoutine },
        { "text", new TextRoutine },
        { "ftext", new FTextRoutine },
        { "arctext", new ArctextRoutine },
        { "help", new HelpRoutine }
        // { "test", new TestRoutine }
    }
};

Session::Session() 
    : last_compositor(nullptr),
      magick_has_been_initialized(false)
{}

void Session::init_magick() {
    if (!magick_has_been_initialized) {
        Magick::InitializeMagick(NULL);
        magick_has_been_initialized = true;
    }
}

Compositor* Session::load_compositor(const char* type) {
    if (cstr_eq(type, "over"))
        last_compositor = &composite_over;
    else if (cstr_eq(type, "multiply"))
        last_compositor = &composite_multiply;

    return last_compositor;
}

Image* Session::load_letter_atlas(const char* filename) {
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

CharacterSet* Session::load_charset(const char* filename) {
    if (cstr_eq(filename, "--"))
        return charset.get();

    charset.reset(new CharacterSet);
    FILE* file = fopen(filename, "r");
    bool success = charset->load_json(file);
    if (file) fclose(file);

    if (!success)
        return nullptr;

    return charset.get();
}

Image* Session::load_canvas(const char* filename) {
    if (cstr_eq(filename, "--"))
        return last_canvas.get();

    last_canvas.reset(new Image);
    FILE* file = fopen(filename, "rb");
    bool success = last_canvas->load_file(file);
    if (file) fclose(file);

    if (!success)
        return nullptr;

    return last_canvas.get();
}

Image* Session::get_last_canvas() const {
    return last_canvas.get();
}

void print_all_help_texts() {
    for (auto &r : Routine::subcommands) {
        std::cout << "==== \"" << r.first << "\" (" << r.second->args_used() << " arguments) ====\n";
        std::cout << r.second->help_text() << '\n' << std::endl;
    }
}

}
