#include <stdio.h>
#include <unordered_map>
#include <string>
#include "cJSON.h"
#include "utf8/checked.h"
#include "utf8/unchecked.h"

#define CHARSET_OFFSET_COUNT 256

namespace mockbot {
    struct CharacterOffsets {
        CharacterOffsets();
        CharacterOffsets(cJSON* node);

        int x;
        int y;
        int width;
        int height;
        int l_pad;
        int r_pad;
        int al_pad;
        int ar_pad;
    };
    enum DimensionType {
        Linear,
        Angular
    };

    class CharacterSet {
    public:
        CharacterSet();

        bool load_json(FILE* input);
        CharacterOffsets* operator[](char* c);
        CharacterOffsets* operator[](uint32_t codepoint);
        void get_dimensions(DimensionType dim, char* string, int* total_width, int* total_height, int* len);
        void get_dimensions(DimensionType dim, char* string, int* total_width, int* total_height);

    private:
        // CharacterOffsets offsets[CHARSET_OFFSET_COUNT];
        std::unordered_map<uint32_t, CharacterOffsets> offsets;
    };
}
