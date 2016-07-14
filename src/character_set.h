#include <stdio.h>
#include "cJSON.h"

namespace mockbot {
    struct CharacterOffsets {
        CharacterOffsets();

        int x;
        int y;
        int width;
        int height;
    };

#define CHARSET_OFFSET_COUNT 256

    class CharacterSet {
    public:
        CharacterSet();

        bool load_json(FILE* input);

    private:
        CharacterOffsets offsets[CHARSET_OFFSET_COUNT];
    };
}
