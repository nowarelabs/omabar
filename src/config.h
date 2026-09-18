#ifndef OMABAR_CONFIG_H
#define OMABAR_CONFIG_H

#include <stddef.h>
#include <stdbool.h>

/*
 * Load an OMABC binary configuration file from `path`.
 * Validates the 'OMABC' magic header (5 bytes), parses the JSON payload,
 * applies bar parameters and items to `g_bar_manager`, and copies
 * daemon.lock_file into `out_lock_file` if present and non-empty.
 *
 * Returns 0 on success, or -1 on failure (error printed to stderr).
 */
int config_load(const char *path, char *out_lock_file, size_t lock_file_size);

#endif
