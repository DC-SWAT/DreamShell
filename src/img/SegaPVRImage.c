//
//  SegaPVRImage.c
//  SEGAPVR
//
//  Created by Yevgeniy Logachev on 4/13/14.
//  Copyright (c) 2014 yev. All rights reserved.
//  Copyright (C) 2024-2025 Maniac Vera
//

#include "SegaPVRImage.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#if defined(__DREAMCAST__)
#include <malloc.h>
#include <kos/fs.h>
#endif

// Twiddle
static const unsigned int kTwiddleTableSize = 1024;
unsigned long int gTwiddledTable[1024];

unsigned long int GetUntwiddledTexelPosition(unsigned long int x, unsigned long int y);

int MipMapsCountFromWidth(unsigned long int width);

// RGBA Utils
void TexelToRGBA( unsigned short int srcTexel, enum TextureFormatMasks srcFormat, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a);

unsigned int ToUint16(unsigned char* value)
{
    return (value[0] | (value[1] << 8));
}

int LoadPVRFromFile(const char* filename, unsigned char** image, unsigned long int* imageSize, struct PVRTHeader* outPvrtHeader)
{
    file_t pFile =  fs_open(filename, O_RDONLY);
    if (pFile == FILEHND_INVALID)
    {
        return 0;
    }
    
    size_t fsize = fs_total(pFile);    
    unsigned char* buff  = (unsigned char *)memalign(32, fsize);
    fs_read(pFile, buff, fsize);
    fs_close(pFile);
    
    unsigned char* srcPtr = buff;
    
    struct PVRTHeader pvrtHeader;
    unsigned int offset = ReadPVRHeader(srcPtr, &pvrtHeader);
    if (offset == 0)
    {
        free(buff);
        return 0;
    }

    if (outPvrtHeader)
    {
        *outPvrtHeader = pvrtHeader;
    }
    
    printf("Image size: (%d, %d)\n", pvrtHeader.width, pvrtHeader.height);
    
    srcPtr += offset;
    
    *imageSize = pvrtHeader.width * pvrtHeader.height * 4; // RGBA8888
    *image = (unsigned char *)malloc(*imageSize);
    memset(*image, 0, *imageSize);
    
    DecodePVR(srcPtr, &pvrtHeader, *image);
    
    free(buff);
    
    return 1;
}

unsigned int ReadPVRHeader(unsigned char* srcData, struct PVRTHeader* pvrtHeader)
{
    unsigned int offset = 0;
    struct GBIXHeader gbixHeader = {0};
    if (strncmp((char*)srcData, "GBIX", 4) == 0)
    {
        memcpy(&gbixHeader, srcData, 8);
        offset += 8;

        memcpy(&gbixHeader.globalIndex, srcData + offset, gbixHeader.nextTagOffset);
        offset += gbixHeader.nextTagOffset;
    }
    
    memcpy(pvrtHeader, srcData + offset, sizeof(struct PVRTHeader));
    offset += sizeof(struct PVRTHeader);
    
    return offset;
}

int DecodePVR16bit(unsigned char* srcData, const struct PVRTHeader* pvrtHeader, unsigned char* dstData)
{
    const unsigned int kSrcStride = sizeof(unsigned short int);  // 16-bit source texel size
    const unsigned int kDstStride = sizeof(uint16_t);            // 16-bit destination texel size

    enum TextureFormatMasks srcFormat = (enum TextureFormatMasks)(pvrtHeader->textureAttributes & 0xFF);

    // Only support 16-bit formats
    if (srcFormat != TFM_RGB565 && srcFormat != TFM_ARGB1555 && srcFormat != TFM_ARGB4444)
        return 0;

    bool isTwiddled = false;
    bool isMipMaps = false;
    bool isVQCompressed = false;
    unsigned int codeBookSize = 0;

    switch ((pvrtHeader->textureAttributes >> 8) & 0xFF)
    {
        case TTM_TwiddledMipMaps:
            isMipMaps = true;
            // fallthrough
        case TTM_Twiddled:
        case TTM_TwiddledNonSquare:
            isTwiddled = true;
            break;

        case TTM_VectorQuantizedMipMaps:
            isMipMaps = true;
            // fallthrough
        case TTM_VectorQuantized:
            isVQCompressed = true;
            codeBookSize = 256;
            break;

        case TTM_VectorQuantizedCustomCodeBookMipMaps:
            isMipMaps = true;
            // fallthrough
        case TTM_VectorQuantizedCustomCodeBook:
            isVQCompressed = true;
            codeBookSize = pvrtHeader->width;
            if (codeBookSize < 16)
                codeBookSize = 16;
            else if (codeBookSize == 64)
                codeBookSize = 128;
            else
                codeBookSize = 256;
            break;

        case TTM_Raw:
        case TTM_RawNonSquare:
            break;

        default:
            return 0;
    }

    unsigned char* srcVQ = NULL;
    if (isVQCompressed)
    {
        srcVQ = srcData;
        srcData += 4 * kSrcStride * codeBookSize; // 4 components * codebook size (each 16-bit)
    }

    unsigned int mipWidth = 0;
    unsigned int mipHeight = 0;
    unsigned int mipSize = 0;

    int mipMapCount = (isMipMaps) ? MipMapsCountFromWidth(pvrtHeader->width) : 1;
    while (mipMapCount)
    {
        mipWidth = (pvrtHeader->width >> (mipMapCount - 1));
        mipHeight = (pvrtHeader->height >> (mipMapCount - 1));
        mipSize = mipWidth * mipHeight;

        if (--mipMapCount > 0)
        {
            if (isVQCompressed)
                srcData += mipSize / 4;
            else
                srcData += kSrcStride * mipSize;
        }
        else if (isMipMaps)
        {
            srcData += (isVQCompressed) ? 1 : kSrcStride; // skip 1x1 mip
        }
    }

    if (isVQCompressed)
    {
        mipWidth /= 2;
        mipHeight /= 2;
        mipSize = mipWidth * mipHeight;
    }

    unsigned long x = 0, y = 0;
    unsigned int processed = 0;

    while (processed < mipSize)
    {
        unsigned long srcPos = 0;
        unsigned long dstPos = 0;
        unsigned short srcTexel = 0;

        if (isVQCompressed)
        {
            unsigned long vqIndex = srcData[GetUntwiddledTexelPosition(x, y)] * 4; // codebook index * 4 components (2x2 block)

            for (unsigned int yOffset = 0; yOffset < 2; ++yOffset)
            {
                for (unsigned int xOffset = 0; xOffset < 2; ++xOffset)
                {
                    srcPos = (vqIndex + (xOffset * 2 + yOffset)) * kSrcStride;
                    srcTexel = (srcVQ[srcPos] | (srcVQ[srcPos + 1] << 8));

                    dstPos = ((y * 2 + yOffset) * 2 * mipWidth + (x * 2 + xOffset)) * kDstStride;

                    // Convert srcTexel to 16-bit pixel in destination format:
                    uint16_t pixel16;
                    unsigned char r, g, b, a;
                    TexelToRGBA(srcTexel, srcFormat, &r, &g, &b, &a);

                    switch (srcFormat)
                    {
                        case TFM_RGB565:
                            pixel16 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                            break;
                        case TFM_ARGB1555:
                            pixel16 = ((a > 127) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
                            break;
                        case TFM_ARGB4444:
                        default:
                            pixel16 = ((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
                            break;
                    }

                    ((uint16_t*)dstData)[dstPos / 2] = pixel16;
                }
            }

            if (++x >= mipWidth)
            {
                x = 0;
                ++y;
            }
        }
        else
        {
            x = processed % mipWidth;
            y = processed / mipWidth;

            srcPos = (isTwiddled) ? GetUntwiddledTexelPosition(x, y) : processed;
            srcPos *= kSrcStride;

            srcTexel = (srcData[srcPos] | (srcData[srcPos + 1] << 8));

            dstPos = processed * kDstStride;

            unsigned char r, g, b, a;
            TexelToRGBA(srcTexel, srcFormat, &r, &g, &b, &a);

            uint16_t pixel16;
            switch (srcFormat)
            {
                case TFM_RGB565:
                    pixel16 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                    break;
                case TFM_ARGB1555:
                    pixel16 = ((a > 127) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
                    break;
                case TFM_ARGB4444:
                default:
                    pixel16 = ((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
                    break;
            }

            ((uint16_t*)dstData)[dstPos / 2] = pixel16;
        }
        ++processed;
    }

    return 1;
}

int DecodePVR(unsigned char* srcData, const struct PVRTHeader* pvrtHeader, unsigned char* dstData)
{
    const unsigned int kSrcStride = sizeof(unsigned short int);
    const unsigned int kDstStride = sizeof(unsigned int); // RGBA8888
    
    enum TextureFormatMasks srcFormat = (enum TextureFormatMasks)(pvrtHeader->textureAttributes & 0xFF);
    
    // Unpack data
    bool isTwiddled = false;
    bool isMipMaps = false;
    bool isVQCompressed = false;
    unsigned int codeBookSize = 0;
    
    switch((pvrtHeader->textureAttributes >> 8) & 0xFF)
    {
        case TTM_TwiddledMipMaps:
            isMipMaps = true;
        case TTM_Twiddled:
        case TTM_TwiddledNonSquare:
            isTwiddled = true;
            break;
            
        case TTM_VectorQuantizedMipMaps:
            isMipMaps = true;
        case TTM_VectorQuantized:
            isVQCompressed = true;
            codeBookSize = 256;
            break;
            
        case TTM_VectorQuantizedCustomCodeBookMipMaps:
            isMipMaps = true;
        case TTM_VectorQuantizedCustomCodeBook:
            isVQCompressed = true;
            codeBookSize = pvrtHeader->width;
            if(codeBookSize < 16)
            {
                codeBookSize = 16;
            }
            else if(codeBookSize == 64)
            {
                codeBookSize = 128;
            }
            else
            {
                codeBookSize = 256;
            }
            break;
            
        case TTM_Raw:
        case TTM_RawNonSquare:
            break;
            
        default:
            return 0;
            break;
    }
    
    const unsigned int numCodedComponents = 4;
    unsigned char* srcVQ = 0;
    if (isVQCompressed)
    {
        srcVQ = srcData;
        srcData += numCodedComponents * kSrcStride * codeBookSize;
    }
    
    unsigned int mipWidth = 0;
    unsigned int mipHeight = 0;
    unsigned int mipSize = 0;
    // skip mipmaps
    int mipMapCount = (isMipMaps) ? MipMapsCountFromWidth(pvrtHeader->width) : 1;
    while (mipMapCount)
    {
        mipWidth = (pvrtHeader->width >> (mipMapCount - 1));
        mipHeight = (pvrtHeader->height >> (mipMapCount - 1));
        mipSize = mipWidth * mipHeight;
        
        if (--mipMapCount > 0)
        {
            if (isVQCompressed)
            {
                srcData += mipSize / 4;
            }
            else
            {
                srcData += kSrcStride * mipSize;
            }
        }
        else if (isMipMaps)
        {
            srcData += (isVQCompressed) ? 1 : kSrcStride;  // skip 1x1 mip
        }
    }
    
    // Compressed textures processes only half-size
    if (isVQCompressed)
    {
        mipWidth /= 2;
        mipHeight /= 2;
        mipSize = mipWidth * mipHeight;
    }
    
    //extract image data
    unsigned long int x = 0;
    unsigned long int y = 0;
    
    int proccessed = 0;
    while(proccessed < mipSize)
    {
        unsigned long int srcPos = 0;
        unsigned long int dstPos = 0;
        unsigned short int srcTexel = 0;
        
        if (isVQCompressed)
        {
            unsigned long int vqIndex = srcData[GetUntwiddledTexelPosition(x, y)] * numCodedComponents; // Index of codebook * numbers of 2x2 block components
            // Bypass elements in 2x2 block
            for (unsigned int yoffset = 0; yoffset < 2; ++yoffset)
            {
                for (unsigned int xoffset = 0; xoffset < 2; ++xoffset)
                {   
                    srcPos = (vqIndex + (xoffset * 2 + yoffset)) * kSrcStride;
                    srcTexel = ToUint16(&srcVQ[srcPos]);
                    
                    dstPos = ((y * 2 + yoffset) * 2 * mipWidth + (x * 2 + xoffset)) * kDstStride;
                    TexelToRGBA(srcTexel, srcFormat, &dstData[dstPos], &dstData[dstPos + 1], &dstData[dstPos + 2], &dstData[dstPos + 3]);
                }
            }
            
            if (++x >= mipWidth)
            {
                x = 0;
                ++y;
            }
        }
        else
        {
            x = proccessed % mipWidth;
            y = proccessed / mipWidth;
            
            srcPos = ((isTwiddled) ? GetUntwiddledTexelPosition(x, y) : proccessed) * kSrcStride;
            srcTexel = ToUint16(&srcData[srcPos]);
            
            dstPos = proccessed * kDstStride;
            TexelToRGBA(srcTexel, srcFormat, &dstData[dstPos], &dstData[dstPos + 1], &dstData[dstPos + 2], &dstData[dstPos + 3]);
        }
        
        ++proccessed;
    }
    
    return 1;
}

int MipMapsCountFromWidth(unsigned long int width)
{
    unsigned int mipMapsCount = 0;
    while( width )
    {
        ++mipMapsCount;
        width /= 2;
    }
    
    return mipMapsCount;
}

// Twiddle
unsigned long int UntwiddleValue(unsigned long int value)
{
    unsigned long int untwiddled = 0;
    
    for (size_t i = 0; i < 10; i++)
    {
        unsigned long int shift = pow(2, i);
        if (value & shift) untwiddled |= (shift << i);
    }
    
    return untwiddled;
}

void BuildTwiddleTable()
{
    for( unsigned long int i = 0; i < kTwiddleTableSize; i++ )
    {
        gTwiddledTable[i] = UntwiddleValue( i );
    }
}

unsigned long int GetUntwiddledTexelPosition(unsigned long int x, unsigned long int y)
{
    unsigned long int pos = 0;
    
    if(x >= kTwiddleTableSize || y >= kTwiddleTableSize)
    {
        pos = UntwiddleValue(y)  |  UntwiddleValue(x) << 1;
    }
    else
    {
        pos = gTwiddledTable[y]  |  gTwiddledTable[x] << 1;
    }
    
    return pos;
}

void TexelToRGBA(unsigned short int srcTexel, enum TextureFormatMasks srcFormat, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
    switch( srcFormat )
    {
        case TFM_RGB565:
            *a = 0xFF;
            *r = (srcTexel & 0xF800) >> 8;
            *g = (srcTexel & 0x07E0) >> 3;
            *b = (srcTexel & 0x001F) << 3;
            break;
            
        case TFM_ARGB1555:
            *a = (srcTexel & 0x8000) ? 0xFF : 0x00;
            *r = (srcTexel & 0x7C00) >> 7;
            *g = (srcTexel & 0x03E0) >> 2;
            *b = (srcTexel & 0x001F) << 3;
            break;
            
        case TFM_ARGB4444:
            *a = (srcTexel & 0xF000) >> 8;
            *r = (srcTexel & 0x0F00) >> 4;
            *g = (srcTexel & 0x00F0);
            *b = (srcTexel & 0x000F) << 4;
            break;
            
        case TFM_YUV422: // wip
            *a = 0xFF;
            *r = 0xFF;
            *g = 0xFF;
            *b = 0xFF;
            break;
    }
}
