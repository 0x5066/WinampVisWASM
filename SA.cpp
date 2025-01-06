/*
Spectrum Analyzer - Linux Port
*/

#include "main.h"
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static int last_pos;
typedef struct sa_l {
    int timestamp;
    unsigned char data[2 * 75];
    char which;
} sa_l;

int paused = 0;
int playing = 1;
int config_saref = 2;
int config_minimized = 0;
int config_pe_open = 0;
int config_pe_width = 0;
int config_pe_height = 0;
int config_disvis = 0;

static int sa_fps = 76;
static sa_l *sa_bufs;
static int sa_position;
static int sa_length, sa_size;
static pthread_mutex_t cs = PTHREAD_MUTEX_INITIALIZER;

extern "C" {
EMSCRIPTEN_KEEPALIVE
void sa_init(int numframes){

    pthread_mutex_lock(&cs);
    sa_length = 0;
    if (numframes < 1) numframes = 1;

    if (numframes > sa_size)
    {
        free(sa_bufs);
        sa_bufs = (sa_l *)calloc(numframes, sizeof(sa_l));
        sa_size = numframes;
    }
    sa_position = 0;
    sa_length = numframes;
    last_pos = 0;
    pthread_mutex_unlock(&cs);
}
}

void sa_deinit(void)
{
    pthread_mutex_lock(&cs);
    sa_length = 0;
    pthread_mutex_unlock(&cs);
}

int sa_add(char *values, int timestamp, int csa)
{
    pthread_mutex_lock(&cs);
    if (!sa_bufs || sa_length == 0) 
    {
        pthread_mutex_unlock(&cs);
        return 1;
    }

    if (sa_length == 1)
    {
        sa_position = 0;
    }
    if (csa == 3) csa = 1; 
    csa &= 0x7fffffff;

    sa_bufs[sa_position].timestamp = timestamp;
    sa_bufs[sa_position].which = (char)csa;

    if (csa & 1)
    {
        memcpy(sa_bufs[sa_position].data, values, 75);
        values += 75;
    }
    else
        memset(sa_bufs[sa_position].data, 0, 75);

    if (csa & 2)
        memcpy(sa_bufs[sa_position].data + 75, values, 75);
    else
        memset(sa_bufs[sa_position].data + 75, 0, 75);

    sa_position++;
    if (sa_position >= sa_length) sa_position -= sa_length;
    pthread_mutex_unlock(&cs);
    return 0;
}

char *sa_get(int timestamp, int csa, char data[75*2+8])
{
    static int sa_pos;
    int closest = 1000000, closest_v = -1;
    pthread_mutex_lock(&cs);

    if (!sa_bufs || sa_length == 0)
    {
        pthread_mutex_unlock(&cs);
        return 0;
    }

    if (sa_length == 1)
    {
        memcpy(data, sa_bufs[0].data, 75*2);
        pthread_mutex_unlock(&cs);
        return data;
    }

    int i = last_pos;
    for (int x = 0; x < sa_length; x++)
    {
        if (i >= sa_length) i = 0;
        int d = timestamp - sa_bufs[i].timestamp;
        if (d < 0) d = -d;
        if (d < closest)
        {
            closest = d;
            closest_v = i;
        }
        else if (closest <= 6) break;
        i++;
    }

    if (closest < 400 && closest_v >= 0 && sa_bufs[closest_v].which & csa)
    {
        sa_pos = 0;
        last_pos = closest_v;
        memcpy(data, sa_bufs[closest_v].data, 75*2);
        pthread_mutex_unlock(&cs);
        return data;
    }

    if (closest_v < 0 || !(sa_bufs[closest_v].which & csa) || closest > 400)
    {
        memset(data, 0, 75);
        data[(sa_pos % 150) >= 75 ? 149 - (sa_pos % 150) : (sa_pos % 150)] = 15;
        for (int x = 0; x < 75; x++)
            data[x + 75] = (char)(int)(7.0 * sin((sa_pos + x) * 0.1));
        sa_pos++;
        pthread_mutex_unlock(&cs);
        return data;
    }
    pthread_mutex_unlock(&cs);
    return 0;
}

volatile int sa_override;
void export_sa_setreq(int want)
{
    pthread_mutex_lock(&cs);
    sa_override = want;
    pthread_mutex_unlock(&cs);
}

char *export_sa_get_deprecated()
{
    static char data[75*2 + 8];
    int now = in_getouttime();
    char *p = sa_get(now, 3, data);
    if (!p) memset(data, 0, 75*2);

    return data;
}

char *export_sa_get(char data[75*2 + 8])
{
    try
    {
        int now = in_getouttime();
        char *p = sa_get(now, 3, data);
        if (!p) memset(data, 0, 75*2);
    }
    catch(...) {}
    return data;
}

#define KILL_EVENT 0
#define BLANK_EVENT 1
#define ON_EVENT 2
#define NUM_EVENTS 3

static pthread_cond_t sa_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t sa_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int sa_event = BLANK_EVENT;

static int SA_Wait()
{
    pthread_mutex_lock(&sa_mutex);
    while (sa_event == BLANK_EVENT)
        pthread_cond_wait(&sa_cond, &sa_mutex);

    int event = sa_event;
    if (sa_event == KILL_EVENT)
        sa_event = BLANK_EVENT;
    pthread_mutex_unlock(&sa_mutex);
    return event;
}

static void *bivis_thread(void *none)
{
    int cycleCount = 0;
    char data[75*2 + 8] = {0};
    unsigned char vdata[2] = {0};

    struct timespec deadline;

    clock_gettime(CLOCK_MONOTONIC, &deadline);

    while (1)
    {
    deadline.tv_nsec += 13.157894736842104f * 1000000;
    if (deadline.tv_nsec >= 1000000000) {
        deadline.tv_sec += deadline.tv_nsec / 1000000000;
        deadline.tv_nsec %= 1000000000;
    }
        /* Sleep until new deadline */
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL) != 0)
            if (errno != EINTR) continue;
        switch (SA_Wait())
        {
        case KILL_EVENT:
            specData[76 * 16 * 4] = {0};
            return NULL;

        case BLANK_EVENT:
            draw_sa(NULL, 1, NULL);
            specData[76 * 16 * 4] = {0};
            break;

        case ON_EVENT:
        {
            int draw = 1;

            if (++cycleCount < config_saref)
                draw = 0;
            else
            {
                cycleCount = 0;
                draw = 1;
            }

            if (config_sa && !paused && playing && !config_minimized &&
                (config_mw_open || (config_pe_open && config_pe_width >= 350 && config_pe_height != 14)) &&
                (!config_disvis || !vis_running()))
            {
                int a = in_getouttime();
                int t = config_sa;
                char *c = sa_get(a, t, data);
                unsigned char *d;
                if (vu_get(a, vdata)) d = vdata;
                
                //std::cout << export_vu_get(0) << std::endl;;

                if (c)
                {
                    if (t == 2) c += 75;
                    else memset(c + 75, 0, 4);
                    draw_sa(c, 1, d);
                }
            }
        }
        break;
        }
    }
    return NULL;
}

pthread_t saThread;

// Define the FFT instance and buffer sizes
float out_spectraldata[BUFFER_SIZE]; // Adjust size as per the output requirement
extern "C" {
EMSCRIPTEN_KEEPALIVE
void SpectralAnalyzer_Create()
{
    pthread_mutex_init(&cs, NULL);
    pthread_create(&saThread, NULL, bivis_thread, NULL);

    sa_length = sa_size = 0;

    VU_Create();
}
}

void SpectralAnalyzer_Destroy()
{
    VU_Destroy();

    pthread_mutex_lock(&sa_mutex);
    sa_event = KILL_EVENT;
    pthread_cond_signal(&sa_cond);
    pthread_mutex_unlock(&sa_mutex);

    pthread_join(saThread, NULL);

    pthread_mutex_destroy(&cs);
    pthread_mutex_destroy(&sa_mutex);
    pthread_cond_destroy(&sa_cond);

    free(sa_bufs);
    sa_bufs = 0;
    sa_size = 0;
}

extern "C" {
EMSCRIPTEN_KEEPALIVE
void sa_setthread(int mode)
{
    if (mode == -1)
        mode = 0;

    sa_curmode = mode;

    pthread_mutex_lock(&sa_mutex);
    sa_event = mode ? ON_EVENT : BLANK_EVENT;
    pthread_cond_signal(&sa_cond);
    pthread_mutex_unlock(&sa_mutex);
}
}