Module.onRuntimeInitialized = function () {
    console.log("WASM Module Loaded!");
    // Allocate a char array in memory for `values`
    const values = Module._malloc(152); // Example: 10 bytes

    // Allocate memory for vuData (e.g., 4 bytes)
    const vuData = Module._malloc(4);

    // Initialize vuData with some values
    const vuDataArray = [1, 2, 3, 4];
    Module.HEAPU8.set(vuDataArray, vuData);

    // Canvas setup
    const canvas = document.getElementById("specCanvas");
    const ctx = canvas.getContext("2d");

    // Known dimensions for specData
    const specDataWidth = 76 * 2;
    const specDataHeight = 16 * 2;
    const specDataLength = specDataWidth * specDataHeight; // 1 byte per pixel (grayscale)

    const fft = new FFT();

    // Set up audio context and analyser
    const audioContext = new (window.AudioContext || window.webkitAudioContext)();
    const analyser = audioContext.createAnalyser();
    BUFFER_SIZE = 2048;
    analyser.fftSize = BUFFER_SIZE;
    const bufferLength = analyser.frequencyBinCount;
    const dataArray = new Uint8Array(bufferLength);
    console.log(bufferLength);
    let sample = new Float32Array(BUFFER_SIZE).fill(0);

    let audioSource = null;

    let outSpectraldata = new Float32Array(BUFFER_SIZE / 2);

    let currentConfig = Module._get_config_sa();

    // Input element for selecting an audio file
    const audioFileInput = document.getElementById("audioFileInput");
    audioFileInput.addEventListener("change", function (event) {
    const file = event.target.files[0];
        if (file) {
            const reader = new FileReader();
            reader.onload = function (e) {
                audioContext.decodeAudioData(e.target.result, function (buffer) {
                    // Stop the previous audio if any
                    if (audioSource) {
                        audioSource.stop();
                    }
                    // Create a new audio buffer source from the uploaded file
                    audioSource = audioContext.createBufferSource();
                    audioSource.buffer = buffer;
                    audioSource.connect(analyser);
                    analyser.connect(audioContext.destination);
                    audioSource.start(0);
                });
            };
            reader.readAsArrayBuffer(file);
        }
    });

    canvas.addEventListener('click', () => {
        // Get the current value of config_sa
        console.log("Current config_sa value:", currentConfig);

        // Toggle or set a new value for config_sa
        const newConfig = (currentConfig + 1) % 3; // Cycle: 0 -> 1 -> 2 -> 0
        Module._set_config_sa(newConfig);

        // Log the updated value
        currentConfig = Module._get_config_sa();
        console.log("Updated config_sa value:", currentConfig);
    });

    function adjustFFT(outSpectraldata){
        const maxFreqIndex = 512;
        const logMaxFreqIndex = Math.log10(maxFreqIndex);
        const logMinFreqIndex = 0;
        const scale = 0.95;

        const targetSize = 75;

        for (let x = 0; x < targetSize; x++) {
            const linearIndex = x / (targetSize - 1) * (maxFreqIndex - 1);
            const logScaledIndex = logMinFreqIndex + (logMaxFreqIndex - logMinFreqIndex) * x / (targetSize - 1);
            const logIndex = Math.pow(10, logScaledIndex);
            const scaledIndex = (1.0 - scale) * linearIndex + scale * logIndex;
            let index1 = Math.floor(scaledIndex);
            let index2 = Math.ceil(scaledIndex);

            if (index1 >= maxFreqIndex) {
                index1 = maxFreqIndex - 1;
            }
            if (index2 >= maxFreqIndex) {
                index2 = maxFreqIndex - 1;
            }

                if (index1 === index2) {
                    sample[x] = outSpectraldata[index1];
                } else {
                    const frac2 = scaledIndex - index1;
                    const frac1 = 1.0 - frac2;
                    sample[x] = frac1 * outSpectraldata[index1] + frac2 * outSpectraldata[index2];
                }
            }
        }

    function updateAndRender() {
        // Get the audio data (time domain)
        analyser.getByteTimeDomainData(dataArray);

        // Copy audio data to `values` in WebAssembly memory
        const draw = 1; // Example integer value
        for (let i = 0; i < 75; i++) {
            // Store FFT data into the first 75 positions
            const fftValue = Math.round(sample[i] * 4); // Round the FFT value
            const transformedFFTValue = fftValue; // Clamp to -128..127
            Module.HEAP8[values + i] = transformedFFTValue; // Store FFT value

            // Store audio data into the next 75 positions
            const audioValue = ((dataArray[Math.floor(i * (dataArray.length / 75))] - 128) / 8) + 32.5;
            const transformedAudioValue = audioValue; // Clamp to -128..127
            Module.HEAP8[values + i + 76] = transformedAudioValue; // Store raw audio value
        }

        const inputArray = new Float32Array(BUFFER_SIZE);
        analyser.getFloatTimeDomainData(inputArray);
        fft.timeToFrequencyDomain(inputArray, outSpectraldata);

        adjustFFT(outSpectraldata);

        // Call the `draw_sa` function to update `specData`
        //Module._calcVuData(vuData, values2, 1, 16);
        if (currentConfig === 1){
            Module._draw_sa(values, draw, vuData);
        } else {
            Module._draw_sa(values+76, draw, vuData);
        }

        // Access the `specData` array from WASM
        const specDataPtr = Module._get_specData();
        const specData = new Uint8Array(Module.HEAPU8.buffer, specDataPtr, specDataLength);

        // Create an ImageData object to render the pixels
        const imageData = ctx.createImageData(specDataWidth, specDataHeight);

        let colors = [
            [0, 0, 0],          // color 0 = black
            [24, 24, 41],       // color 1 = grey for dots
            [239, 49, 16],      // color 2 = top of spec
            [206, 41, 16],      // 3
            [214, 90, 0],       // 4
            [214, 102, 0],      // 5
            [214, 115, 0],      // 6
            [198, 123, 8],      // 7
            [222, 165, 24],     // 8
            [214, 181, 33],     // 9
            [189, 222, 41],     // 10
            [148, 222, 33],     // 11
            [41, 206, 16],      // 12
            [50, 190, 16],      // 13
            [57, 181, 16],      // 14
            [49, 156, 8],       // 15
            [41, 148, 0],       // 16
            [24, 132, 8],       // 17
            [255, 255, 255],    // 18 = osc 1
            [214, 214, 222],    // 19 = osc 2
            [181, 189, 189],    // 20 = osc 3
            [160, 170, 175],    // 21 = osc 4
            [148, 156, 165],    // 22 = osc 4
            [150, 150, 150]     // 23 = analyzer peak
        ]

        function getColor(index, channel) {
            if (index >= colors.length) return 0;
            return colors[index][channel];
        }

        for (let i = 0; i < specDataLength; i++) {
            const brightness = specData[i]; // Pixel brightness from specData
            const baseIndex = i * 4;
            imageData.data[baseIndex] = getColor(brightness, 0);       // Red channel
            imageData.data[baseIndex + 1] = getColor(brightness, 1);   // Green channel
            imageData.data[baseIndex + 2] = getColor(brightness, 2);   // Blue channel
            imageData.data[baseIndex + 3] = 255;          // Alpha (fully opaque)
        }
        ctx.putImageData(imageData, 0, 0);

        // Loop the function
        requestAnimationFrame(updateAndRender);
    }

    // Start the loop
    updateAndRender();

    // Clean up memory on unload (optional but good practice)
    window.addEventListener("beforeunload", () => {
        Module._free(values);
        Module._free(vuData);
    });
};