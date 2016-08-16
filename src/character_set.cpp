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

    CharacterOffsets::CharacterOffsets() : x(-1), y(-1), width(-1), height(-1),
                                           l_pad(0), r_pad(0), al_pad(0), ar_pad(0) {
    };

    CharacterOffsets::CharacterOffsets(cJSON* node) : l_pad(0), r_pad(0), al_pad(0), ar_pad(0) {
        auto element = node->child;
        x = element->valueint;

        element = element->next;
        y = element->valueint;

        element = element->next;
        width = element->valueint;

        element = element->next;
        height = element->valueint;

        element = element->next;
        if (!element) return;
        l_pad = element->valueint;

        element = element->next;
        if (!element) return;
        r_pad = element->valueint;

        element = element->next;
        if (!element) return;
        al_pad = element->valueint;

        element = element->next;
        if (!element) return;
        ar_pad = element->valueint;
    };

    CharacterSet::CharacterSet() {
    }

    CharacterOffsets* CharacterSet::operator[](char* c) {
        char* letter = c;
        uint32_t codepoint = utf8::unchecked::next(letter);
        return &offsets[codepoint];
    }

    CharacterOffsets* CharacterSet::operator[](uint32_t codepoint) {
        return &offsets[codepoint];
    }

    void CharacterSet::get_dimensions(DimensionType dim, char* string, int* total_width, int* total_height, int* len) {
        int total_text_width  = 0;
        int total_text_height = 0;
        int str_len = 0;

        char* letter = string;

        while (*letter != '\0') {
            str_len += 1;
            uint32_t codepoint = utf8::unchecked::next(letter);

            auto offs = &offsets[codepoint];
            if (offs->width >= 0) {
                int total_padding = 0;
                if (dim == Linear)
                    total_padding = offs->l_pad + offs->r_pad;
                else if (dim == Angular)
                    total_padding = offs->al_pad + offs->ar_pad;

                total_text_width += offs->width + total_padding;
            }
            if (offs->height >= 0) {
                if (offs->height > total_text_height)
                    total_text_height = offs->height;
            }
        }

        *total_width  = total_text_width;
        *total_height = total_text_height;
        if (len)
            *len = str_len;
    }
    void CharacterSet::get_dimensions(DimensionType dim, char* string, int* total_width, int* total_height) {
        get_dimensions(dim, string, total_width, total_height, NULL);
    }

    bool CharacterSet::load_json(FILE* input) {
        if (!input) return false;
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
            // We want entries of type array
            if (entry->type != cJSON_Array) { entry = entry->next; continue; };

            char* letter = entry->string;
            uint32_t codepoint = utf8::unchecked::next(letter);

            offsets[codepoint] = CharacterOffsets(entry);

            entry = entry->next;
        }

	return true;
    }
}
