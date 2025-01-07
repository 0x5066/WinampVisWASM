#define _USE_MATH_DEFINES
#include <math.h>
#include "main.h"
#include "pffft/pffft.h"

static PFFFT_Setup *pffft_setup = nullptr;

extern "C" {
    void* pffft_aligned_malloc(size_t size) {
        return aligned_alloc(32, size);  // PFFFT requires 32-byte alignment
    }

    void pffft_aligned_free(void* ptr) {
        free(ptr);
    }
}

void fft_init()
{
    if (!pffft_setup)
        pffft_setup = pffft_new_setup(BUFFER_SIZE, PFFFT_REAL);
}

void fft_9(float* input)
{
    if (!pffft_setup)
        fft_init();

    pffft_transform(pffft_setup, input, input, nullptr, PFFFT_FORWARD);
}
