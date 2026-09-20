// Exercise the callback ABI that previously skipped a wrapper's table update.
#include "runtime/memory.h"
#include "runtime/x86.h"
#include <cstdio>
#include <cstring>

extern "C" void mf_enumerate_mode(X86 *);
extern "C" void mf_initialize_renderer(X86 *);
extern "C" void mf_scaled_font(X86 *);
static int failures, checks, render_w, render_h, init_calls, font_calls;
static void check(bool good, const char *what) {
    ++checks;
    if (!good) {
        ++failures;
        std::fprintf(stderr, "FAIL: %s\n", what);
    }
}
extern "C" void host_set_render_resolution(int w, int h) {
    render_w = w;
    render_h = h;
}
extern "C" void fn_1000b620(X86 *) {
    ++init_calls;
}
extern "C" void fn_1000d860(X86 *) {
    ++font_calls;
    check(rdf32(0x10031428) == 0, "scaled glyphs retain their full atlas rectangle");
}

int main() {
    mem_init();
    static const uint32_t address[] = {0x10002e60};
    static void (*const functions[])(X86 *) = {mf_enumerate_mode};
    static RecompHookFn hooks[1]{};
    static uint8_t hooked[1]{};
    static const RecompModule module = {"display-test", address[0], address[0] + 1, address, 1,
                                        functions,      hooks,      hooked,         nullptr, 0,
                                        nullptr};
    recomp_module_register(&module);
    const uint32_t advertised[6][2] = {{640, 480},   {800, 600},   {1024, 768},
                                       {1920, 1080}, {2560, 1440}, {3840, 2160}};
    const uint32_t original[6][2] = {{640, 480},  {800, 600},   {960, 720},
                                     {1024, 768}, {1280, 1024}, {1600, 1200}};
    uint32_t desc = heap_alloc(0x80, true), driver = heap_alloc(0x700, true);
    X86 cpu{};
    cpu.r[R_ESP] = heap_alloc(0x1000, true) + 0x800;
    cpu.r[R_EBX] = 0x12345678;
    const uint32_t esp = cpu.r[R_ESP];
    for (int cycle = 0; cycle < 3; ++cycle) {
        for (int i = 0; i < 6; ++i) {
            wr32(0x100380c0 + i * 8, advertised[i][0]);
            wr32(0x100380c4 + i * 8, advertised[i][1]);
            wr32(driver + 0x6a8 + i * 4, 0);
        }
        // Later callbacks must still match physical modes after the first
        // callback has converted the table to logical dimensions.
        for (int i = 5; i >= 0; --i) {
            wr32(desc + 0xc, advertised[i][0]);
            wr32(desc + 8, advertised[i][1]);
            wr32(desc + 0x4c, 0x40);
            wr32(desc + 0x54, 16);
            check(guest_call(&cpu, address[0], desc, driver) == 1, "enumeration continues");
            check(cpu.r[R_ESP] == esp && cpu.r[R_EBX] == 0x12345678,
                  "callback preserves caller stack and callee-saved registers");
            check(rd32(driver + 0x6a8 + i * 4) == 16, "physical output is available");
            check(rd32(0x100380e8) == 1280 && rd32(0x100380ec) == 720,
                  "logical table is updated before callback return unwinds");
        }
        for (int i = 0; i < 6; ++i) {
            wr32(0x00522d5c, i);
            mf_initialize_renderer(&cpu);
            check(render_w == (i < 3 ? 0 : int(advertised[i][0])) &&
                      render_h == (i < 3 ? 0 : int(advertised[i][1])),
                  "selected output survives repeated renderer initialization");
        }
    }
    check(init_calls == 18, "original initialization runs once per call");
    check(guest_call(&cpu, address[0], 0, driver) == 0, "null callback descriptor aborts");
    for (int i = 0; i < 6; ++i) {
        wr32(0x100380c0 + i * 8, original[i][0]);
        wr32(0x100380c4 + i * 8, original[i][1]);
    }
    wr32(desc + 0xc, 960);
    wr32(desc + 8, 720);
    wr32(driver + 0x6b0, 0);
    guest_call(&cpu, address[0], desc, driver);
    check(rd32(driver + 0x6b0) == 16 && rd32(0x100380e8) == 1600,
          "original mode table retains original behavior when plugin is absent");
    mf_initialize_renderer(&cpu);
    check(render_w == 0 && render_h == 0, "unadapted renderer clears output override");
    wrf32(0x10031428, 1.0f / 32);
    mf_scaled_font(&cpu);
    check(font_calls == 1 && rdf32(0x10031428) == 1.0f / 32,
          "shared terrain UV constant is restored after font drawing");
    std::printf("display: %d checks, %d failures\n", checks, failures);
    mem_shutdown();
    return failures ? 1 : 0;
}
