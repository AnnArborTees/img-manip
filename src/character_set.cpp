#include <iostream>
#include <memory>
#include <string.h>
#include "character_set.h"

namespace mockbot {
    // RAII for destroying cJSON root
    struct cJSONDeferDestroy {
        cJSONDeferDestroy(cJSON* r) : root(r) {
        }
        ~cJSONDeferDestroy() {
            if (root) cJSON_Delete(root);
        }

        cJSON* root;
    };

    CharacterOffsets::CharacterOffsets() : x(-1), y(-1), width(-1), height(-1) {
    };

    CharacterSet::CharacterSet() {
    }

    CharacterOffsets* CharacterSet::operator[](char c) {
        return &offsets[c];
    }

    void CharacterSet::get_dimensions(char* string, int* total_width, int* total_height) {
        int total_text_width  = 0;
        int total_text_height = 0;

        for (char* c = string; *c != '\0'; c++) {
            CharacterOffsets* offs = &offsets[*c];

            if (offs->width >= 0)
                total_text_width += offs->width;
            if (offs->height >= 0) {
                if (offs->height > total_text_height)
                    total_text_height = offs->height;
            }
        }

        *total_width  = total_text_width;
        *total_height = total_text_height;
    }
    void CharacterSet::get_dimensions(const char* string, int* total_width, int* total_height) {
        auto size = strlen(string) + 1;
        char* s = (char*)malloc(size);
        memcpy(s, string, size);
        get_dimensions(s, total_width, total_height);
        free(s);
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
        // Make sure the root is destroyed when this function exits
        cJSONDeferDestroy destroy(root);

        auto entry = root->child;
        while (entry) {
#define NEXT { entry = entry->next; continue; }
            // We want entries of type array and single-character names
            if (entry->type != cJSON_Array) NEXT;
            if (strlen(entry->string) != 1) NEXT;
            char c = entry->string[0];
            // if (c >= CHARSET_OFFSET_COUNT) continue; // with CHARSET_OFFSET_COUNT as 256, this will always be false

            CharacterOffsets* offset = &offsets[c];

            // read x,y,w,h from entry->child and siblings
            auto offsetList = entry;
#define NEXT_VALUE(_child_, _var_) { offsetList = offsetList->_child_; if (!offsetList) return false; offset->_var_ = offsetList->valueint; }
            NEXT_VALUE(child, x);
            NEXT_VALUE(next,  y);
            NEXT_VALUE(next,  width);
            NEXT_VALUE(next,  height);
            // TODO not 100% sure why doing this causes it to work :(
            // offset->x /= 2;
            // offset->y /= 2;

            entry = entry->next;
        }

	return true;
    }
}
