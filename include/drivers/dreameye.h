/** 
 * \file    dreameye.h
 * \brief   Dreameye driver extension
 * \date    2015, 2023, 2024, 2025
 * \author  SWAT www.dc-swat.ru
 */

#ifndef __DS_DREAMEYE_H
#define __DS_DREAMEYE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <arch/types.h>
#include <dc/maple.h>
#include <dc/maple/dreameye.h>

/** \brief  Type for a dreameye capture callback.

    This is the signature that is required for a function to accept frames
    from the dreameye as it is capturing. This function will be called about
    once per frame, and in an interrupt context (so it should be pretty quick
    to execute). Basically, all you should do in one of these is copy the
    frame out to your own buffer -- do not do any processing on the frames
    in your callback other than to copy them out!

    \param  dev             The device the frames are coming from.
    \param  frame           Pointer to the frame buffer.
    \param  len             The number of bytes in the frame buffer.

    \headerfile dc/maple/dreameye.h
*/
typedef void (*dreameye_frame_cb)(maple_device_t *dev, uint8_t *frame, size_t len);

/** \brief  Dreameye status structure.

    This structure contains information about the status of the Camera device
    and can be fetched with maple_dev_status(). You should not change any of
    this information, it should all be considered read-only.
*/
typedef struct dreameye_state_ext {
    /** \brief  The number of images on the device. */
    int             image_count;

    /** \brief  Is the image_count field valid? */
    int             image_count_valid;

    /** \brief  The number of transfer operations required for the selected
                image. */
    int             transfer_count;

    /** \brief  Is an image transferring now? */
    int             img_transferring;

    /** \brief  Storage for image data. */
    uint8_t         *img_buf;

    /** \brief  The size of the image in bytes. */
    int             img_size;

    /** \brief  The image number currently being transferred. */
    uint8_t          img_number;
	
    /** \brief  The value from/to subsystems. */
    int             value;

    /** \brief  Frame size in capture mode. */
    int             width;
    int             height;
    int             frame_size;

    /** \brief  Frame pixel format. */
    int             format;

    /** \brief  Is a frame compressed. */
    int             compressed;

    /** \brief  Capturing is in progress. */
    int             is_capturing;

    /** \brief  Frame capture callback. */
    dreameye_frame_cb callback;

    /** \brief  Last frame request time in NS */
    uint64_t last_request;

} dreameye_state_ext_t;

typedef struct dreameye_preview {

    int isp_mode;
    int bpp;

    int fullscreen;
    int scale;
    int x;
    int y;

    dreameye_frame_cb callback;

} dreameye_preview_t;


/** \brief  Read/write CMOS Image Sensor registers. */
#define DREAMEYE_COND_REG_CIS    0x00

/** \brief  Read/write Image Signal Processor registers. */
#define DREAMEYE_COND_REG_ISP    0x10

/** \brief  Read/write JangGu Compression Engine registers. */
#define DREAMEYE_COND_REG_JANGGU 0x20

/** \brief  Read/write JPEG Compression Engine registers. */
#define DREAMEYE_COND_REG_JPEG   0x21 // Supported by Dreameye???

/**
 * Really clock frequency request for each subsystem, 
 * but Dreameye response only for maple bus (use 0x90 as argument too)
 */
#define DREAMEYE_COND_MAPLE_BITRATE    0x90
#define DREAMEYE_GETCOND_RESOLUTION    0x91
#define DREAMEYE_GETCOND_COMPRESS_FMT  0x92
#define DREAMEYE_COND_FLASH_TOTAL      0x94
#define DREAMEYE_COND_FLASH_REMAIN     0x96

#define DREAMEYE_DATA_IMAGE   0x00
#define DREAMEYE_DATA_PROGRAM 0xC1

#define DREAMEYE_ISP_MODE_QSIF 0x00 /* 160x120 */
#define DREAMEYE_ISP_MODE_QCIF 0x01 /* 176x144 */
#define DREAMEYE_ISP_MODE_SIF  0x02 /* 320x240 */
#define DREAMEYE_ISP_MODE_CIF  0x03 /* 352x288 */
#define DREAMEYE_ISP_MODE_VGA  0x04 /* 640x480 */

#define DREAMEYE_FRAME_FMT_YUV420DE 0 /* JangGu native 12bpp */
#define DREAMEYE_FRAME_FMT_YUYV422 1 /* JangGu native 16bpp */
#define DREAMEYE_FRAME_FMT_YUV420P 2 /* Converted from yuv420de */
#define DREAMEYE_FRAME_FMT_NV21 3 /* Converted from yuv420de */
#define DREAMEYE_FRAME_FMT_YUV420DE_COMPRESSED 10 /* Unknown compressed format */
#define DREAMEYE_FRAME_FMT_YUYV422_COMPRESSED 11 /* Unknown compressed format */

/** \brief  Transfer a captured frame from the Dreameye.

    This function fetches a single image from the specified Dreameye device.
    This function will block, and can take a little while to execute. You must
    use the first subdevice of the MAPLE_FUNC_CONTROLLER root device of the
    Dreameye as the dev parameter.

    \param  dev             The device to get an image from.
    \param  data            A pointer to a buffer to store things in. This
                            will be allocated by the function and you are
                            responsible for freeing the data when you are done.
    \param  img_sz          A pointer to storage for the size of the image, in
                            bytes.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EFAIL     On error.
*/
int dreameye_get_video_frame(maple_device_t *dev, uint8_t **data, int *img_sz);

/** \brief  Request a captured frame from the Dreameye.
 * 
 */
int dreameye_req_video_frame(maple_device_t *dev);

/** \brief  Get params from any subsystem
 * TODO
 */
int dreameye_get_param(maple_device_t *dev, uint8_t param, uint8_t arg, uint16_t *value);

/** \brief  Set params for any subsystem
 * 
 */
int dreameye_set_param(maple_device_t *dev, uint8_t param, uint8_t arg, uint16_t value);

/** \brief  Add to queue params for any subsystem
 * 
 */
int dreameye_queue_param(maple_device_t *dev, uint8_t param, uint8_t arg, uint16_t value);

/** \brief  Setup CIS and ISP regs for video capturing
 * 
 */
int dreameye_setup_video_camera(maple_device_t *dev, int isp_mode, int format);

/** \brief  Stop video capture
 * 
 */
int dreameye_stop_video_camera(maple_device_t *dev);

/** \brief  Initialize preview
 * 
 */
int dreameye_preview_init(maple_device_t *dev, dreameye_preview_t *params);

/** \brief  Shutdown preview
 * 
 */
void dreameye_preview_shutdown(maple_device_t *dev);

/** \brief  Start frames capturing
 * 
 */
int dreameye_start_capturing(maple_device_t *dev, dreameye_frame_cb cb);

/** \brief  Stop frames capturing
 * 
 */
int dreameye_stop_capturing(maple_device_t *dev);

/** \brief  Polling device
 * 
 */
int dreameye_poll(maple_device_t *dev);

/** \brief  Get extended state from device
 * 
 */
dreameye_state_ext_t *dreameye_get_state_ext(maple_device_t *dev);

__END_DECLS

#endif  /* __DS_DREAMEYE_H */
