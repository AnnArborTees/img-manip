#include <iostream>
#include <memory>
#include <string.h>
#include "character_set.h"

namespace mockbot {
    CharacterOffsets::CharacterOffsets() : x(-1), y(-1), width(-1), height(-1) {
    };

    CharacterSet::CharacterSet() {
    }

    bool CharacterSet::load_json(FILE* input) {
        // Read all of file
        fseek(input, 0, SEEK_END);
        size_t filesize = ftell(input);
        rewind(input);

        std::unique_ptr<char[]> json_text(new char[filesize]);
        fread(json_text.get(), 1, filesize, input);
        json_text[filesize] = '\0';

        // Tokenize JSON
        cJSON* root = cJSON_Parse(json_text.get());

        auto entry = root->child;
        while (entry) {
            // We want entries of type array and single-character names
            if (entry->type != cJSON_Array) continue;
            if (strlen(entry->string) != 1) continue;
            char c = entry->string[0];
            // if (c >= CHARSET_OFFSET_COUNT) continue; // with CHARSET_OFFSET_COUNT as 256, this will always be false

            CharacterOffsets* offsets = &offsets[c];

            // TODO TODO read x,y,w,h from entry->child and siblings

            entry = entry->next;
        }

        cJSON_Delete(root);
    }
}
