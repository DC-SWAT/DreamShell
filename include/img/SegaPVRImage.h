//
//  SegaPVRImage.h
//  SEGAPVR
//
//  Created by Yevgeniy Logachev on 4/13/14.
//  Copyright (c) 2014 yev. All rights reserved.
//  Copyright (c) 2024 Maniac Vera
//

#ifndef SEGAPVRIMAGE_H
#define SEGAPVRIMAGE_H

enum TextureTypeMasks
{
    TTM_Twiddled                             = 0x01,
    TTM_TwiddledMipMaps                      = 0x02,
    TTM_VectorQuantized                      = 0x03,
    TTM_VectorQuantizedMipMaps               = 0x04,
    TTM_VectorQuantizedCustomCodeBook        = 0x10,
    TTM_VectorQuantizedCustomCodeBookMipMaps = 0x11,
    TTM_Raw                                  = 0x0B,
    TTM_RawNonSquare                         = 0x09,
    TTM_TwiddledNonSquare                    = 0x0D
};

enum TextureFormatMasks
{
    TFM_ARGB1555 = 0x00,
    TFM_RGB565,
    TFM_ARGB4444,
    TFM_YUV422
};

#pragma pack(push,4)
struct GBIXHeader                       // Global index header
{
    unsigned int        version;        // ID, "GBIX" in ASCII
    unsigned int        nextTagOffset;  // Bytes number to the next tag
    unsigned long long	globalIndex;
};
#pragma pack(pop)

#pragma pack(push,4)
struct PVRTHeader
{
    unsigned int        version;            // ID, "PVRT" in ASCII
    unsigned int        textureDataSize;
    unsigned int        textureAttributes;
    unsigned short      width;              // Width of the texture.
    unsigned short      height;             // Height of the texture.
};
#pragma pack(pop)


extern void             BuildTwiddleTable();
extern int              LoadPVRFromFile(const char* filename, unsigned char** image, unsigned long int* imageSize, struct PVRTHeader* outPvrtHeader);
extern unsigned int     ReadPVRHeader(unsigned char* srcData, struct PVRTHeader* pvrtHeader);
extern int              DecodePVR(unsigned char* srcData, const struct PVRTHeader* pvrtHeader, unsigned char* dstData);

#endif
