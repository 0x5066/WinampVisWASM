/** (c) Nullsoft, Inc.         C O N F I D E N T I A L
 ** Filename: 
 ** Project:
 ** Description:
 ** Author:
 ** Created:
 **/
/* 

// emcc -o draw_sa.js draw_sa.cpp SA.cpp classic_vis.cpp SABuffer.cpp vu.cpp FFTNullsoft.cpp wa_stubs.cpp -s NO_EXIT_RUNTIME=1 -s "EXPORTED_FUNCTIONS=['_get_specData', '_free_specData', '_malloc', '_free', '_get_config_sa', '_set_config_sa', '_SpectralAnalyzer_Create', '_sa_setthread', '_sa_init', '_sa_addpcmdata', '_get_config_sa', '_set_config_sa', '_in_getouttime', '_set_playtime']" -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap']" --std=c++23 -pthread -s USE_PTHREADS=1 -sPTHREAD_POOL_SIZE=4 -sALLOW_MEMORY_GROWTH=1

#include "Main.h"
#include "draw.h"
#include "WADrawDC.h" */
#include <algorithm>
#include <cstring>
#include "main.h"

unsigned char config_dsize = 0;
unsigned char config_windowshade = 0;
unsigned char config_sa = 0;
unsigned char config_vufire = 0;
unsigned char config_safire = 4;
unsigned char config_safalloff = 2;
unsigned char config_sa_peaks = 1;
unsigned char config_vu_peaks = 1;
unsigned char config_sa_peak_falloff = 1;
unsigned char config_vu_peak_falloff = 1;
int config_mw_open = 1;
int draw_initted = 1;
unsigned char* specData = new unsigned char[76 * 16 * 4];
int sa_safe=0;
int sa_kill=0;
int s=1;
int ws=0;

unsigned char ppal[] = { 
    0, 0, 0,          // color 0 = black
    24, 24, 41,       // color 1 = grey for dots
    239, 49, 16,      // color 2 = top of spec
    206, 41, 16,      // 3
    214, 90, 0,       // 4
    214, 102, 0,      // 5
    214, 115, 0,      // 6
    198, 123, 8,      // 7
    222, 165, 24,     // 8
    214, 181, 33,     // 9
    189, 222, 41,     // 10
    148, 222, 33,     // 11
    41, 206, 16,      // 12
    50, 190, 16,      // 13
    57, 181, 16,      // 14
    49, 156, 8,       // 15
    41, 148, 0,       // 16
    24, 132, 8,       // 17
    255, 255, 255,    // 18 = osc 1
    214, 214, 222,    // 19 = osc 2
    181, 189, 189,    // 20 = osc 3
    160, 170, 175,    // 21 = osc 4
    148, 156, 165,    // 22 = osc 4
    150, 150, 150     // 23 = analyzer peak
};

int  palette[256];

#define SA_BLEND(c) (palette[c] | 0xFF000000) //(alpha << 24))

extern "C" {

    // Getter for config_sa
    unsigned char get_config_sa() {
        return config_sa;
    }

    // Setter for config_sa
    void set_config_sa(unsigned char value) {
        config_sa = value;
    }

	// Getter for config_safire
    unsigned char get_config_safire() {
        return config_safire;
    }

    // Setter for config_safire
    void set_config_safire(unsigned char value) {
        config_safire = value;
    }

		// Getter for config_vufire
    unsigned char get_config_vufire() {
        return config_vufire;
    }

    // Setter for config_vufire
    void set_config_vufire(unsigned char value) {
        config_vufire = value;
    }

	// Getter for config_safalloff
    unsigned char get_config_safalloff() {
        return config_safalloff;
    }

    // Setter for config_safalloff
    void set_config_safalloff(unsigned char value) {
        config_safalloff = value;
    }

	// Getter for config_sa_peak_falloff
    unsigned char get_config_sa_peak_falloff() {
        return config_sa_peak_falloff;
    }

    // Setter for config_sa_peak_falloff
    void set_config_sa_peak_falloff(unsigned char value) {
        config_sa_peak_falloff = value;
    }

	// Getter for config_sa_peaks
    unsigned char get_config_sa_peaks() {
        return config_sa_peaks;
    }

    // Setter for config_sa_peaks
    void set_config_sa_peaks(unsigned char value) {
        config_sa_peaks = value;
    }

	// Getter for config_vu_peak_falloff
    unsigned char get_config_vu_peak_falloff() {
        return config_vu_peak_falloff;
    }

    // Setter for config_vu_peak_falloff
    void set_config_vu_peak_falloff(unsigned char value) {
        config_vu_peak_falloff = value;
    }

	// Getter for config_vu_peaks
    unsigned char get_config_vu_peaks() {
        return config_vu_peaks;
    }

    // Setter for config_vu_peaks
    void set_config_vu_peaks(unsigned char value) {
        config_vu_peaks = value;
    }

    EMSCRIPTEN_KEEPALIVE
    unsigned char* get_specData() {
        return specData;
    }

    EMSCRIPTEN_KEEPALIVE
    void free_specData() {
        delete[] specData;
        specData = nullptr;
    }
}

	void draw_sa(char *values, int draw, unsigned char* vuData)
	{
		static int bx[75];
		static int t_bx[75];
		static float t_vx[75];
		static int v_bx[2];
		static float v_vx[2];
		int x;
		int fo[5] = {3, 6, 12, 16, 32 };
		float pfo[5]={1.05f,1.1f,1.2f,1.4f,1.6f};
		int dbx;
		float spfo;
		float vpfo;

		//int ws=0;
		//s = 1;
		unsigned char *gmem;

		dbx = fo[std::max(static_cast<int>(std::min(static_cast<int>(config_safalloff), 4)), 0)];
		spfo = pfo[std::max(static_cast<int>(std::min(static_cast<int>(config_sa_peak_falloff), 4)), 0)];
		vpfo = pfo[std::max(static_cast<int>(std::min(static_cast<int>(config_vu_peak_falloff), 4)), 0)];
		sa_safe++;
		if (sa_kill || !draw_initted || !specData)
		{
			sa_safe--;
			return ;
		}

		memset(specData, 0, 76 * 2 * 32);

		if (s && draw)
		{
			int y;
			gmem = specData;
			if (!ws)
			{
				for (y = 0; y < 8; y++) 
				{
					int *smem = (int *) gmem;
					for (int x = 0; x < 76; x ++)
						*smem++ = 0x0101;
					gmem += 76*2*2;
					memset(gmem,0,76*2*2);
					gmem += 76*2*2;
				}
			}
			else
			{
				gmem += 76*2*(32-10);
				for (y = 0; y < 10; y++) 
				{
					memset(gmem,0,76);
					gmem += 76*2;
				}
			}
		} 
		else if (draw)
		{
			int y;
			gmem = specData+76*2*16;
			if (!ws)
			{
				for (y = 0; y < 8; y++) 
				{
					int *smem = (int *) gmem;
					for (int x = 0; x < 76/4; x ++)
						*smem++ = 0x10001;
					gmem += 76*2;
					memset(gmem,0,76);
					gmem += 76*2;
				}
			}
			else
			{
				gmem += 76*2*(16-5);
				for (y = 0; y < 5; y++) 
				{
					memset(gmem,0,76/2);
					gmem += 76*2;
				}
			}
		}

		if (!values)
		{
			memset(bx,0,75*sizeof(int));
		}
		else if (!s) // singlesize
		{
			if (!ws) // non windowshade singlesize
			{
				if (config_sa == 2) // non windowshade singlesize oscilliscope
				{
					gmem = specData + 76*2*14;
					if (draw) 
					{
						int lv=-1;
						if (((config_safire>>2)&3)==0) for (x = 0; x < 75; x ++)
						{
							int v; char c;
							v = (((int) ((signed char *)values)[x])) + 8;
							if (v < 0) v = 0 ; if (v > 15) v = 15; c = v/2-4; if (c < 0) c = -c; c += 18;
							gmem[v*76*2] = c;
							gmem++;
						}
						else if (((config_safire>>2)&3)==1) for (x = 0; x < 75; x ++)
						{
							int v,t; char c;
							v = (((int) ((signed char *)values)[x])) + 8;
							if (v < 0) v = 0 ; if (v > 15) v = 15; c = v/2-4; if (c < 0) c = -c; c += 18;
							if (lv == -1) lv=v;
							t=lv;
							lv=v;
							if (v >= t) while (v >= t) gmem[v--*76*2] = c;
							else while (v < t) gmem[v++*76*2] = c;
							gmem++;
						}
						else if (((config_safire>>2)&3)==2) for (x = 0; x < 75; x ++) // solid
						{
							int v; char c;
							v = (((int) ((signed char *)values)[x])) + 8;
							if (v < 0) v = 0 ; if (v > 15) v = 15; c = v/2-4; if (c < 0) c = -c; c += 18;
							if (v > 7) while (v > 7) gmem[v--*76*2] = c;
							else while (v <= 7) gmem[v++*76*2] = c;
							gmem++;
						}
					}
				} else if (config_sa == 3) {
					int y, v, v1;
					float scaleFactor = 75.0f / 255.0f;
					v = static_cast<int>(vuData[0] * scaleFactor);
					v1 = static_cast<int>(vuData[1] * scaleFactor);
					int y1 = 24; // Position of the first bar
					int y2 = 16; // Position of the second bar, adjusted to avoid overlap

					if (v_bx[0] <= v*256) {
						v_bx[0]=v*256;
						v_vx[0]=3.0f;
					}

					if (v_bx[1] <= v1*256) {
						v_bx[1]=v1*256;
						v_vx[1]=3.0f;
					}

					// Draw first VU meter bar
					for (y = 0; y < 7; y++) { // Iterate over the height (7 pixels tall)
						gmem = specData + (76 * 2 * (y1 + y)); // Calculate the starting point for each row
						for (int x = 0; x < v && (x < 76); x++) { // Ensure bounds
							unsigned char colorIndex = static_cast<unsigned char>((-x + 84) / 4.7f);
							*gmem = colorIndex; // Set the pixel value for the bar
							gmem += 1; // Move horizontally by 1 pixel
						}

						// Draw the peak marker horizontally
						int peakPos = static_cast<int>(v_bx[0]) / 256; // Calculate the position of the peak marker
						if (peakPos > 0 && peakPos < 76) { // Ensure the peak position is within bounds
							specData[76 * 2 * (y1 + y) + peakPos] = 23; // Set the pixel value for the peak marker
						}
					}

					// Draw second VU meter bar
					for (y = 0; y < 7; y++) { // Iterate over the height (7 pixels tall)
						gmem = specData + (76 * 2 * (y2 + y)); // Calculate the starting point for each row
						for (int x = 0; x < v1 && (x < 76); x++) { // Ensure bounds
							unsigned char colorIndex = static_cast<unsigned char>((-x + 84) / 4.7f);
							*gmem = colorIndex; // Set the pixel value
							gmem += 1; // Move horizontally by 1 pixel
						}
						// Draw the peak marker horizontally
						int peakPos = static_cast<int>(v_bx[1]) / 256; // Calculate the position of the peak marker
						if (peakPos > 0 && peakPos < 76) { // Ensure the peak position is within bounds
							specData[76 * 2 * (y2 + y) + peakPos] = 23; // Set the pixel value for the peak marker
						}
					}

					v_bx[0] -= (int)v_vx[0];
					v_vx[0] *= spfo;
					if (v_bx[0] < 0) v_bx[0]=0;

					v_bx[1] -= (int)v_vx[1];
					v_vx[1] *= spfo;
					if (v_bx[1] < 0) v_bx[1]=0;
				} else if (config_sa == 0)
				{
					
				}
				else // non windowshade singlesize spectrum analyzer
				{
					for (x = 0; x < 75; x ++)
					{
						int y,v,t;
						t=x&~3;
						if (!(config_safire&32)) 
						{
							int a=values[t],b=values[t+1],c=values[t+2],d=values[t+3];
							v = a+b+c+d;//-min(a,min(b,min(c,d)));
							v/=4;
						}
						else v = (((int)values[x]));
						if (v > 15) v = 15;
						if ((v<<4) < bx[x]) v = (bx[x]-=dbx)>>4;
						else bx[x] = v<<4;
						if (bx[x] < 0) bx[x] = 0;
						if (v < 0) v = 0;
						gmem = specData + 76*2*14 + x;
						if ((config_safire&3)==1) t = v+2;
						else if ((config_safire&3)==2) t=17-(v);
						else t = 17;
				
						if (t_bx[x] <= v*256) {
							t_bx[x]=v*256;
							t_vx[x]=3.0f;
						}
						if (draw && (config_safire&32 || (x&3)!=3)) 
						{
							if ((config_safire&3)!=2) for (y = 0; y < v; y ++)
							{
								*gmem = t-y;
								gmem += 76*2;
							}
							else for (y = 0; y < v; y ++)
							{
								*gmem = t;
								gmem += 76*2;
							}
							if (config_sa_peaks && t_bx[x]/256 >= 0 && t_bx[x]/256 <= 15)
							{
								specData[76*2*14 + (t_bx[x]/256)*76*2 + x]=23;
							}
						}
						t_bx[x] -= (int)t_vx[x];
						t_vx[x] *= spfo;
						if (t_bx[x] < 0) t_bx[x]=0;
					}
				}
			}
			else // windowshade singlesize
			{
				if (config_sa==1)  // windowshade singlesize spectrum analyzer
				{
					gmem = specData+76*2*(32-5);
					for (x = 0; x < 37; x ++)
					{
						int y,v,t;
						t=((x)&~3)*2;
						if (!(config_safire&32)) 
						{
							int a=values[t],b=values[t+1],c=values[t+2],d=values[t+3];
							v = a+b+c+d;//-min(a,min(b,min(c,d)));
							v/=4;
						}
						else v = (((int)values[x*2])+((int)values[x*2+1]))/2;
						if (v > 15) v = 15;
						if ((v<<4) < bx[x*2]) v = (bx[x*2]-=dbx)>>4;
						else bx[x*2] = v<<4;
						if (bx[x*2] < 0) bx[x*2] = 0;
						if (v < 0) v = 0;
						gmem = specData + 76*2*(32-5) + x;
						if ((config_safire&3)==1) t = v+2;
						else if ((config_safire&3)==2) t=17-(v);
						else t = 17;
				
						if (t_bx[x*2] <= v*256) {
							t_bx[x*2]=v*256;
							t_vx[x*2]=3.0f;
						}
						v = (v * 5)/15;
						if (v > 5) v=5;
						if (draw && (config_safire&32 || (x&3)!=3)) 
						{
							int poo=(t_bx[x*2]*5)/15/256;
							if ((config_safire&3)!=2) for (y = 0; y < v; y ++)
							{
								*gmem = t-(y*15)/5;
								gmem += 76*2;
							}
							else for (y = 0; y < v; y ++)
							{
								*gmem = t;
								gmem += 76*2;
							}            
							if (config_sa_peaks && poo >= 0 && poo <= 4)
							{
								specData[76*2*(32-5) + poo*76*2 + x]=23;
							}
						}
						t_bx[x*2] -= (int)t_vx[x*2];
						t_vx[x*2] *= spfo;
						if (t_bx[x*2] < 0) t_bx[x*2]=0;
					}
				}
				else if (config_sa == 2)  // windowshade singlesize oscilliscope 
				{
					int wm=((config_safire>>2)&3);
					int lastv=-5;
					gmem = specData+76*2*(32-5);
					for (x = 0; x < 38; x ++)
					{
						int v = (((int) ((signed char *)values)[x])) + 8;
						v *= 5;
						v /= 16;
						if (v < 0) v = 0 ; if (v > 4) v = 4;
						if (wm==0 || lastv==-5)
						{
							lastv=v;
							gmem[x+v*76*2] = 18;
						}
						else if (wm == 1)
						{
							int tmp=lastv;
							lastv=v;
							if (v >= tmp) while (v>=tmp) { gmem[x+v--*76*2] = 18; }
							else while (v<=tmp) { gmem[x+v++*76*2] = 18; }
						}
						else if (wm == 2)
						{
							if (v >= 2) while (v>=2) { gmem[x+v--*76*2] = 18; }
							else while (v<=2) { gmem[x+v++*76*2] = 18; }
						}
					}
				} else if (config_sa == 3) { //windowshade singlesize vu meter 
					int y, v, v1;
					float scaleFactor = 37.0f / 255.0f;
					v = static_cast<int>(vuData[0] * scaleFactor);
					v1 = static_cast<int>(vuData[1] * scaleFactor);
					int y1 = 30; // Position of the first bar
					int y2 = 27; // Position of the second bar, adjusted to avoid overlap

					if (v_bx[0] <= v*256) {
						v_bx[0]=v*256;
						v_vx[0]=3.0f;
					}

					if (v_bx[1] <= v1*256) {
						v_bx[1]=v1*256;
						v_vx[1]=3.0f;
					}

					// Draw first VU meter bar
					for (y = 0; y < 2; y++) { // Iterate over the height (7 pixels tall)
						gmem = specData + (76 * 2 * (y1 + y)); // Calculate the starting point for each row
						for (int x = 0; x < v && (x < 38); x++) { // Ensure bounds
							unsigned char colorIndex = static_cast<unsigned char>((-x + 41) / 2.3f);
							*gmem = colorIndex; // Set the pixel value for the bar
							gmem += 1; // Move horizontally by 1 pixel
						}

						// Draw the peak marker horizontally
						int peakPos = static_cast<int>(v_bx[0]) / 256; // Calculate the position of the peak marker
						if (peakPos >= 0 && peakPos < 38) { // Ensure the peak position is within bounds
							specData[76 * 2 * (y1 + y) + peakPos] = 23; // Set the pixel value for the peak marker
						}
					}

					// Draw second VU meter bar
					for (y = 0; y < 2; y++) { // Iterate over the height (7 pixels tall)
						gmem = specData + (76 * 2 * (y2 + y)); // Calculate the starting point for each row
						for (int x = 0; x < v1 && (x < 38); x++) { // Ensure bounds
							unsigned char colorIndex = static_cast<unsigned char>((-x + 41) / 2.3f);
							*gmem = colorIndex; // Set the pixel value
							gmem += 1; // Move horizontally by 1 pixel
						}
						// Draw the peak marker horizontally
						int peakPos = static_cast<int>(v_bx[1]) / 256; // Calculate the position of the peak marker
						if (peakPos >= 0 && peakPos < 38) { // Ensure the peak position is within bounds
							specData[76 * 2 * (y2 + y) + peakPos] = 23; // Set the pixel value for the peak marker
						}
					}

					v_bx[0] -= (int)v_vx[0];
					v_vx[0] *= spfo;
					if (v_bx[0] < 0) v_bx[0]=0;

					v_bx[1] -= (int)v_vx[1];
					v_vx[1] *= spfo;
					if (v_bx[1] < 0) v_bx[1]=0;
				}
			}
		}
		else // doublesize
		{
			if (!ws)
			{
				if (config_sa == 2)
				{
					gmem = specData;// + 76*2*16;
					if (draw) 
					{
						int lv=-1;
						if (((config_safire>>2)&3)==0) for (x = 0; x < 75*2; x += 2)
						{
							int v;	char c;
							v = (((int) ((signed char *)values)[x/2])) + 8;
							if (v < 0) v = 0; if (v > 15) v = 15; c = v/2-4; if (c < 0) c = -c; c += 18;
							gmem[v*76*2*2] = c;	gmem++[v*76*2*2 + 76*2] = c;
							gmem[v*76*2*2] = c;	gmem++[v*76*2*2 + 76*2] = c;
						}
						else if (((config_safire>>2)&3)==1) for (x = 0; x < 75*2; x += 2)
						{
							int v,t;	char c;
							v = (((int) ((signed char *)values)[x/2])) + 8;
							if (v < 0) v = 0; if (v > 15) v = 15; c = v/2-4; if (c < 0) c = -c; c += 18;
							if (lv == -1) lv=v;
							t=lv;
							lv=v;
							if (v >= t) while (v >= t) 
							{
								gmem[v*76*2*2] = c;
								gmem[v*76*2*2 + 76*2] = c;
								gmem[v*76*2*2 + 1] = c;
								gmem[v*76*2*2 + 76*2 + 1] = c;
								v--;
							}
							else while (v < t)
							{
								gmem[v*76*2*2] = c;
								gmem[v*76*2*2 + 76*2] = c;
								gmem[v*76*2*2 + 1] = c;
								gmem[v*76*2*2 + 76*2 + 1] = c;
								v++;
							}
							gmem+=2;
						}
						else if (((config_safire>>2)&3)==2) for (x = 0; x < 75*2; x += 2)
						{
							int v;	char c;
							v = (((int) ((signed char *)values)[x/2])) + 8;
							if (v < 0) v = 0; if (v > 15) v = 15; c = v/2-4; if (c < 0) c = -c; c += 18;
							if (v > 7) while (v > 7) 
							{
								gmem[v*76*2*2] = c;
								gmem[v*76*2*2 + 76*2] = c;
								gmem[v*76*2*2 + 1] = c;
								gmem[v*76*2*2 + 76*2 + 1] = c;
								v--;
							}
							else while (v <= 7)
							{
								gmem[v*76*2*2] = c;
								gmem[v*76*2*2 + 76*2] = c;
								gmem[v*76*2*2 + 1] = c;
								gmem[v*76*2*2 + 76*2 + 1] = c;
								v++;
							}
							gmem+=2;
						}
					}
				} if (config_sa == 3) { //doublesize vu meter
					int y, v, v1;
					float scaleFactor = 150.0f / 255.0f;
					v = static_cast<int>(vuData[0] * scaleFactor);
					v1 = static_cast<int>(vuData[1] * scaleFactor);
					//std::cout << "VU Data: " << v << ", " << v1 << std::endl;
					int y1 = 16; // Position of the first bar
					int y2 = 0; // Position of the second bar, adjusted to avoid overlap

					if (v_bx[0] <= v*256) {
						v_bx[0]=v*256;
						v_vx[0]=3.0f;
					}

					if (v_bx[1] <= v1*256) {
						v_bx[1]=v1*256;
						v_vx[1]=3.0f;
					}

					// Draw first VU meter bar
					for (y = 0; y < 7*2; y++) { // Iterate over the height (7 pixels tall)
					unsigned char colorIndex;
						gmem = specData + (76 * 2 * (y1 + y)); // Calculate the starting point for each row
						//v = 150;
						if (v >= 150) v = 150;
						// it is very stupid to assume that our value will never go above the maximum, evidently this can still happen
						for (int x = 0; x < v && (x < 150); x++) { // Ensure bounds
							if (config_vufire == 0) {
								colorIndex = static_cast<unsigned char>((-x + (84*2)) / (4.667f * 2)); // normal
							} else if (config_vufire == 1) {
								colorIndex = static_cast<unsigned char>((-x + (v - 150) + (84*2)) / (4.667f * 2)); // fire
							} else if (config_vufire == 2) {
								colorIndex = static_cast<unsigned char>((-v + (84*2)) / (4.667f * 2)); // line
							}
							*gmem = colorIndex; // Set the pixel value for the bar
							gmem += 1; // Move horizontally by 1 pixel
						}

						// Draw the peak marker horizontally
						int peakPos = static_cast<int>(v_bx[0]) / 256; // Calculate the position of the peak marker
						if (config_vu_peaks && (peakPos > 1 && peakPos < 152)) { // Ensure the peak position is within bounds
							specData[76 * 2 * (y1 + y) + peakPos] = 23; // Set the pixel value for the peak marker
							specData[76 * 2 * (y1 + y) + peakPos + 1] = 23; // Set the pixel value for the peak marker
						}
					}

					// Draw second VU meter bar
					for (y = 0; y < 7*2; y++) { // Iterate over the height (7 pixels tall)
					unsigned char colorIndex;
						gmem = specData + (76 * 2 * (y2 + y)); // Calculate the starting point for each row
						for (int x = 0; x < v1 && (x < 150); x++) { // Ensure bounds
							if (config_vufire == 0) {
								colorIndex = static_cast<unsigned char>((-x + (84*2)) / (4.667f * 2)); // normal
							} else if (config_vufire == 1) {
								colorIndex = static_cast<unsigned char>((-x + (v1 - 150) + (84*2)) / (4.667f * 2)); // fire
							} else if (config_vufire == 2) {
								colorIndex = static_cast<unsigned char>((-v1 + (84*2)) / (4.667f * 2)); // line
							}
							*gmem = colorIndex; // Set the pixel value
							gmem += 1; // Move horizontally by 1 pixel
						}
						// Draw the peak marker horizontally
						int peakPos = static_cast<int>(v_bx[1]) / 256; // Calculate the position of the peak marker
						if (config_vu_peaks && (peakPos > 1 && peakPos < 152)) { // Ensure the peak position is within bounds
							specData[76 * 2 * (y2 + y) + peakPos] = 23; // Set the pixel value for the peak marker
							specData[76 * 2 * (y2 + y) + peakPos + 1] = 23; // Set the pixel value for the peak marker
						}
					}

					v_bx[0] -= (int)v_vx[0];
					v_vx[0] *= vpfo;
					if (v_bx[0] < 0) v_bx[0]=0;

					v_bx[1] -= (int)v_vx[1];
					v_vx[1] *= vpfo;
					if (v_bx[1] < 0) v_bx[1]=0;
				}
				if (config_sa == 1)
				{
					for (x = 0; x < 75*2;)
					{
						int y,v, t;
						t=(x/2)&~3;
						if (!(config_safire&32)) 
						{
							int a=values[t],b=values[t+1],c=values[t+2],d=values[t+3];
							v = a+b+c+d;//-min(a,min(b,min(c,d)));
							v/=4;
						}
						else v = (((int)values[x/2]));
						if (v > 15) v = 15;
						if ((v<<4) < bx[x/2]) v = (bx[x/2]-=dbx)>>4;
						else bx[x/2] = v<<4;
						if (bx[x/2] < 0) bx[x/2] = 0;
						if (v < 0) v = 0;
						gmem = specData+x;
						if ((config_safire&3)==1) t = v+2;
						else if ((config_safire&3)==2) t = 17 - v;
						else t = 17;
						if (t_bx[x/2] <= v*256) {
							t_bx[x/2]=v*256;
							t_vx[x/2]=3.0f;
						}
						v*=2;
						if (draw && (config_safire&32 || ((x/2)&3)!=3)) 
						{
							if ((config_safire&3)!=2) for (y = 0; y < v; y ++)
							{
								gmem[0] = gmem[1] = t-y/2;
								gmem += 76*2;
							}
							else for (y = 0; y < v; y ++)
							{
								gmem[0] = gmem[1] = t;
								gmem += 76*2;
							}
							if (config_sa_peaks && t_bx[x/2]/256 > 0 && t_bx[x/2]/256 <= 15)
							{
								specData[(t_bx[x/2]/256)*76*4 + x]=specData[(t_bx[x/2]/256)*76*4 + x+1]=23;
								specData[(t_bx[x/2]/256)*76*4 + x + 76*2]=specData[(t_bx[x/2]/256)*76*4 + x+1+ 76*2]=23;
							}
						}
						t_bx[x/2] -= (int)t_vx[x/2];
						t_vx[x/2] *=spfo;
						if (t_bx[x/2] < 0) t_bx[x/2]=0;
						x+=2;
					}
				}
			}
			else
			{
				if (config_sa == 2) // doublesize window shade scope
				{
					int wm=((config_safire>>2)&3);
					int lastv=-5;
					gmem = specData+76*2*(32-10);
					for (x = 0; x < 75; x ++)
					{
						int v = (((int) ((signed char *)values)[x])) + 8;
						v *= 10;
						v /= 16;
						if (v < 0) v = 0 ; if (v > 9) v = 9;
						if (wm==0 || lastv==-5)
						{
							lastv=v;
							gmem[x+v*76*2] = 18;
						}
						else if (wm == 1)
						{
							int tmp=lastv;
							lastv=v;
							if (v >= tmp) while (v>=tmp) { gmem[x+v--*76*2] = 18; }
							else while (v<=tmp) { gmem[x+v++*76*2] = 18; }
						}
						else if (wm == 2)
						{
							if (v >= 4) while (v>=4) { gmem[x+v--*76*2] = 18; }
							else while (v<=4) { gmem[x+v++*76*2] = 18; }
						}
					}
				}
				if (config_sa == 1) { // doublesize window shade spectrum
					for (x = 0; x < 75; x ++)
					{
						int y,v,t;
						t=(x)&~3;
						if (!(config_safire&32)) 
						{
							int a=values[t],b=values[t+1],c=values[t+2],d=values[t+3];
							v = a+b+c+d;//-min(a,min(b,min(c,d)));
							v/=4;
						}
						else v = (int)values[x];
						if (v > 15) v = 15;
						if ((v<<4) < bx[x]) v = (bx[x]-=dbx)>>4;
						else bx[x] = v<<4;
						if (bx[x] < 0) bx[x] = 0;
						if (v < 0) v = 0;
						gmem = specData + 76*2*(32-10) + x;
						if ((config_safire&3)==1) t = v+2;
						else if ((config_safire&3)==2) t=17-(v);
						else t = 17;
				
						if (t_bx[x] <= v*256) {
							t_bx[x]=v*256;
							t_vx[x]=3.0f;
						}
						v = (v * 10)/15;
						if (draw && (config_safire&32 || (x&3)!=3)) 
						{
							int poo=(t_bx[x]*10)/15/256;
							if ((config_safire&3)!=2) for (y = 0; y < v; y ++)
							{
								*gmem = t-(y*15)/10;
								gmem += 76*2;
							}
							else for (y = 0; y < v; y ++)
							{
								*gmem = t;
								gmem += 76*2;
							}            
							if (config_sa_peaks && poo >= 0 && poo <= 9)
							{
								specData[76*2*(32-10) + poo*76*2 + x]=23;
							}
						}
						t_bx[x] -= (int)t_vx[x];
						t_vx[x] *= spfo;
						if (t_bx[x] < 0) t_bx[x]=0;
					}
				} if (config_sa == 3) { //windowshade doublesize vu meter
					int y, v, v1;
					float scaleFactor = 75.0f / 255.0f;
					v = static_cast<int>(vuData[0] * scaleFactor);
					v1 = static_cast<int>(vuData[1] * scaleFactor);
					int y1 = 27; // Position of the first bar
					int y2 = 22; // Position of the second bar, adjusted to avoid overlap

					if (v_bx[0] <= v*256) {
						v_bx[0]=v*256;
						v_vx[0]=3.0f;
					}

					if (v_bx[1] <= v1*256) {
						v_bx[1]=v1*256;
						v_vx[1]=3.0f;
					}

					// Draw first VU meter bar
					for (y = 0; y < 4; y++) { // Iterate over the height (7 pixels tall)
						gmem = specData + (76 * 2 * (y1 + y)); // Calculate the starting point for each row
						for (int x = 0; x < v && (x < 75); x++) { // Ensure bounds
							unsigned char colorIndex = static_cast<unsigned char>((-x + 84) / 4.7f);
							*gmem = colorIndex; // Set the pixel value for the bar
							gmem += 1; // Move horizontally by 1 pixel
						}

						// Draw the peak marker horizontally
						int peakPos = static_cast<int>(v_bx[0]) / 256; // Calculate the position of the peak marker
						if (peakPos >= 0 && peakPos < 75) { // Ensure the peak position is within bounds
							specData[76 * 2 * (y1 + y) + peakPos] = 23; // Set the pixel value for the peak marker
						}
					}

					// Draw second VU meter bar
					for (y = 0; y < 4; y++) { // Iterate over the height (7 pixels tall)
						gmem = specData + (76 * 2 * (y2 + y)); // Calculate the starting point for each row
						for (int x = 0; x < v1 && (x < 75); x++) { // Ensure bounds
							unsigned char colorIndex = static_cast<unsigned char>((-x + 84) / 4.7f);
							*gmem = colorIndex; // Set the pixel value
							gmem += 1; // Move horizontally by 1 pixel
						}
						// Draw the peak marker horizontally
						int peakPos = static_cast<int>(v_bx[1]) / 256; // Calculate the position of the peak marker
						if (peakPos >= 0 && peakPos < 75) { // Ensure the peak position is within bounds
							specData[76 * 2 * (y2 + y) + peakPos] = 23; // Set the pixel value for the peak marker
						}
					}

					v_bx[0] -= (int)v_vx[0];
					v_vx[0] *= spfo;
					if (v_bx[0] < 0) v_bx[0]=0;

					v_bx[1] -= (int)v_vx[1];
					v_vx[1] *= spfo;
					if (v_bx[1] < 0) v_bx[1]=0;
				}
			}
		}

		sa_safe--;
	}