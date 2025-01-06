#include "main.h"
#include "AutoLock.h"
#include <math.h>
#include <cstdint>
//#include "WinampAttributes.h"
//#include "../nsutil/stats.h"
using namespace Nullsoft::Utility;

struct VU_list
{
	int timestamp;
	unsigned char data[2]; // two channels for now, we'll worry about more later
};

static VU_list *vu_bufs;
static int vu_position, vu_length, vu_size;
static int last_pos;

LockGuard VU_guard GUARDNAME("VU Guard");

extern "C" {
EMSCRIPTEN_KEEPALIVE
void vu_init(int numframes, int srate)
{
	AutoLock lock (VU_guard LOCKNAME("vu_init"));
	
	vu_length = 0;
	if (numframes < 1) numframes = 1;

	if (numframes > vu_size)
	{
		free(vu_bufs);
		vu_bufs = (VU_list *)calloc(numframes, sizeof(VU_list));
		vu_size = numframes;
	}
	vu_position = 0;
	vu_length = numframes;
	last_pos = 0;
}
}

void vu_deinit(void)
{
	AutoLock lock (VU_guard LOCKNAME("vu_deinit"));
	vu_length = 0;
}

void VU_Create()
{
	vu_length = vu_size = 0;
}

void VU_Destroy()
{
	free(vu_bufs);
	vu_bufs = 0;
	vu_size = 0;
}

int vu_add(char *values, int timestamp)
{
	AutoLock lock (VU_guard LOCKNAME("vu_add"));

	if (!vu_bufs || vu_length == 0)
		return 1;

	if (vu_length < 2)
	{
		vu_position = 0;
	}

	vu_bufs[vu_position].timestamp = timestamp;//+600;

	memcpy(vu_bufs[vu_position].data, values, 2);

	vu_position++;
	if (vu_position >= vu_length) 
		vu_position -= vu_length;
	
	return 0;
}

bool vu_get(int timestamp, unsigned char data[2])
{
	int x, closest = 1000000, closest_v = -1;
	int i;

	AutoLock lock (VU_guard LOCKNAME("vu_get"));

	if (!vu_bufs || vu_length==0)
		return false;

	if (vu_length < 2)
	{
		memcpy(data, vu_bufs[0].data, 2);
		return true;
	}

	i = last_pos;
	for (x = 0; x < vu_length; x ++)
	{
		int d;
		if (i >= vu_length) i = 0;
		d = timestamp - vu_bufs[i].timestamp;
		if (d < 0) d = -d;
		if (d < closest)
		{
			closest = d;
			closest_v = i;
		}
		else if (closest < 200) break;
		i++;
	}

	if (closest < 200 && closest_v >= 0)
	{
		last_pos = closest_v;
		memcpy(data, vu_bufs[closest_v].data, 2);
		return true;
	}

	return true;
}

int export_vu_get(int channel)
{
	if (channel>2 || channel<0)
		return -1;
	unsigned char data[2];
	int now = in_getouttime();
	if (vu_get(now, data)){
		return data[channel];
	} else {
		return -1;
	}
}

float historyL=0, historyR=0;
static void FillFloat(float *floatBuf, void *samples, size_t bps, size_t numSamples, size_t numChannels, float preamp) {
    if (floatBuf == nullptr || samples == nullptr || numSamples == 0 || numChannels == 0) {
        return;  // Early exit on invalid input
    }

    switch(bps) {
        case 8: {
            preamp /= 256.0f;
            uint8_t *samples8 = (uint8_t *)samples;  
            for (size_t x = 0; x < numSamples; x++) {
                floatBuf[x] = (float)(samples8[x * numChannels] - 128) * preamp;
            }
        } break;

        case 16: {
            preamp /= 32768.0f;
            int16_t *samples16 = (int16_t *)samples;  
            for (size_t x = 0; x < numSamples; x++) {
                floatBuf[x] = (float)samples16[x * numChannels] * preamp;
            }
        } break;

        case 24: {
            preamp /= 2147483648.0f;  
            uint8_t *samples8 = (uint8_t *)samples;  
            for (size_t x = 0; x < numSamples; x++) {
                if (x * numChannels * 3 + 2 >= numSamples * numChannels * 3) break; // Safety check

                int32_t temp = 0;
                temp |= (samples8[x * numChannels * 3 + 2] << 16);  
                temp |= (samples8[x * numChannels * 3 + 1] << 8);   
                temp |= (samples8[x * numChannels * 3 + 0]);        

                if (temp & 0x800000) {  
                    temp |= 0xFF000000;  
                }

                floatBuf[x] = (float)temp * preamp;
            }
        } break;

		case 32:
			{
				preamp /= 2147483648.0f;
			int32_t *samples32 = (int32_t *)samples;
			for (size_t x = 0; x != numSamples; x ++)
			{
				floatBuf[x] = (float)samples32[x*numChannels] * preamp;
			}
		}
		break;
	}
}

void nsutil_stats_RMS_F32(float* input, int size, float* rms_out) {
    float sum_of_squares = 0.0f;

    for (int i = 0; i < size; i++) {
        sum_of_squares += input[i] * input[i];  // Square each sample
    }

    *rms_out = std::sqrt(sum_of_squares / size);  // RMS calculation with scaling
}

void calcVuData(unsigned char *out, char *data, const int channels, const int bits)
{
	static float left_peak=10.0f, left_history=0.0f;
	static float right_peak=10.0f, right_history=0.0f;
	// 576
	float left[BUFFER_SIZE] = {0}, right[BUFFER_SIZE] = {0};
	float left_rms, right_rms;

	//std::cout << right_rms << std::endl;
	float scale = 24.0;

	FillFloat(left, data, bits, BUFFER_SIZE, channels, scale);
	if (channels > 1)
		FillFloat(right, data+(bits/8), bits, BUFFER_SIZE, channels, scale);
	else
		memcpy(right, left, BUFFER_SIZE*sizeof(float));
	
	nsutil_stats_RMS_F32(left, BUFFER_SIZE, &left_rms);
	nsutil_stats_RMS_F32(right, BUFFER_SIZE, &right_rms);

	// use a simple one-pole IIR to 'adjust' the vu meter to the songs loudness level
	// the small constant 
	left_peak = 0.855f*left_peak + 0.095f*left_rms + 0.5f;
	left_rms = 10.0f*left_rms /left_peak;

	right_peak = 0.855f*right_peak + 0.095f*right_rms + 0.5f;
	right_rms = 10.0f*right_rms /right_peak;

	float left_db=left_rms*left_rms;
	float right_db=right_rms*right_rms;
	
	
	
	if (left_db < left_history)
		left_db = left_db*0.2f + left_history*0.8f;
	else
		left_db = left_db*0.75f + left_history*0.25f;

	if (right_db < right_history)
		right_db = right_db*0.2f + right_history*0.8f;
	else
		right_db = right_db*0.75f + right_history*0.25f;

	left_history = left_db;
	right_history = right_db;
	left_db = std::min(left_db, 255.0f);
	left_db = std::max(left_db, 0.0f);
	right_db = std::min(right_db, 255.0f);
	right_db = std::max(right_db, 0.0f);

	out[0] = (unsigned char)(left_db );
	out[1] = (unsigned char)(right_db);
}