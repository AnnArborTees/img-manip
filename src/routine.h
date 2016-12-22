#ifndef ROUTINE_H
#define ROUTINE_H
#include <string>
#include <iostream>
#include "image.h"
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <memory>
#include <functional>
#include <unordered_map>

static bool cstr_eq(const char* s1, const char* s2) {
    size_t i = 0;
    char c1;
    char c2;

    while (true) {
        c1 = s1[i];
        c2 = s2[i];

        if (c1 != c2)
            return false;

        if (c1 == '\0')
            return true;

        i += 1;
    }
}

static int positive_int_arg(char* arg) {
    if (cstr_eq(arg, "--"))
        return -1;
    else
        return atoi(arg);
}

namespace mockbot {
    class Session {
    public:
        Session();

        void init_magick();
        Compositor*   load_compositor(const char* type);
        Image*        load_letter_atlas(const char* filename);
        CharacterSet* load_charset(const char* filename);
        Image*        load_canvas(const char* filename);

        Image*        get_last_canvas() const;

        CompositeOver composite_over;
        CompositeMultiply composite_multiply;

    private:
        Compositor* last_compositor;

        std::unique_ptr<Image>        letter_atlas;
        std::unique_ptr<CharacterSet> charset;
        std::unique_ptr<Image>        last_canvas;
        bool magick_has_been_initialized;
    };

    class Routine {
    public:
        static std::unordered_map<std::string, Routine*> subcommands;

        virtual int perform(Session &session, char** argv) = 0;
        virtual int args_used() const = 0;
    };

}
#endif // ROUTINE_H
