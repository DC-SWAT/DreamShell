/** \file    naomi/coins.c
    \brief   NAOMI coin/credit helpers.
    \ingroup naomi

    This file is part of DreamShell.

    Copyright (C) 2026 SWAT
*/

#include <arch/arch.h>
#include <dc/maple/mie.h>

#include <naomi/bsram.h>
#include <naomi/coins.h>
#include <sfx.h>

static naomi_coin_callback_t naomi_coin_cb;
static void *naomi_coin_cb_user;

static int naomi_coins_readable(void) {
    return hardware_sys_mode(NULL) != HW_TYPE_RETAIL;
}

static int naomi_coin_input_available(void) {
    return mie_port0_mode() == MIE_PORT0_JVS;
}

static uint32_t naomi_coins_raw(void) {
    if(!naomi_coins_readable()) {
        return 0;
    }

    return naomi_bsram_read_credits();
}

uint32_t naomi_coins_count(void) {
    uint32_t total;

    total = naomi_coins_raw();
    if(total > NAOMI_COINS_MAX) {
        total = NAOMI_COINS_MAX;
    }

    return total;
}

void naomi_coin_set_callback(naomi_coin_callback_t cb, void *user) {
    naomi_coin_cb = cb;
    naomi_coin_cb_user = user;
}

static bool naomi_coins_store(uint8_t slot, uint32_t amount) {
    uint32_t credits;

    if(amount == 0 || !naomi_coins_readable()) {
        return false;
    }

    credits = naomi_coins_raw();
    if(credits >= NAOMI_COINS_MAX) {
        return false;
    }

    if(amount > NAOMI_COINS_MAX - credits) {
        amount = NAOMI_COINS_MAX - credits;
    }

    if(!naomi_bsram_add_credits(amount)) {
        return false;
    }

    if(naomi_coin_input_available() && slot < MIE_JVS_COIN_SLOTS) {
        mie_coin_decrease(slot, (uint16_t)amount, false);
    }

    return true;
}

static void naomi_coins_notify(void) {
    ds_sfx_play(DS_SFX_COIN);

    if(naomi_coin_cb) {
        naomi_coin_cb(naomi_coins_count(), naomi_coin_cb_user);
    }
}

bool naomi_coin_on_meter(uint8_t slot) {
    if(!naomi_coin_input_available()) {
        return false;
    }

    if(slot >= MIE_JVS_COIN_SLOTS) {
        return false;
    }

    if(!naomi_coins_store(slot, 1)) {
        return false;
    }

    naomi_coins_notify();
    return true;
}

bool naomi_coin_service(uint8_t slot) {
    if(!naomi_coin_input_available()) {
        return false;
    }

    if(slot >= MIE_JVS_COIN_SLOTS) {
        return false;
    }

    if(!naomi_coins_store(slot, 1)) {
        return false;
    }

    naomi_coins_notify();
    return true;
}
