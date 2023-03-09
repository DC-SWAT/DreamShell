/* DreamShell ##version##

   profiler.c
   Copyright (C) 2020 Luke Benstead
   Copyright (C) 2023 Ruslan Rostovtsev
*/

#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <kos/thread.h>
#include <dc/fs_dcload.h>

static char KERNEL_OUTPUT_FILENAME[128];
static char VIDEO_OUTPUT_FILENAME[128];
static kthread_t* THREAD;
static volatile bool PROFILER_RUNNING = false;
static volatile bool PROFILER_RECORDING = false;
static tid_t KERNEL_TID = 0;
static tid_t VIDEO_TID = 0;

#define BASE_ADDRESS 0x8c010000
#define BUCKET_SIZE 10000

#define INTERVAL_IN_MS 10

/* Simple hash table of samples. An array of Samples
 * but, each sample in that array can be the head of
 * a linked list of other samples */
typedef struct Arc {
    uint32_t pc;
    uint32_t pr; // Caller return address
    uint32_t count;
    tid_t tid;
    struct Arc* next;
} Arc;

static Arc ARCS[BUCKET_SIZE];

/* Hashing function for two uint32_ts */
#define HASH_PAIR(x, y) ((x * 0x1f1f1f1f) ^ y)

#define BUFFER_SIZE (1024 * 8)  // 8K buffer

const static size_t MAX_ARC_COUNT = BUFFER_SIZE / sizeof(Arc);
static size_t ARC_COUNT = 0;

static bool write_samples(tid_t tid, const char* path);
static void clear_samples();

static Arc* new_arc(tid_t tid, uint32_t PC, uint32_t PR) {
    Arc* s = (Arc*) malloc(sizeof(Arc));
    s->count = 1;
    s->pc = PC;
    s->pr = PR;
    s->tid = tid;
    s->next = NULL;

    ++ARC_COUNT;

    return s;
}

static void record_thread(tid_t tid, uint32_t PC, uint32_t PR) {
    uint32_t bucket = HASH_PAIR(PC, PR) % BUCKET_SIZE;

    Arc* s = &ARCS[bucket];

    if(s->pc) {
        /* Initialized sample in this bucket,
         * does it match though? */
        while(s->pc != PC || s->pr != PR) {
            if(s->next) {
                s = s->next;
            } else {
                s->next = new_arc(tid, PC, PR);
                return; // We're done
            }
        }

        s->count++;
    } else {
        /* Initialize this sample */
        s->count = 1;
        s->pc = PC;
        s->pr = PR;
        s->tid = tid;
        s->next = NULL;
        ++ARC_COUNT;
    }
}

static int thd_each_cb(kthread_t* thd, void* data) {
    (void) data;

    // FIXME: Process all threads without TID's.
    if(VIDEO_TID == 0 || KERNEL_TID == 0) {
        if(strcmp(thd->label, "[video]") == 0) {
            VIDEO_TID = thd->tid;
        } else if(strcmp(thd->label, "[kernel]") == 0) {
            KERNEL_TID = thd->tid;
        } else {
            return 0;
        }
    } else if(thd->tid != KERNEL_TID && thd->tid != VIDEO_TID) {
        return 0;
    }

    /* The idea is that if this code right here is running in the profiling
     * thread, then all the PCs from the other threads are
     * current. Obviouly thought between iterations the
     * PC will change so it's not like this is a true snapshot
     * in time across threads */
    uint32_t PC = thd->context.pc;
    uint32_t PR = thd->context.pr;
    record_thread(thd->tid, PC, PR);
    return 0;
}

static void record_samples() {
    /* Go through all the active threads and increase
     * the sample count for the PC for each of them */

    size_t initial = ARC_COUNT;

    thd_each(&thd_each_cb, NULL);

    if(ARC_COUNT >= MAX_ARC_COUNT) {
        /* TIME TO FLUSH! */
        if(!write_samples(KERNEL_TID, KERNEL_OUTPUT_FILENAME)) {
            dbglog(DBG_ERROR, "Error writing samples\n");
        }
        if(!write_samples(VIDEO_TID, VIDEO_OUTPUT_FILENAME)) {
            dbglog(DBG_ERROR, "Error writing samples\n");
        }
        clear_samples();
    }

    /* We log when the number of PCs recorded hits a certain increment */
    if((initial != ARC_COUNT) && ((ARC_COUNT % 1000) == 0)) {
        dbglog(DBG_INFO, "-- %d arcs recorded...\n", ARC_COUNT);
    }
}


#define GMON_COOKIE "gmon"
#define GMON_VERSION 1

typedef struct {
    char cookie[4];  // 'g','m','o','n'
    int32_t version; // 1
    char spare[3 * 4]; // Padding
} GmonHeader;

typedef struct {
    uint32_t low_pc;
    uint32_t high_pc;
    uint32_t hist_size;
    uint32_t prof_rate;
    char dimen[15];			/* phys. dim., usually "seconds" */
    char dimen_abbrev;			/* usually 's' for "seconds" */
} GmonHistHeader;

typedef struct {
    unsigned char tag; // GMON_TAG_TIME_HIST = 0, GMON_TAG_CG_ARC = 1, GMON_TAG_BB_COUNT = 2
    size_t ncounts; // Number of address/count pairs in this sequence
} GmonBBHeader;

typedef struct {
    uint32_t from_pc;	/* address within caller's body */
    uint32_t self_pc;	/* address within callee's body */
    uint32_t count;			/* number of arc traversals */
} GmonArc;

static bool init_sample_file(const char* path) {

    FILE* out = fopen(path, "w");
    if(!out) {
        return false;
    }

    /* Write the GMON header */

    GmonHeader header;
    memcpy(&header.cookie[0], GMON_COOKIE, sizeof(header.cookie));
    header.version = 1;
    memset(header.spare, '\0', sizeof(header.spare));

    fwrite(&header, sizeof(header), 1, out);

    fclose(out);
    return true;
}

#define ROUNDDOWN(x,y) (((x)/(y))*(y))
#define ROUNDUP(x,y) ((((x)+(y)-1)/(y))*(y))

static bool write_samples(tid_t tid, const char* path) {
    /* Appends the samples to the output file in gmon format
     *
     * We iterate the data twice, first generating arcs, then generating
     * basic block counts. While we do that though we calculate the data
     * for the histogram so we don't need a third iteration */

    extern char _etext;

    const uint32_t HISTFRACTION = 8;

    /* We know the lowest address, it's the same for all DC games */
    uint32_t lowest_address = ROUNDDOWN(BASE_ADDRESS, HISTFRACTION);

    /* We need to calculate the highest address though */
    uint32_t highest_address = ROUNDUP((uint32_t) &_etext, HISTFRACTION);

    /* Histogram data */
    const int BIN_COUNT = ((highest_address - lowest_address) / HISTFRACTION);
    uint16_t* bins = (uint16_t*) malloc(BIN_COUNT * sizeof(uint16_t));
    memset(bins, 0, sizeof(uint16_t) * BIN_COUNT);

    FILE* out = fopen(path, "r+");  /* Append, as init_sample_file would have created the file */
    if(!out) {
        dbglog(DBG_ERROR, "-- Error writing samples to output file\n");
        return false;
    }

    // Seek to the end of the file
    fseek(out, 0, SEEK_END);

    uint8_t tag = 1;
    size_t written = 0;

    /* Write arcs */
    Arc* root = ARCS;
    for(int i = 0; i < BUCKET_SIZE; ++i) {
        if(root->pc) {
            if(root->tid != tid) {
                root++;
                continue;
            }
            GmonArc arc;
            arc.from_pc = root->pr;
            arc.self_pc = root->pc;
            arc.count = root->count;

            /* Write the root sample if it has a program counter */
            fwrite(&tag, sizeof(tag), 1, out);
            fwrite(&arc, sizeof(GmonArc), 1, out);

            ++written;

            /* If there's a next pointer, traverse the list */
            Arc* s = root->next;
            while(s) {
                if(s->tid != tid) {
                    s = s->next;
                    continue;
                }
                arc.from_pc = s->pr;
                arc.self_pc = s->pc;
                arc.count = s->count;

                /* Write the root sample if it has a program counter */
                fwrite(&tag, sizeof(tag), 1, out);
                fwrite(&arc, sizeof(GmonArc), 1, out);

                ++written;
                s = s->next;
            }
        }

        root++;
    }

    uint32_t histogram_range = highest_address - lowest_address;
    uint32_t bin_size = histogram_range / BIN_COUNT;

    root = ARCS;
    for(int i = 0; i < BUCKET_SIZE; ++i) {
        if(root->pc && root->tid == tid) {
            // dbglog(DBG_INFO, "Incrementing %ld for %x. ", (root->pc - lowest_address) / bin_size, (unsigned int) root->pc);
            bins[(root->pc - lowest_address) / bin_size]++;
            // dbglog(DBG_INFO, "Now: %d\n", (int) bins[(root->pc - lowest_address) / bin_size]);

            /* If there's a next pointer, traverse the list */
            Arc* s = root->next;
            while(s) {
                assert(s->pc);
                if(s->tid == tid) {
                    bins[(s->pc - lowest_address) / bin_size]++;
                }
                s = s->next;
            }
        }

        root++;
    }

    /* Write histogram now that we have all the information we need */
    GmonHistHeader hist_header;
    hist_header.low_pc = lowest_address;
    hist_header.high_pc = highest_address;
    hist_header.hist_size = BIN_COUNT;
    hist_header.prof_rate = INTERVAL_IN_MS;
    strcpy(hist_header.dimen, "seconds");
    hist_header.dimen_abbrev = 's';

    unsigned char hist_tag = 0;
    fwrite(&hist_tag, sizeof(hist_tag), 1, out);
    fwrite(&hist_header, sizeof(hist_header), 1, out);
    fwrite(bins, sizeof(uint16_t), BIN_COUNT, out);

    fclose(out);
    free(bins);

    dbglog(DBG_INFO, "-- Written %d arcs to %s\n", written, path);

    return true;
}


static void* run(void* args) {
    dbglog(DBG_INFO, "-- Entered profiler thread!\n");

    while(PROFILER_RUNNING){
        if(PROFILER_RECORDING) {
            record_samples();
            usleep(INTERVAL_IN_MS * 1000); //usleep takes milliseconds
        }
    }

    dbglog(DBG_INFO, "-- Profiler thread finished!\n");

    return NULL;
}

void profiler_init(const char* output) {
    /* Store the filenames */
    sprintf(KERNEL_OUTPUT_FILENAME, "%s/kernel_gmon.out", output);
    sprintf(VIDEO_OUTPUT_FILENAME, "%s/video_gmon.out", output);

    /* Initialize the file */
    dbglog(DBG_INFO, "Creating profiler samples file for kernel thread...\n");
    if(!init_sample_file(KERNEL_OUTPUT_FILENAME)) {
        dbglog(DBG_ERROR, "Can't create %s\n", KERNEL_OUTPUT_FILENAME);
        return;
    }

    dbglog(DBG_INFO, "Creating profiler samples file for video thread...\n");
    if(!init_sample_file(VIDEO_OUTPUT_FILENAME)) {
        dbglog(DBG_ERROR, "Can't create %s\n", VIDEO_OUTPUT_FILENAME);
        return;
    }

    dbglog(DBG_INFO, "Creating profiler thread...\n");
    // Initialize the samples to zero
    memset(ARCS, 0, sizeof(ARCS));

    PROFILER_RUNNING = true;
    THREAD = thd_create(0, run, NULL);

    /* Lower priority is... er, higher */
    thd_set_prio(THREAD, PRIO_DEFAULT / 2);

    dbglog(DBG_INFO, "Profiler thread started.\n");
}

void profiler_start() {
    assert(PROFILER_RUNNING);

    if(PROFILER_RECORDING) {
        return;
    }

    PROFILER_RECORDING = true;
    dbglog(DBG_INFO, "Starting profiling...\n");
}

static void clear_samples() {
    /* Free the samples we've collected to start again */
    Arc* root = ARCS;
    for(int i = 0; i < BUCKET_SIZE; ++i) {
        Arc* s = root;
        Arc* next = s->next;

        // While we have a next pointer
        while(next) {
            s = next; // Point S at it
            next = s->next; // Store the new next pointer
            free(s); // Free S
        }

        // We've wiped the chain so we can now clear the root
        // which is statically allocated
        root->next = NULL;
        root++;
    }

    // Wipe the lot
    memset(ARCS, 0, sizeof(ARCS));
    ARC_COUNT = 0;
}

bool profiler_stop() {
    if(!PROFILER_RECORDING) {
        return false;
    }

    dbglog(DBG_INFO, "Stopping profiling...\n");

    PROFILER_RECORDING = false;

    if(!write_samples(KERNEL_TID, KERNEL_OUTPUT_FILENAME)) {
        dbglog(DBG_ERROR, "ERROR WRITING SAMPLES (RO filesystem?)!\n");
        return false;
    }

    if(!write_samples(VIDEO_TID, VIDEO_OUTPUT_FILENAME)) {
        dbglog(DBG_ERROR, "ERROR WRITING SAMPLES (RO filesystem?)!\n");
        return false;
    }

    clear_samples();
    return true;
}

void profiler_clean_up() {
    profiler_stop(); // Make sure everything is stopped

    PROFILER_RUNNING = false;
    thd_join(THREAD, NULL);
}
