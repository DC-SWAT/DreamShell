/** \file    naomi/coins.h
    \brief   NAOMI coin/credit helpers.
    \ingroup naomi

    This file is part of DreamShell.

    Copyright (C) 2026 SWAT
*/

#ifndef __NAOMI_COINS_H
#define __NAOMI_COINS_H

#include <stdbool.h>
#include <stdint.h>

#define NAOMI_COINS_MAX 9

typedef void (*naomi_coin_callback_t)(uint32_t count, void *user);

uint32_t naomi_coins_count(void);
bool naomi_coin_on_meter(uint8_t slot);
bool naomi_coin_service(uint8_t slot);
void naomi_coin_set_callback(naomi_coin_callback_t cb, void *user);

#endif
