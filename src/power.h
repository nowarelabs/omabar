#ifndef OMABAR_POWER_H
#define OMABAR_POWER_H

#include <stdbool.h>

void forced_power_event(void);
void power_begin(void);
void power_end(void);
int power_get_charge(void);
bool power_is_charging(void);

#endif