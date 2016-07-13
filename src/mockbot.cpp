#include <string>
#include <iostream>
#include "image.h"
#include "stdio.h"

using namespace mockbot;

/*
Example usage I guess:

mockbot -a artwork.png -m mockup.png -w 350 -h 450
*/

int main(int argc, char** argv) {
    if (argc == 9) {
        Image img1;
        Image img2;

        FILE* f1 = fopen(argv[1], "rb");
        if (!f1) {
            std::cerr << "Couldn't open " << argv[1] << '\n';
            return 2;
        }

        FILE* f2 = fopen(argv[2], "rb");
        if (!f2) {
            std::cerr << "Couldn't open " << argv[2] << '\n';
            return 2;
        }

        int x = std::stoi(std::string(argv[3]));
        int y = std::stoi(std::string(argv[4]));
        int w = std::stoi(std::string(argv[5]));
        int h = std::stoi(std::string(argv[6]));

        if (!img1.load_file(f1)) {
            std::cerr << "Failed decode PNG: " << argv[1] << '\n';
            return 2;
        }
        if (!img2.load_file(f2)) {
            std::cerr << "Failed decode PNG: " << argv[2] << '\n';
            return 2;
        }

        img1.make_background_transparent();
        img2.make_background_transparent();

        CompositeOver over;
        CompositeMultiply multiply;
        Compositor* comp;

        std::string operation = argv[7];
        if (operation == "multiply")
            comp = &multiply;
        else
            comp = &over;

        if (!img1.composite(img2, x, y, w, h, comp)) {
            std::cerr << "Failed to composite " << argv[2] << " onto " << argv[1] << '\n';
            return 2;
        }

        fclose(f1);
        fclose(f2);

        FILE* f3 = fopen(argv[8], "wb");
        if (!f3) {
            std::cerr << "Couldn't reopen " << argv[9] << " for writing\n";
            return 2;
        }

        if (!img1.save(f3)) {
            std::cerr << "Failed to encode result to " << argv[9] << '\n';
            return 2;
        }
        fclose(f3);
    }
    else {
        std::cout << "Do `" << argv[0] << " file1 file2 x y width height (over|multiply) outputfile` to comp file2 onto file1 at the given position/dimensions, then save into outputfile\n";
        return 1;
    }
    return 0;
}
