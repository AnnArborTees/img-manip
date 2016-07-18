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

    private:
        CharacterOffsets offsets[CHARSET_OFFSET_COUNT];
    };
}
