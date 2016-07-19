#include <stdio.h>
#include "cJSON.h"

#define CHARSET_OFFSET_COUNT 256

namespace mockbot {
    struct CharacterOffsets {
        CharacterOffsets();

        int x;
        int y;
        int width;
        int height;
    };

    class CharacterSet {
    public:
        CharacterSet();

        bool load_json(FILE* input);
        CharacterOffsets* operator[](char c);
        void get_dimensions(char* string, int* total_width, int* total_height);
        void get_dimensions(const char* string, int* total_width, int* total_height);

    private:
        CharacterOffsets offsets[CHARSET_OFFSET_COUNT];
    };
}
