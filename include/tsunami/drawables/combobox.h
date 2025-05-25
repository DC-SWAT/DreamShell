/*
   Tsunami for KallistiOS ##version##

   combobox.h

   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_DRW_COMBOBOX_H
#define __TSUNAMI_DRW_COMBOBOX_H

#include "../drawable.h"

#ifdef __cplusplus

class ComboBox : public Drawable {
public:
	ComboBox();
	virtual ~ComboBox();
};

#else

typedef struct scene ComboBox;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

ComboBox* TSU_ComboBoxCreate();
void TSU_ComboBoxDestroy(ComboBox **combobox_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRW_COMBOBOX_H */
