/*
   Tsunami for KallistiOS ##version##

   tsunamiutils.h

   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAME_UTILS_H
#define __TSUNAME_UTILS_H

typedef void SendMessageCallback(const char *message);
extern SendMessageCallback *send_message_callback;

#ifdef __cplusplus
    
#else
    
#endif 

#ifdef __cplusplus
extern "C"
{
#endif

void TSU_SendMessageCallback(SendMessageCallback *callback);
void TSU_SendMessage(const char *message);

#ifdef __cplusplus
};
#endif

#endif // __TSUNAME_UTILS_H