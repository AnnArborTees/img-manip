#include <inttypes.h>
#include <png.h>
#include <zlib.h>
#include <pngconf.h>
#include <functional>

// Accepted color types:
#define COLOR_TYPE_RGBA 6
#define COLOR_TYPE_RGB  2

namespace mockbot {
    enum CompositeOp {
        COMP_OVER,
        COMP_MULTIPLY
    };

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

        uint8_t* pixel(int x, int y);

        bool load_file(FILE* input);
        bool make_background_transparent();
        // Composites image data of other onto image data this, modifying this->image_data
        bool composite(Image& other, int x, int y, int width, int height, Compositor* func);
        bool save(FILE* output);
        void cleanup();

    private:
        uint8_t* image_data;
        uint32_t width;
        uint32_t height;
        int bit_depth;
        int color_type;
        int bytes_per_pixel;
    };
}
