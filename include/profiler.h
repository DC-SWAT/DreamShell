
/* DreamShell ##version##

   profiler.h
   Copyright (C) 2020 Luke Benstead
   Copyright (C) 2023 Ruslan Rostovtsev
*/

/** \file   src/profiler.h
    \brief  gprof compatible sampling profiler.

    The Dreamcast doesn't have any kind of profiling support from GCC
    so this is a cumbersome sampling profiler that runs in a background thread.
    Once profiling has begun, the background thread will regularly gather the PC and PR registers stored by
    the other threads.
    The way thread scheduling works is that when other threads are blocked their current program counter is stored
    in a context. If the profiler thread is doing work then all the other threads aren't and so the stored program
    counters will be up-to-date.
    The profiling thread gathers PC/PR pairs and how often that pairing appears.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void profiler_init(const char* output);
void profiler_start();
void profiler_stop();
void profiler_clean_up();

#ifdef __cplusplus
}
#endif
