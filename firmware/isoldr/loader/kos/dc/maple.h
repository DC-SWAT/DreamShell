/**
 * DreamShell ISO Loader
 * Maple bus
 * (c)2023 SWAT <http://www.dc-swat.ru>
 */

#include <sys/cdefs.h>
#include <arch/types.h>

#ifndef __MAPLE_H
#define __MAPLE_H

#define MAPLE_REG(x) (*(vuint32 *)(x))
#define MAPLE_BASE 0xa05f6c00
#define MAPLE_DMA_ADDR (MAPLE_BASE + 0x04)
#define MAPLE_DMA_STATUS (MAPLE_BASE + 0x18)

/** \defgroup maple_cmds            Maple commands and responses

    These are all either commands or responses to commands sent to or from Maple
    in normal operation.

    @{
*/
#define MAPLE_RESPONSE_FILEERR      -5  /**< \brief File error */
#define MAPLE_RESPONSE_AGAIN        -4  /**< \brief Try again later */
#define MAPLE_RESPONSE_BADCMD       -3  /**< \brief Bad command sent */
#define MAPLE_RESPONSE_BADFUNC      -2  /**< \brief Bad function code */
#define MAPLE_RESPONSE_NONE         -1  /**< \brief No response */
#define MAPLE_COMMAND_DEVINFO       1   /**< \brief Device info request */
#define MAPLE_COMMAND_ALLINFO       2   /**< \brief All info request */
#define MAPLE_COMMAND_RESET         3   /**< \brief Reset device request */
#define MAPLE_COMMAND_KILL          4   /**< \brief Kill device request */
#define MAPLE_RESPONSE_DEVINFO      5   /**< \brief Device info response */
#define MAPLE_RESPONSE_ALLINFO      6   /**< \brief All info response */
#define MAPLE_RESPONSE_OK           7   /**< \brief Command completed ok */
#define MAPLE_RESPONSE_DATATRF      8   /**< \brief Data transfer */
#define MAPLE_COMMAND_GETCOND       9   /**< \brief Get condition request */
#define MAPLE_COMMAND_GETMINFO      10  /**< \brief Get memory information */
#define MAPLE_COMMAND_BREAD         11  /**< \brief Block read */
#define MAPLE_COMMAND_BWRITE        12  /**< \brief Block write */
#define MAPLE_COMMAND_BSYNC         13  /**< \brief Block sync */
#define MAPLE_COMMAND_SETCOND       14  /**< \brief Set condition request */
#define MAPLE_COMMAND_MICCONTROL    15  /**< \brief Microphone control */
#define MAPLE_COMMAND_CAMCONTROL    17  /**< \brief Camera control */
/** @} */

/** \defgroup maple_functions       Maple device function codes

    This is the list of maple device types (function codes). Each device must
    have at least one function to actually do anything.

    @{
*/

/* Function codes; most sources claim that these numbers are little
   endian, and for all I know, they might be; but since it's a bitmask
   it doesn't really make much different. We'll just reverse our constants
   from the "big-endian" version. */
#define MAPLE_FUNC_PURUPURU     0x00010000  /**< \brief Jump pack */
#define MAPLE_FUNC_MOUSE        0x00020000  /**< \brief Mouse */
#define MAPLE_FUNC_CAMERA       0x00080000  /**< \brief Camera (Dreameye) */
#define MAPLE_FUNC_CONTROLLER   0x01000000  /**< \brief Controller */
#define MAPLE_FUNC_STORAGE      0x02000000  /**< \brief Storage */
#define MAPLE_FUNC_LCD          0x04000000  /**< \brief LCD screen */
#define MAPLE_FUNC_CLOCK        0x08000000  /**< \brief Clock */
#define MAPLE_FUNC_MICROPHONE   0x10000000  /**< \brief Microphone */
#define MAPLE_FUNC_ARGUN        0x20000000  /**< \brief AR gun? */
#define MAPLE_FUNC_KEYBOARD     0x40000000  /**< \brief Keyboard */
#define MAPLE_FUNC_LIGHTGUN     0x80000000  /**< \brief Lightgun */
/** @} */

/** \brief  Maple frame structure.

    \headerfile dc/maple.h
*/
typedef struct {
	int8 cmd;			/* command (defined above) */
	uint8 to;			/* recipient address */
	uint8 from;			/* sender address */
	uint8 datalen;			/* length in words of data */
	void *data;			/* ptr to parameter data */
} maple_frame_t;


/** \brief  Maple device info structure.

    This structure is used by the hardware to deliver the response to the device
    info request.

    \headerfile dc/maple.h
*/
typedef struct maple_devinfo {
    uint32  functions;              /**< \brief Function codes supported */
	uint32  function_data[3];       /**< \brief Additional data per function */
    uint8   area_code;              /**< \brief Region code */
    uint8   connector_direction;    /**< \brief 0: UP (most controllers), 1: DOWN (lightgun, microphones) */
    char    product_name[30];       /**< \brief Name of device */
    char    product_license[60];    /**< \brief License statement */
    uint16  standby_power;          /**< \brief Power consumption (standby) */
    uint16  max_power;              /**< \brief Power consumption (max) */
} maple_devinfo_t;


#endif /* __MAPLE_H */
