#pragma once

/*
 * The Dreamcast doesn't have any kind of profiling support from GCC
 * so this is a cumbersome sampling profiler that runs in a background thread
 */
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
