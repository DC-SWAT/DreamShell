/*
   Tsunami for KallistiOS ##version##

   tsudefinition.h

   Copyright (C) 2024-2025 Maniac Vera

*/

#ifndef __TSUDEFINITION_H
#define __TSUDEFINITION_H

#define IMAGE_TYPE_MASK 0XFFFF
#define DEFAULT_SHADOW_COLOR { 0.7, 0.0f, 0.0f, 0.0f }
#define DEFAULT_TOP_COLOR { 1, 0.22f, 0.06f, 0.25f }
#define DEFAULT_BOTTOM_COLOR { 1, 0.22f, 0.06f, 0.25f }
#define DEFAULT_BORDER_COLOR { 1, 1.0f, 1.0f, 1.0f }
#define DEFAULT_BODY_COLOR { 1, 0.22f, 0.06f, 0.25f }

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
