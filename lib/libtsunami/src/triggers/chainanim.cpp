/*
   Tsunami for KallistiOS ##version##

   logxymover.cpp

   Copyright (C) 2024 Maniac Vera
   
*/
#include "triggers/chainanim.h"

extern "C"
{
    ChainAnimation* TSU_ChainAnimationCreate(Animation *animation_ptr, Drawable *target_ptr)
    {
        return new ChainAnimation(animation_ptr, target_ptr);
    }

    void TSU_ChainAnimationDestroy(ChainAnimation **chainanimation_ptr)
	{
		if (*chainanimation_ptr != NULL) {
			delete *chainanimation_ptr;
			*chainanimation_ptr = NULL;
		}
	}
}