// compile with emcc -o draw_sa.js draw_sa.cpp SA.cpp classic_vis.cpp SABuffer.cpp vu.cpp FFTNullsoft.cpp wa_stubs.cpp -s NO_EXIT_RUNTIME=1 -s "EXPORTED_FUNCTIONS=['_get_specData', '_free_specData', '_malloc', '_free', '_get_config_sa', '_set_config_sa', '_SpectralAnalyzer_Create', '_sa_setthread', '_sa_init', '_sa_addpcmdata', '_get_config_sa', '_set_config_sa', '_in_getouttime', '_set_playtime']" -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap']" --std=c++23 -pthread -g3

#include <pthread.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cstdint>
#include <thread>
#include <iostream>
#include <semaphore.h>
//#include <SDL2/SDL.h>
#include "FFTNullsoft.h"
#include <emscripten/emscripten.h>

// only 512 works here, why?
#define BUFFER_SIZE 512

extern volatile int sa_override;
extern int sa_curmode;
extern int _srate;

extern unsigned char config_dsize;
extern unsigned char config_windowshade;
extern unsigned char config_sa, config_safire, config_safalloff, config_sa_peaks, config_sa_peak_falloff;
extern int config_mw_open;
extern int draw_initted;
extern int s, ws, playtime;

extern unsigned char* specData;
inline long MulDiv (long a, long b, long c);

extern "C" void SpectralAnalyzer_Create();
void SpectralAnalyzer_Destroy();
extern "C" void sa_setthread(int enabled);
void sa_deinit(void);
extern "C" void sa_init(int numframes);
int sa_add(char *values, int timestamp, int csa);
char *sa_get(int timestamp, int csa, char data[75*2 + 8]);
void sa_addpcmdata(void *PCMData, int nch, int bps, int timestamp);

extern "C" void vu_init(int numframes, int srate);
void vu_deinit();
void VU_Create();
void VU_Destroy();
int vu_add(char *values, int timestamp);
bool vu_get(int timestamp, unsigned char data[2]);
int export_vu_get(int channel);
void calcVuData(unsigned char *out, char *data, const int channels, const int bits);

void draw_sa(char *values, int draw, unsigned char* vuData);

extern "C" int in_getouttime(void);
int vis_running();