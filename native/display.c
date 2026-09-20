/* Metal Fatigue's menus use scaled fonts, but its mission UI and camera use
 * pixel coordinates. Keep a 1280x720 logical canvas for the three HD modes
 * while the host rasterizes their triangles at the selected output resolution.
 * All addresses are tied to the two hashes in game.toml.
 */
#include "x86.h"
#include "dx/host_api.h"

void fn_1000d860(X86 *c);
void fn_1000b620(X86 *c);

static const uint32_t output_modes[3][2] = {{1920, 1080}, {2560, 1440}, {3840, 2160}};

/* The core display plugin installs the reviewed six-mode table at DllMain.
 * Accept either its physical form or the logical form left by enumeration.
 */
static int adapted_modes(void) {
    for (uint32_t i = 0; i < 3; ++i) {
        uint32_t w = rd32(0x100380d8 + i * 8), h = rd32(0x100380dc + i * 8);
        if (!((w == output_modes[i][0] && h == output_modes[i][1]) || (w == 1280 && h == 720)))
            return 0;
    }
    return 1;
}

/* EnumDisplayModes calls this once per physical mode. Compare against the
 * advertised HD sizes, then leave logical sizes for device/surface creation,
 * renderer rectangles, UI hit testing, cursor bounds and camera constraints.
 * Using one coordinate space avoids separately scaling every HUD control.
 */
void mf_enumerate_mode(X86 *c) {
    int adapted = adapted_modes();
    uint32_t desc = rd32(c->r[R_ESP] + 4), driver = rd32(c->r[R_ESP] + 8);
    if (desc && driver && rd32(desc + 0x4c) == 0x40) {
        uint32_t depth = rd32(desc + 0x54);
        if (depth == 16 || depth == 32)
            for (uint32_t i = 0; i < 6; ++i) {
                uint32_t w = adapted && i >= 3 ? output_modes[i - 3][0] : rd32(0x100380c0 + i * 8);
                uint32_t h = adapted && i >= 3 ? output_modes[i - 3][1] : rd32(0x100380c4 + i * 8);
                if (rd32(desc + 0xc) == w && rd32(desc + 8) == h)
                    wr32(driver + 0x6a8 + 4 * i, rd32(driver + 0x6a8 + 4 * i) | depth);
            }
    }
    if (adapted)
        for (uint32_t i = 0; i < 3; ++i) {
            wr32(0x100380d8 + i * 8, 1280);
            wr32(0x100380dc + i * 8, 720);
        }
    // The callback RET can unwind directly into guest_call; finish all table
    // changes before returning rather than wrapping the translated RET.
    c->r[R_EAX] = desc && driver ? 1 : 0;
    c->eip = rd32(c->r[R_ESP]);
    c->r[R_ESP] += 12;
    recomp_return(c);
}

void mf_initialize_renderer(X86 *c) {
    uint32_t mode = rd32(0x00522d5c);
    if (adapted_modes() && mode >= 3 && mode < 6)
        host_set_render_resolution(output_modes[mode - 3][0], output_modes[mode - 3][1]);
    else
        host_set_render_resolution(0, 0);
    fn_1000b620(c);
}

/* The old scaled-font method subtracts (scale-1)/32 from each glyph's UV
 * endpoint, eventually reversing its atlas rectangle. Its unscaled sibling
 * uses the original UVs. Apply that same sampling to scaled text, retaining
 * the original layout, glyph advances, colours and draw calls. The shared
 * 1/32 constant is also used for terrain UVs, so restore it before returning.
 * This renderer leaf does not switch the game's cooperative stack.
 */
void mf_scaled_font(X86 *c) {
    uint32_t saved = rd32(0x10031428);
    wrf32(0x10031428, 0.0f);
    fn_1000d860(c);
    wr32(0x10031428, saved);
}
