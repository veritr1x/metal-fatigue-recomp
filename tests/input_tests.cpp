// Touch placement must use the game's live cursor pair, not a host estimate.
#include "runtime/memory.h"
#include "runtime/native_seam.h"
#include <cstdio>
#include <cstring>

static int checks, failures;
static void check(bool good, const char *what) {
    ++checks;
    if (!good) {
        ++failures;
        std::fprintf(stderr, "FAIL: %s\n", what);
    }
}

int main() {
    mem_init();
    check(!recomp_pointer_place(10, 20, 640, 480), "startup has no live input");
    const uint32_t input = heap_alloc(0x250, true), menu = heap_alloc(0x98, true),
                   mission = heap_alloc(0xfcc, true);
    wr32(0x005229c0, input);
    wr32(input + 0x10, 1);
    wr32(0x00522a00, menu);
    wr32(menu, 0x004d4e70);
    wr32(menu + 0x90, input);
    wr32(0x005229f8, mission);
    wr32(mission, 0x004e1ab8);
    const int sizes[][2] = {{640, 480}, {800, 600}, {1024, 768}, {1280, 720}};
    for (const auto &size : sizes) {
        for (int i = 1; i <= 3; ++i) {
            const int x = size[0] * i / 4, y = size[1] * (4 - i) / 4;
            wrf32(menu + 0x84, 5);
            wrf32(menu + 0x88, 9);
            wrf32(mission + 0xf0, 600);
            wrf32(mission + 0xf4, 400);
            wr32(input + 0x14, 175);
            wr32(input + 0x18, uint32_t(-90));
            memset(g_mem + input + 0x1c, 0x81, 0x230);
            unsigned char saved[0x230];
            memcpy(saved, g_mem + input + 0x1c, sizeof saved);
            check(recomp_pointer_place(x, y, size[0], size[1]), "live cursor placed");
            check(rdf32(menu + 0x84) == 160 * i && rdf32(menu + 0x88) == 120 * (4 - i),
                  "menu coordinates scale to its fixed canvas");
            check(rdf32(mission + 0xf0) == x && rdf32(mission + 0xf4) == y,
                  "mission uses logical renderer pixels");
            check(rd32(input + 0x14) == 0 && rd32(input + 0x18) == 0,
                  "cached deltas cannot move the cursor again");
            check(!memcmp(saved, g_mem + input + 0x1c, sizeof saved),
                  "wheel, buttons, previous edges and keyboard are untouched");
        }
    }
    recomp_pointer_place(9999, -50, 1280, 720);
    check(rdf32(menu + 0x84) == 630 && rdf32(menu + 0x88) == 0, "menu retains its sprite bounds");
    check(rdf32(mission + 0xf0) == 1279 && rdf32(mission + 0xf4) == 0,
          "mission stays inside its canvas");
    check(!recomp_pointer_place(1, 2, 0, 720), "invalid canvas rejected");
    wr32(menu, 0);
    wr32(mission, 0);
    wr32(input + 0x14, 55);
    check(!recomp_pointer_place(100, 200, 640, 480), "unrecognized objects rejected");
    check(rd32(input + 0x14) == 55, "fallback retains relative input");
    wr32(0x00522a00, GUEST_SIZE - 4);
    wr32(0x005229f8, 0xfffffff0);
    check(!recomp_pointer_place(1, 2, 640, 480), "invalid cursor addresses rejected");
    wr32(0x005229c0, GUEST_SIZE - 4);
    check(!recomp_pointer_place(1, 2, 640, 480), "invalid input address rejected");
    std::printf("input: %d checks, %d failures\n", checks, failures);
    mem_shutdown();
    return failures ? 1 : 0;
}
