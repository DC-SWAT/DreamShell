/*
   Tsunami for KallistiOS ##version##

   menudefinition.h

   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUDEFINITION_H
#define __TSUDEFINITION_H

enum MenuTypeEnum
{
    MT_IMAGE_TEXT_64_5X2 = 1,
    MT_IMAGE_128_4X3 = 2,
    MT_IMAGE_256_3X2 = 3,
    MT_PLANE_TEXT = 4
};

enum MenuLayerEnum
{
    ML_BACKGROUND = 1,
    ML_CURSOR,
    ML_ITEM,
    ML_SELECTED
};

enum ImageTypeEnum
{
    IT_PNG = 1,
    IT_JPG = 2,
    IT_BPM = 3,
    IT_PVR = 4,
    IT_KMG = 5
};

#endif //__TSUDEFINITION_H
