/*
   Tsunami for KallistiOS ##version##

   menudefinition.h

   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUDEFINITION_H
#define __TSUDEFINITION_H

#define IMAGE_TYPE_MASK 0XFFFF

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
	ML_CURSOR = ML_ITEM + ML_SELECTED + 20,
	ML_POPUP = 160
};

enum ImageTypeEnum
{
	IT_PNG = (1 << 0),
	IT_JPG = (1 << 1),	
	IT_PVR = (1 << 2),
	IT_KMG = (1 << 3),
	IT_BPM = (1 << 4),
	IT_ALL = (IT_PNG | IT_JPG | IT_BPM | IT_PVR | IT_KMG)
};

#endif //__TSUDEFINITION_H
