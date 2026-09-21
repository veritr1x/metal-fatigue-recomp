/* The menu and mission integrate relative DirectInput motion into different
 * float cursor pairs. Touch is absolute: place those pairs in their respective
 * coordinate systems before the original game performs its picking. See
 * docs/analysis.md for the pinned executable's constructors and integrators.
 */
#include "x86.h"
#include "runtime/native_seam.h"

static int valid(uint32_t address, uint32_t size) {
    return address && address < GUEST_SIZE && size <= GUEST_SIZE - address;
}

static float clamp(float value, float maximum) {
    return value < 0 ? 0 : value > maximum ? maximum : value;
}

int recomp_pointer_place(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (!g_mem || width <= 0 || height <= 0)
        return 0;
    uint32_t input = rd32(0x005229c0);
    if (!valid(input, 0x24c) || rd32(input + 0x10) != 1)
        return 0;
    uint32_t menu = rd32(0x00522a00), mission = rd32(0x005229f8);
    int placed = 0;
    if (valid(menu, 0x98) && rd32(menu) == 0x004d4e70 && rd32(menu + 0x90) == input) {
        // Menu art and hit boxes remain 640x480 at every renderer resolution.
        // The final ten pixels are reserved for the cursor sprite itself.
        wrf32(menu + 0x84, clamp((float)x * 640 / width, 630));
        wrf32(menu + 0x88, clamp((float)y * 480 / height, 470));
        placed = 1;
    }
    if (valid(mission, 0xf8) && rd32(mission) == 0x004e1ab8) {
        // Mission picking uses renderer pixels, including the HD logical canvas.
        // Its original update still applies any modal bounds and Screen2World.
        wrf32(mission + 0xf0, clamp((float)x, (float)(width - 1)));
        wrf32(mission + 0xf4, clamp((float)y, (float)(height - 1)));
        placed = 1;
    }
    if (placed) {
        // Placement can arrive after Read but before the cursor coroutine.
        // Remove that cached motion as well as any pending DirectInput motion,
        // without consuming wheel movement or changing button edge history.
        wr32(input + 0x14, 0);
        wr32(input + 0x18, 0);
        dinput_discard_mouse_motion(rd32(input + 8));
    }
    return placed;
}
