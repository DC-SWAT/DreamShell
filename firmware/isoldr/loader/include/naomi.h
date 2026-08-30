/**
 * DreamShell ISO Loader
 * NAOMI
 * (c)2025-2026 SWAT <http://www.dc-swat.ru>
 */

#ifndef _NAOMI_H
#define _NAOMI_H

#include <stdint.h>
#include <stdbool.h>
#include "syscalls.h"

#define NAOMI_ID(a, b, c, d) \
    ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#define NAOMI_ID_BAD0 NAOMI_ID('B', 'A', 'D', '0') /* Zombie Revenge */
#define NAOMI_ID_BAL1 NAOMI_ID('B', 'A', 'L', '1') /* Dead or Alive 2 Millennium */
#define NAOMI_ID_BAU0 NAOMI_ID('B', 'A', 'U', '0') /* Jambo Safari */
#define NAOMI_ID_BAC0 NAOMI_ID('B', 'A', 'C', '0') /* Crazy Taxi */
#define NAOMI_ID_ABC0 NAOMI_ID('A', 'B', 'C', '0') /* Power Stone */
#define NAOMI_ID_BBJ0 NAOMI_ID('B', 'B', 'J', '0') /* Power Stone 2 */
#define NAOMI_ID_BBK0 NAOMI_ID('B', 'B', 'K', '0') /* 18 Wheeler */
#define NAOMI_ID_BCV0 NAOMI_ID('B', 'C', 'V', '0') /* Gundam Federation vs Zeon */
#define NAOMI_ID_BDS0 NAOMI_ID('B', 'D', 'S', '0') /* Virtua Tennis 2 */
#define NAOMI_ID_BDF0 NAOMI_ID('B', 'D', 'F', '0') /* Monkey Ball */
#define NAOMI_ID_BCW0 NAOMI_ID('B', 'C', 'W', '0') /* Heavy Metal Geomatrix */

typedef struct naomi_ingame_test {
    uint32_t id;
    uint32_t hook;
    uint32_t task;
    uint32_t watch;
    uint32_t exit;
    uint32_t jump;
} naomi_ingame_test_t;

typedef enum naomi_test {
    NAOMI_TEST_NONE = 0,
    NAOMI_TEST_MENU,
    NAOMI_TEST_HOOK
} naomi_test_t;

typedef struct naomi_state {
    uint32_t game_id;
    uint32_t win_off;
    naomi_test_t test;
    bool have_win;
    bool aw;
} naomi_state_t;

naomi_state_t *get_naomi(void);

#define NAOMI_CART_DMA_STATUS_ADDR   (RAM_START_ADDR + 0xac)
#define NAOMI_CART_REGION_ADDR       (RAM_START_ADDR + 0x1f100)
#define NAOMI_CART_ADVSND_ADDR       (RAM_START_ADDR + 0x1f104)
#define NAOMI_CART_MONITOR_ADDR      (RAM_START_ADDR + 0x1f108)
#define NAOMI_CART_DISPLAY_ADDR      (RAM_START_ADDR + 0x1f10c)
#define NAOMI_CART_GAMEID_ADDR       (RAM_START_ADDR + 0x1f110)
#define NAOMI_CART_TEST_ADDR         (RAM_START_ADDR + 0x1f120)
#define NAOMI_BOOTID_ADDR            (RAM_START_ADDR + 0x1f400)
#define NAOMI_BOOT_FLAG_ADDR         (RAM_START_ADDR + 0x1ff00)
#define NAOMI_BOOTSVC_ADDR           (RAM_START_ADDR + 0x18000)
#define NAOMI_TEST_ENTER_ADDR        (NAOMI_BOOTSVC_ADDR - 4)
#define NAOMI_GAME_ENTER_ADDR        (NAOMI_BOOTSVC_ADDR - 8)
#define NAOMI_BOOTSVC_JUMP_INDEX     16
#define NAOMI_BOOTSVC_JUMP_COUNT     4

uint32_t naomi_load_bin(int test_mode);
void naomi_setup_env(void);
void naomi_hook_boot_services(uint32_t test_enter, uint32_t game_enter);
void naomi_bad0_test_svc(void);
void naomi_aw_prepare(void);
void naomi_enter_test(void);
void naomi_enter_game(void);
void naomi_cart_read(gdc_cart_read_params_t *params);
void naomi_cart_win_read(void *dst, uint32_t size, uint32_t file_base);
void naomi_cart_stack_setup(void);
void naomi_patch_cart_read(uint8_t *dst, uint32_t size);
int naomi_is_aw_stub(uint8_t *dst, uint32_t size);
void naomi_aw_patch_stub(uint8_t *dst, uint32_t size);
const naomi_ingame_test_t *naomi_ingame_test_by_id(const char *id);
const naomi_ingame_test_t *naomi_ingame_test_match(const char *id);
int naomi_cart_wait_dma(void);
void naomi_cart_win_hook(void);
void naomi_aw_gdst_read(void *dst, uint32_t size, uint32_t addr);
void naomi_aw_gdst_xor(void *dst, uint32_t size, uint32_t addr, uint32_t key_off);

#endif
