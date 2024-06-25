/*
   Tsunami for KallistiOS ##version##

   itemmenu.cpp

   Copyright (C) 2024 Maniac Vera
   
*/

#include "tsunamiutils.h"

SendMessageCallback *send_message_callback = nullptr;

extern "C"
{
    void TSU_SendMessageCallback(SendMessageCallback *callback)
    {
        if (callback != nullptr) {
            send_message_callback = callback;
        }
    }

    void TSU_SendMessage(const char *message)
    {
        if (send_message_callback != nullptr) {
            send_message_callback(message);
        }
    }
}