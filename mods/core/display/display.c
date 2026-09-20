/* Display adaptation for the hash-pinned MFatigue.exe / DirectXRendEng.dll.
 * Keep the original six-button Options flow and renderer reboot ownership.
 * Addresses below were verified against the original PE code/data; originals
 * and generated translations remain untouched.
 */
#include "pop_mod_api.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

POP_MOD_DECLARE_ABI();

static const uint32_t modes[6][2] = {{640, 480},   {800, 600},   {1024, 768},
                                     {1920, 1080}, {2560, 1440}, {3840, 2160}};
static const uint32_t original_modes[6][2] = {{640, 480},  {800, 600},   {960, 720},
                                              {1024, 768}, {1280, 1024}, {1600, 1200}};
static uint32_t labels, registry_scratch;
static int profile_seeded;

static uint32_t read32(const PopModApi *api, uint32_t address) {
    uint32_t value = 0;
    api->guest_read_u32(api, address, &value);
    return value;
}

/* Fresh installations need the supported DirectX driver registered before
 * the game reads its options. Use the pinned EXE's imported registry APIs,
 * filling only missing values so resolution, audio and scrolling persist.
 * This also gives mobile bundles the same first-run defaults as desktop.
 */
static void prepare_profile(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv,
                            void *user) {
    if (profile_seeded)
        return;
    profile_seeded = 1;
    unsigned char *scratch = NULL;
    if (api->guest_ptr(api, registry_scratch, 512, (void **)&scratch) != POP_OK)
        return;
    strcpy((char *)scratch, "SOFTWARE\\Psygnosis\\Metal Fatigue");
    uint32_t args[9] = {0x80000002, registry_scratch,       0, 0, 0, 0xf003f,
                        0,          registry_scratch + 256, 0},
             result = 1;
    if (api->guest_call(api, read32(api, 0x004d2014), 0, args, 9, &result) != POP_OK || result)
        return;
    uint32_t key = read32(api, registry_scratch + 256);
    static const struct {
        const char *name;
        uint32_t value;
    } defaults[] = {{"RegVersion", 0x101}, {"Driver", 0},        {"NumDrivers", 1},
                    {"Resolution", 0},     {"ColorDepth", 0},    {"Rendering", 8},
                    {"Particles", 1},      {"ScrollRate", 5},    {"PlayVoice", 1},
                    {"PlayMusic", 1},      {"SoundLevel", 8},    {"MusicLevel", 5},
                    {"VoiceLevel", 8},     {"Gamma", 0x3f800000}};
    for (uint32_t i = 0; i < sizeof(defaults) / sizeof(defaults[0]) + 2; ++i) {
        uint32_t count = sizeof(defaults) / sizeof(defaults[0]);
        const char *name = i < count ? defaults[i].name : i == count ? "driver:0" : "CDPath";
        strcpy((char *)scratch, name);
        uint32_t query[6] = {key, registry_scratch, 0, 0, 0, registry_scratch + 260};
        if (api->guest_call(api, read32(api, 0x004d2018), 0, query, 6, &result) != POP_OK ||
            result != 2) // ERROR_FILE_NOT_FOUND; all existing preferences are retained
            continue;
        uint32_t type = i < count ? 4 : 1, bytes = 4;
        if (i < count)
            memcpy(scratch + 128, &defaults[i].value, 4);
        else {
            const char *value = i == count ? "Direct3D,1,0" : "C:\\GOG Games\\Metal Fatigue\\";
            strcpy((char *)scratch + 128, value);
            bytes = (uint32_t)strlen(value) + 1;
        }
        uint32_t set[6] = {key, registry_scratch, 0, type, registry_scratch + 128, bytes};
        api->guest_call(api, read32(api, 0x004d2000), 0, set, 6, &result);
    }
    api->guest_call(api, read32(api, 0x004d2008), 0, &key, 1, &result);
    api->guest_call(api, read32(api, 0x004d200c), 0, &key, 1, &result);
}

/* DllMain calls the EXE's exported rendmalloc at 1000aee0. That seam is
 * after every verified DLL remap and before device enumeration. Both the
 * enumeration callback (10002e60) and initialization (10001dd0/1000b620)
 * read this same six-entry table; no executable instructions are patched.
 */
static void renderer_attach(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv,
                            void *user) {
    void *table = NULL;
    if (api->guest_ptr(api, 0x100380c0, sizeof(original_modes), &table) != POP_OK ||
        memcmp(table, original_modes, sizeof(original_modes)) != 0) {
        api->log(api, "Renderer mode table differs from the reviewed image; leaving it unchanged");
        return;
    }
    memcpy(table, modes, sizeof(modes));
    api->log(api, "Renderer modes: 640x480, 800x600, 1024x768, 1920x1080, 2560x1440, 3840x2160");
}

/* The graphics panel constructor returns its this pointer in EAX, with six
 * button pointers at +100..114. Use the game's text setter so label storage,
 * font resources and teardown retain their original ownership.
 */
static void graphics_panel(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv,
                           void *user) {
    uint32_t panel = cpu->eax;
    for (uint32_t i = 0; i < 6; ++i) {
        uint32_t button = read32(api, panel + 0x100 + 4 * i);
        if (!button)
            continue;
        uint32_t args[2] = {labels + i * 16, 0}, result;
        api->guest_call(api, 0x00417580, button, args, 2, &result);
    }
}

PopModStatus pop_mod_init(const PopModApi *api) {
    uint32_t hook;
    PopModStatus status = api->guest_alloc(api, 6 * 16, &labels);
    if (status != POP_OK)
        return status;
    status = api->guest_alloc(api, 512, &registry_scratch);
    if (status != POP_OK)
        return status;
    void *storage = NULL;
    if ((status = api->guest_ptr(api, labels, 6 * 16, &storage)) != POP_OK)
        return status;
    for (uint32_t i = 0; i < 6; ++i)
        snprintf((char *)storage + i * 16, 16, "%ux%u", modes[i][0], modes[i][1]);
    status = api->hook_install_ex(api, 0x0040d470, 0x1000aee6, renderer_attach, POP_HOOK_BEFORE,
                                  POP_HOOK_NO_GAME_VIEW, NULL, &hook);
    if (status != POP_OK)
        return status;
    status = api->hook_install_ex(api, 0x00415560, 0, graphics_panel, POP_HOOK_AFTER,
                                  POP_HOOK_NO_GAME_VIEW, NULL, &hook);
    if (status != POP_OK)
        return status;
    return api->hook_install_ex(api, 0x0042d4a0, 0, prepare_profile, POP_HOOK_BEFORE,
                                POP_HOOK_NO_GAME_VIEW, NULL, &hook);
}

PopModStatus pop_mod_exit(void) {
    return POP_OK;
}
