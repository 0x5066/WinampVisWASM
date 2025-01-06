#include "main.h"

int playtime = 0;
extern "C" {

	void set_playtime(int value) {
        playtime = value;
    }

int in_getouttime(void)
{
	//std::cout << playtime << std::endl;
	return playtime;
}

void sa_addpcmdata_thread(char* audioData) {
    while (1) {
        // hardcoded to assume 2 channels, 16 bits per sample since we have no reason to change it
        // it's our setup anyway
        sa_addpcmdata(audioData, 2, 16, playtime);
        //std::cout << audioData << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Adjust sleep duration as needed
    }
}

void start_sa_addpcmdata_thread(char* audioData) {
    pthread_t thread;
    //std::cout << audioData << std::endl;
    pthread_create(&thread, nullptr, (void*(*)(void*))sa_addpcmdata_thread, audioData);
}

}

int vis_running()
{
	return 0;
}