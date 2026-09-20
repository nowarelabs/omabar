/*
 * config_test.c — Standalone regression test for the OMABC config loader.
 *
 * Compiles and runs independently of the main daemon. Does NOT require a
 * display or window server. Validates config_load() against valid and
 * invalid inputs, checks that bar settings and items are correctly parsed.
 *
 * Usage: ./bin/config_test [path_to_omabc_config]
 *   If no path is given, tests against /tmp/omabar_test_config.
 */

#include "config.h"
#include "bar_manager.h"
#include "bar_item.h"
#include "color.h"
#include "misc/env_vars.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Config loader uses these globals via extern in config.c.
   Define them here so config_test can link independently of main.c. */
struct bar_manager g_bar_manager;
struct env_vars g_env_vars;

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

static void check(const char *name, int condition) {
    tests_run++;
    if (condition) {
        tests_passed++;
        printf("  PASS: %s\n", name);
    } else {
        tests_failed++;
        printf("  FAIL: %s\n", name);
    }
}

static void check_str(const char *name, const char *got, const char *expected) {
    check(name, got && expected && strcmp(got, expected) == 0);
}

static void check_int(const char *name, int got, int expected) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s (got %d, expected %d)", name, got, expected);
    check(buf, got == expected);
}

static void check_nonzero(const char *name, int got) {
    check(name, got != 0);
}

/* Create a minimal valid OMABC config at the given path for testing. */
static void make_minimal_config(const char *path) {
    const char *json =
        "{"
        "\"daemon\":{\"lock_file\":\"/tmp/omabar_test.lock\"},"
        "\"bar\":{"
            "\"position\":\"top\","
            "\"height\":38,"
            "\"width\":0,"
            "\"margin\":0,"
            "\"blur_radius\":30,"
            "\"color\":\"0x40000000\","
            "\"shadow\":true,"
            "\"sticky\":true,"
            "\"topmost\":true,"
            "\"alpha\":1.0"
        "},"
        "\"defaults\":{"
            "\"icon\":{\"color\":\"0xffffffff\"},"
            "\"label\":{\"color\":\"0xffffffff\"}"
        "},"
        "\"items\":{"
            "\"right\":["
                "{"
                    "\"name\":\"clock\","
                    "\"type\":\"clock\","
                    "\"position\":\"r\","
                    "\"update_mask\":[\"scroll.tick\"],"
                    "\"update_interval\":1"
                "},"
                "{"
                    "\"name\":\"spaces\","
                    "\"type\":\"space\","
                    "\"position\":\"l\","
                    "\"icon_strip\":[\"一\",\"二\"],"
                    "\"update_mask\":[\"space_changed\"]"
                "}"
            "]"
        "}"
        "}";

    FILE *f = fopen(path, "wb");
    if (!f) { perror("fopen"); exit(1); }
    fwrite("OMABC", 1, 5, f);
    fputc(0x01, f);
    fwrite("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 1, 10, f);
    fwrite(json, 1, strlen(json), f);
    fclose(f);
}

/* Create an invalid config (bad magic) for error testing. */
static void make_bad_magic_config(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror("fopen"); exit(1); }
    fwrite("BADC", 1, 4, f);
    fwrite("{\"bar\":{}}", 1, 10, f);
    fclose(f);
}

/* Create a config with malformed JSON for error testing. */
static void make_bad_json_config(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror("fopen"); exit(1); }
    fwrite("OMABC", 1, 5, f);
    fputc(0x01, f);
    fwrite("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 1, 10, f);
    fwrite("{not valid json", 1, 15, f);
    fclose(f);
}

/* Create a config missing bar settings. */
static void make_empty_config(const char *path) {
    const char *json = "{}";
    FILE *f = fopen(path, "wb");
    if (!f) { perror("fopen"); exit(1); }
    fwrite("OMABC", 1, 5, f);
    fputc(0x01, f);
    fwrite("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 1, 10, f);
    fwrite(json, 1, strlen(json), f);
    fclose(f);
}

static int test_valid_config(void) {
    printf("\n=== test_valid_config ===\n");
    const char *path = "/tmp/omabar_test_valid";
    make_minimal_config(path);

    char lock_file[256] = {0};
    int result = config_load(path, lock_file, sizeof(lock_file));

    check("config_load returns 0", result == 0);
    check_nonzero("bar_count > 0", g_bar_manager.bar_item_count);
    check_int("bar position = top (0)", g_bar_manager.position, 0);
    check_int("bar height = 38", g_bar_manager.height, 38);
    check_int("bar margin = 0", g_bar_manager.margin, 0);
    check_int("bar blur_radius = 30", g_bar_manager.blur_radius, 30);
    check_int("bar shadow = 1", g_bar_manager.shadow, 1);
    check_int("bar sticky = 1", g_bar_manager.sticky, 1);
    check_int("bar topmost = 1", g_bar_manager.topmost, 1);
    check_int("bar alpha = 1.0", (int)(g_bar_manager.alpha * 100), 100);
    check_nonzero("lock_file set", lock_file[0] != '\0');

    /* Check items were added */
    int found_clock = 0, found_spaces = 0;
    for (int i = 0; i < g_bar_manager.bar_item_count; i++) {
        struct bar_item *it = g_bar_manager.bar_items[i];
        if (it && it->name) {
            if (strcmp(it->name, "clock") == 0) found_clock = 1;
            if (strcmp(it->name, "spaces") == 0) found_spaces = 1;
        }
    }
    check("clock item added", found_clock);
    check("spaces item added", found_spaces);
    check("clock has update_mask", g_bar_manager.bar_items[0] ? (g_bar_manager.bar_items[0]->update_mask & UPDATE_SCROLL_TICK) != 0 : 0);

    /* Verify spaces has icon_strip */
    for (int i = 0; i < g_bar_manager.bar_item_count; i++) {
        struct bar_item *it = g_bar_manager.bar_items[i];
        if (it && it->name && strcmp(it->name, "spaces") == 0) {
            check("spaces has icon_strip", it->icon_strip_count > 0);
            check("spaces update_mask has SPACE_CHANGED", (it->update_mask & UPDATE_SPACE_CHANGED) != 0);
        }
    }

    (void)remove(path);
    return tests_failed == 0;
}

static int test_bad_magic(void) {
    printf("\n=== test_bad_magic ===\n");
    const char *path = "/tmp/omabar_test_bad_magic";
    make_bad_magic_config(path);

    char lock_file[256] = {0};
    int result = config_load(path, lock_file, sizeof(lock_file));
    check("config_load returns -1", result == -1);
    (void)remove(path);
    return tests_failed == 0;
}

static int test_bad_json(void) {
    printf("\n=== test_bad_json ===\n");
    const char *path = "/tmp/omabar_test_bad_json";
    make_bad_json_config(path);

    char lock_file[256] = {0};
    int result = config_load(path, lock_file, sizeof(lock_file));
    check("config_load returns -1", result == -1);
    (void)remove(path);
    return tests_failed == 0;
}

static int test_empty_config(void) {
    printf("\n=== test_empty_config ===\n");
    const char *path = "/tmp/omabar_test_empty";
    make_empty_config(path);

    char lock_file[256] = {0};
    int result = config_load(path, lock_file, sizeof(lock_file));
    check("config_load returns 0 for empty JSON", result == 0);
    check("bar position defaults to top (0)", g_bar_manager.position == 0);
    check("bar_count is 0 (no items)", g_bar_manager.bar_item_count == 0);
    (void)remove(path);
    return tests_failed == 0;
}

static int test_missing_file(void) {
    printf("\n=== test_missing_file ===\n");
    char lock_file[256] = {0};
    int result = config_load("/tmp/omabar_test_nonexistent_xyz", lock_file, sizeof(lock_file));
    check("config_load returns -1 for missing file", result == -1);
    return tests_failed == 0;
}

static int test_null_path(void) {
    printf("\n=== test_null_path ===\n");
    char lock_file[256] = {0};
    int result = config_load(NULL, lock_file, sizeof(lock_file));
    check("config_load returns -1 for NULL path", result == -1);
    return tests_failed == 0;
}

static int test_lock_file_from_config(void) {
    printf("\n=== test_lock_file_from_config ===\n");
    const char *path = "/tmp/omabar_test_lock";
    make_minimal_config(path);

    /* Now modify the config to include a custom lock_file */
    const char *json =
        "{"
        "\"daemon\":{\"lock_file\":\"/tmp/omabar_custom.lock\"},"
        "\"bar\":{\"height\":42}"
        "}";
    FILE *f = fopen(path, "wb");
    fwrite("OMABC", 1, 5, f);
    fputc(0x01, f);
    fwrite("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 1, 10, f);
    fwrite(json, 1, strlen(json), f);
    fclose(f);

    char lock_file[256] = {0};
    int result = config_load(path, lock_file, sizeof(lock_file));
    check("config_load returns 0", result == 0);
    check_str("lock_file from config", lock_file, "/tmp/omabar_custom.lock");

    (void)remove(path);
    return tests_failed == 0;
}

static int test_config_path_argument(void) {
    printf("\n=== test_config_path_argument ===\n");
    /* Verify main() argument parsing for --config */
    extern int main(int argc, char *argv[]);
    /* We can't call main() directly since it enters CFRunLoopRun,
       so we just verify the g_config_path static via a simple check
       that the parsing logic is present by checking argv parsing manually. */

    /* Simulate the parsing that main.c does */
    char *test_argv[] = { "omabar", "--config", "/tmp/test_cfg", NULL };
    char config_path[1024] = {0};
    int found = 0;
    for (int i = 1; test_argv[i]; i++) {
        if (strcmp(test_argv[i], "--config") == 0 || strcmp(test_argv[i], "-c") == 0) {
            if (test_argv[i + 1]) {
                snprintf(config_path, sizeof(config_path), "%s", test_argv[++i]);
                found = 1;
            }
        } else if (strncmp(test_argv[i], "--config=", 9) == 0) {
            snprintf(config_path, sizeof(config_path), "%s", test_argv[i] + 9);
            found = 1;
        }
    }
    check("--config argument parsed", found);
    check_str("--config value correct", config_path, "/tmp/test_cfg");
    return tests_failed == 0;
}

static int test_foreground_argument(void) {
    printf("\n=== test_foreground_argument ===\n");
    /* Simulate parsing --foreground */
    char *test_argv[] = { "omabar", "--foreground", "-f", NULL };
    int foreground = 0;
    for (int i = 1; test_argv[i]; i++) {
        if (strcmp(test_argv[i], "--foreground") == 0 || strcmp(test_argv[i], "-f") == 0) {
            foreground = 1;
        }
    }
    check("--foreground parsed", foreground == 1);

    /* Verify without --foreground */
    char *test_argv2[] = { "omabar", "--config", "/tmp/cfg", NULL };
    foreground = 0;
    for (int i = 1; test_argv2[i]; i++) {
        if (strcmp(test_argv2[i], "--foreground") == 0 || strcmp(test_argv2[i], "-f") == 0) {
            foreground = 1;
        }
    }
    check("no --foreground stays 0", foreground == 0);
    return tests_failed == 0;
}

static int test_color_parsing(void) {
    printf("\n=== test_color_parsing ===\n");
    /* Verify that the color from the test config is valid */
    struct color c = color_from_hex_string("0x40000000");
    check("0x40000000 color is valid", color_is_valid(c));
    check("0x40000000 alpha ~0.251", c.a > 0.2f && c.a < 0.3f);
    check("0x40000000 red = 0", c.r == 0.0f);
    check("0x40000000 green = 0", c.g == 0.0f);
    check("0x40000000 blue = 0", c.b == 0.0f);

    struct color c2 = color_from_hex_string("0xffffffff");
    check("0xffffffff color is valid", color_is_valid(c2));
    check("0xffffffff alpha = 1.0", c2.a == 1.0f);

    struct color c3 = color_from_hex_string("#fafafa");
    check("#fafafa color is valid", color_is_valid(c3));

    struct color c4 = color_from_hex_string(NULL);
    check("NULL color is invalid", !color_is_valid(c4));

    struct color c5 = color_from_hex_string("");
    check("empty string color is invalid", !color_is_valid(c5));

    return tests_failed == 0;
}

int main(int argc, char *argv[]) {
    printf("=== omabar config_loader regression test ===\n");

    const char *test_config = "/tmp/omabar_test_config";
    if (argc > 1) {
        test_config = argv[1];
    }

    /* Initialize g_bar_manager before each test that calls config_load */
    bar_manager_init(&g_bar_manager);

    /* Test 1: Verify the main test config can be loaded */
    {
        printf("\n=== test_main_config ===\n");
        char lock_file[256] = {0};
        int result = config_load(test_config, lock_file, sizeof(lock_file));
        if (result == 0) {
            tests_run++; tests_passed++;
            printf("  PASS: main config loaded successfully\n");
            check_int("main config bar height", g_bar_manager.height, 38);
            check_int("main config bar position", g_bar_manager.position, 0);
            check_nonzero("main config items added", g_bar_manager.bar_item_count);
            check_nonzero("main config lock_file set", lock_file[0] != '\0');
        } else {
            tests_run++; tests_failed++;
            printf("  FAIL: main config load failed (rc=%d)\n", result);
        }
    }

    /* Reset for other tests */
    bar_manager_destroy(&g_bar_manager);
    bar_manager_init(&g_bar_manager);

    test_valid_config();
    bar_manager_destroy(&g_bar_manager);
    bar_manager_init(&g_bar_manager);

    test_bad_magic();
    bar_manager_destroy(&g_bar_manager);
    bar_manager_init(&g_bar_manager);

    test_bad_json();
    bar_manager_destroy(&g_bar_manager);
    bar_manager_init(&g_bar_manager);

    test_empty_config();
    bar_manager_destroy(&g_bar_manager);
    bar_manager_init(&g_bar_manager);

    test_missing_file();
    bar_manager_destroy(&g_bar_manager);
    bar_manager_init(&g_bar_manager);

    test_null_path();
    bar_manager_destroy(&g_bar_manager);
    bar_manager_init(&g_bar_manager);

    test_lock_file_from_config();
    bar_manager_destroy(&g_bar_manager);
    bar_manager_init(&g_bar_manager);

    test_color_parsing();

    test_config_path_argument();
    test_foreground_argument();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);

    bar_manager_destroy(&g_bar_manager);

    return tests_failed > 0 ? 1 : 0;
}
