#include "character_set.h"
#include <inttypes.h>
#include <png.h>
#include <zlib.h>
#include <pngconf.h>
#include <functional>
#include <string>

// Accepted color types:
#define COLOR_TYPE_RGBA 6
#define COLOR_TYPE_RGB  2

namespace mockbot {
    struct ImageWrite {
        ImageWrite();
        ~ImageWrite();

        png_structp png;
        png_infop info;
    };
    struct ImageRead {
        ImageRead();
        ~ImageRead();

        png_structp png;
        png_infop info;
    };

    struct Compositor {
        virtual double blend_color(double, double, double, double, double) = 0;
        virtual double blend_alpha(double, double) = 0;
    };
    class CompositeOver : public Compositor {
        virtual double blend_color(double srcc, double srca, double dstc, double dsta, double omsrca);
        virtual double blend_alpha(double srca, double dsta);
    };
    class CompositeMultiply : public Compositor {
        virtual double blend_color(double srcc, double srca, double dstc, double dsta, double omsrca);
        virtual double blend_alpha(double srca, double dsta);
    };

    class Image {
    public:
        Image();
        ~Image();

        // Returns a pointer to the red component of the pixel at x,y.
        // Returns garbage if a file hasn't been loaded yet.
        uint8_t* pixel(int x, int y);

        // Decodes and loads the given file into image_data
        bool load_file(FILE* input);

        // Allocates new image_data with just the given color
        bool fill_blank(std::string hexcode, int width, int height);
        bool fill_blank(uint8_t* color_pixel, int width, int height);

        // Returns true if the image has been loaded with load_file or fill_blank.
        bool loaded();

        // Converts non-transparency images to transparency using the first pixel as the "background" color.
        // Does nothing to images with transparency.
        bool make_background_transparent();

        // Sets background color to the given hexcode. Returns false if the image has no transparency.
        // NOTE this is currently unused.
        bool set_background_to(std::string hexcode, Compositor* func);

        // Composites image data of other onto image data this, modifying this->image_data
        bool composite(Image& other, int x, int y, int width, int height, Compositor* func);

        // More advanced composite that will only copy a section of other onto this
        bool composite(Image& other, int blit_x, int blit_y, int blit_width, int blit_height, int x, int y, int width, int height, Compositor* func);

        // More advanced composite that will only copy a section of other onto this
        bool composite(Image& other, CharacterOffsets* offests, int x, int y, int width, int height, Compositor* func);

        // Encodes and writes image_data to the given file
        bool save(FILE* output);

        // Returns the reason the previous method returned false (currently only works after load_file())
        std::string last_error();

        // Frees image_data if needed
        void cleanup();

        long unsigned int width;
        long unsigned int height;
    private:
        uint8_t* image_data;
        int bit_depth;
        int color_type;
        int bytes_per_pixel;
        std::string error;
    };
}
