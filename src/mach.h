#ifndef OMABAR_MACH_H
#define OMABAR_MACH_H

#include <stdbool.h>
#include <stdint.h>

#include <mach/mach.h>
#include <mach/message.h>
#include <bootstrap.h>

/* bootstrap name lookup (client side of a Mach service) */
mach_port_t mach_get_bs_port(const char *bs_name);

/* send a buffer of bytes to a mach port as one OOL descriptor.
   Returns a heap response string when await_response is set, else NULL. */
char *mach_send_message(mach_port_t port, char *message,
                        uint32_t len, bool await_response);

#endif