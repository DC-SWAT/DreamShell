/*
   Tsunami for KallistiOS ##version##

   menudefinition.h

   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUDEFINITION_H
#define __TSUDEFINITION_H

enum MenuTypeEnum
{
    MT_PLANE_TEXT = 1,
    MT_IMAGE_TEXT_64_5X2 = 2,
    MT_IMAGE_128_4X3 = 3,
    MT_IMAGE_256_3X2 = 4    
};

enum MenuLayerEnum
{
    ML_BACKGROUND = 1,    
    ML_ITEM = 10,
    ML_SELECTED = 20,
    ML_CURSOR = 30    
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
