Module.onRuntimeInitialized = function () {
    function mulDiv(a, b, c) {
        if (c === 0) {
            throw new Error("Division by zero");
        }
        // Perform division first if possible to avoid overflow
        if (Math.abs(a) < Math.abs(c)) {
            return Math.round((a * b) / c);
        } else {
            return Math.round(a * (b / c));
        }
    }
    if (Module._pthread_main_thread_initialize) {
        Module._pthread_main_thread_initialize();
    }
    console.log("Pthreads initialized.");

    if (typeof SharedArrayBuffer === 'undefined') {
        console.error("Browser does not support SharedArrayBuffer. Pthreads cannot be used.");
    }
    
    console.log("WASM Module Loaded!");
    BUFFER_SIZE = 2048;
    // Allocate a char array in memory for `values`
    const values = Module._malloc(BUFFER_SIZE * 2);

    // Canvas setup
    const canvas = document.getElementById("specCanvas");
    const ctx = canvas.getContext("2d");

    // Known dimensions for specData
    const specDataWidth = 76 * 2;
    const specDataHeight = 16 * 2;
    const specDataLength = specDataWidth * specDataHeight; // 1 byte per pixel (grayscale)

    // Set up audio context and analysers for stereo
    const audioContext = new (window.AudioContext || window.webkitAudioContext)();
    const splitter = audioContext.createChannelSplitter(2);
    const analyserLeft = audioContext.createAnalyser();
    const analyserRight = audioContext.createAnalyser();
    analyserLeft.fftSize = BUFFER_SIZE;
    analyserRight.fftSize = BUFFER_SIZE;
    const bufferLength = analyserLeft.frequencyBinCount;
    const leftData = new Uint8Array(bufferLength);
    const rightData = new Uint8Array(bufferLength);
    console.log(bufferLength);

    let audioSource = null;

    let currentConfig = Module._get_config_sa();

    Module._SpectralAnalyzer_Create();

    const newConfig = (currentConfig + 1) % 4; // Cycle: 0 -> 1 -> 2 -> 0
    Module._set_config_sa(newConfig);
    Module._sa_setthread(newConfig);

    // Log the updated value
    currentConfig = Module._get_config_sa();

    let numberOfChannels;

    // for some reason firefox hates this, chrome is behaving though
    //int latency = output->Open(sample_rate, channels, bps, buffer_len_ms, pre_buffer_ms);
    // setting the "pre_buffer_ms" to anything above 0 (well, anything extreme that is, like 60000)
    // causes the nasties of low frequencies to appear a lot more often than they really should
    const latency = (audioContext.sampleRate, 2, 16, 500, 0);
    const maxlatency_in_ms = latency;
    console.log("Max latency in ms:", latency);
    const nf = mulDiv(maxlatency_in_ms * 4, audioContext.sampleRate, 450000);

    Module._sa_init(nf);
    Module._vu_init(nf, audioContext.sampleRate);

    // Start the sa_addpcmdata_thread in a new thread with values
    Module.ccall('start_sa_addpcmdata_thread', null, ['number'], [values]);

    // Input element for selecting an audio file
    const audioPlayer = document.getElementById('audioPlayer');
    const audioFileInput = document.getElementById("audioFileInput");

    const threshold = 0.01; // Define a threshold for detecting missing data
    let silenceDuration = 0; // Counter for silence duration
    const sampleRate = audioContext.sampleRate; // Get the sample rate of the audio context
    const silenceThreshold = 0.125 * sampleRate; // 5 seconds of silence

    audioFileInput.addEventListener("change", function(e) {
        const file = e.target.files[0];
        if (file) {
            const objectURL = URL.createObjectURL(file);
            audioPlayer.src = objectURL;

            // Determine if the file is mono or stereo
            const reader = new FileReader();
            reader.onload = function(event) {
                audioContext.decodeAudioData(event.target.result, function(buffer) {
                    numberOfChannels = buffer.numberOfChannels;
                    if (numberOfChannels === 1) {
                        console.log("The audio file is mono.");
                    } else if (numberOfChannels === 2) {
                        console.log("The audio file is stereo.");
                    } else {
                        console.log("The audio file has " + numberOfChannels + " channels.");
                    }
                });
            };
            reader.readAsArrayBuffer(file);

            // Clean up the object URL when it's no longer needed
            audioPlayer.onended = function() {
                URL.revokeObjectURL(objectURL);
            };
        }
    });

    // Connect audio player to analyzers when playing starts
    audioPlayer.addEventListener('play', async function() {
        if (audioContext.state === 'suspended') {
            await audioContext.resume();
        }
        if (!audioSource) {
            audioSource = audioContext.createMediaElementSource(audioPlayer);
            audioSource.connect(splitter);
            splitter.connect(analyserLeft, 0);
            splitter.connect(analyserRight, 1);
            audioSource.connect(audioContext.destination);
        }
        // Ensure the correct thread mode is set when playback starts
        Module._sa_setthread(Module._get_config_sa());
    });

    // Get references to the UI elements
    const visualizationDropdown = document.getElementById('visualizationDropdown');
    const normalRadio = document.getElementById('normal');
    const fireRadio = document.getElementById('fire');
    const lineRadio = document.getElementById('line');
    const thinRadio = document.getElementById('thin');
    const thickRadio = document.getElementById('thick');
    const sapeaksCheckbox = document.getElementById('sapeaks');
    const falloffSpeedRange = document.getElementById('range23');
    const peakFalloffSpeedRange = document.getElementById('range33');
    const dotsRadio = document.getElementById('dots');
    const linesRadio = document.getElementById('lines');
    const solidRadio = document.getElementById('solid');
    const normalvuRadio = document.getElementById('normalvu');
    const firevuRadio = document.getElementById('firevu');
    const linevuRadio = document.getElementById('linevu');
    const vupeaksCheckbox = document.getElementById('vupeaks');
    const vuPeakFalloffSpeedRange = document.getElementById('range43');

    // Function to save configuration settings to localStorage
    function saveConfig() {
        localStorage.setItem('config_sa', Module._get_config_sa());
        localStorage.setItem('config_safire', Module._get_config_safire());
        localStorage.setItem('config_vufire', Module._get_config_vufire());
        localStorage.setItem('config_safalloff', Module._get_config_safalloff());
        localStorage.setItem('config_sa_peak_falloff', Module._get_config_sa_peak_falloff());
        localStorage.setItem('config_sa_peaks', Module._get_config_sa_peaks());
        localStorage.setItem('config_vu_peak_falloff', Module._get_config_vu_peak_falloff());
        localStorage.setItem('config_vu_peaks', Module._get_config_vu_peaks());
    }

    // Function to load configuration settings from localStorage
    function loadConfig() {
        if (localStorage.getItem('config_sa') !== null) {
            Module._set_config_sa(parseInt(localStorage.getItem('config_sa')));
        }
        if (localStorage.getItem('config_safire') !== null) {
            Module._set_config_safire(parseInt(localStorage.getItem('config_safire')));
        }
        if (localStorage.getItem('config_vufire') !== null) {
            Module._set_config_vufire(parseInt(localStorage.getItem('config_vufire')));
        }
        if (localStorage.getItem('config_safalloff') !== null) {
            Module._set_config_safalloff(parseInt(localStorage.getItem('config_safalloff')));
        }
        if (localStorage.getItem('config_sa_peak_falloff') !== null) {
            Module._set_config_sa_peak_falloff(parseInt(localStorage.getItem('config_sa_peak_falloff')));
        }
        if (localStorage.getItem('config_sa_peaks') !== null) {
            Module._set_config_sa_peaks(parseInt(localStorage.getItem('config_sa_peaks')));
        }
        if (localStorage.getItem('config_vu_peak_falloff') !== null) {
            Module._set_config_vu_peak_falloff(parseInt(localStorage.getItem('config_vu_peak_falloff')));
        }
        if (localStorage.getItem('config_vu_peaks') !== null) {
            Module._set_config_vu_peaks(parseInt(localStorage.getItem('config_vu_peaks')));
        }
    }

    // Load configuration settings from localStorage
    loadConfig();

    // Ensure the correct thread mode is set based on the loaded configuration
    Module._set_config_sa(Module._get_config_sa());
    Module._sa_setthread(Module._get_config_sa());

    canvas.addEventListener('click', () => {
        // Get the current value of config_sa
        //console.log("Current config_sa value:", currentConfig);

        // Toggle or set a new value for config_sa
        currentConfig = Module._get_config_sa();
        const newConfig = (currentConfig + 1) % 4; // Cycle: 0 -> 1 -> 2 -> 0
        Module._set_config_sa(newConfig);
        Module._sa_setthread(newConfig);

        // Log the updated value
        currentConfig = Module._get_config_sa();
        // yeah sure, let currentConfig just exist twice, why not

        // Update the dropdown value
        visualizationDropdown.value = currentConfig;
        saveConfig();
    });

    // Initialize UI elements with current configuration values
    visualizationDropdown.value = Module._get_config_sa();
    sapeaksCheckbox.checked = Module._get_config_sa_peaks() === 1;
    vupeaksCheckbox.checked = Module._get_config_vu_peaks() === 1;
    falloffSpeedRange.value = Module._get_config_safalloff();
    peakFalloffSpeedRange.value = Module._get_config_sa_peak_falloff();
    vuPeakFalloffSpeedRange.value = Module._get_config_vu_peak_falloff();

    const configSafire = Module._get_config_safire();
    if ((configSafire & 3) === 0) normalRadio.checked = true;
    else if ((configSafire & 3) === 1) fireRadio.checked = true;
    else if ((configSafire & 3) === 2) lineRadio.checked = true;

    if ((configSafire & (1 << 5)) === 0) thickRadio.checked = true;
    else thinRadio.checked = true;

    if ((configSafire & (3 << 2)) === 0) dotsRadio.checked = true;
    else if ((configSafire & (3 << 2)) === (1 << 2)) linesRadio.checked = true;
    else if ((configSafire & (3 << 2)) === (2 << 2)) solidRadio.checked = true;

    const configVufire = Module._get_config_vufire();
    if (configVufire === 0) normalvuRadio.checked = true;
    else if (configVufire === 1) firevuRadio.checked = true;
    else if (configVufire === 2) linevuRadio.checked = true;

    // Add event listeners to update the configuration variables and save them to localStorage
    visualizationDropdown.addEventListener('change', () => {
        Module._set_config_sa(parseInt(visualizationDropdown.value));
        const newConfig = visualizationDropdown.value; // Cycle: 0 -> 1 -> 2 -> 0
        Module._set_config_sa(newConfig);
        Module._sa_setthread(newConfig);

        // Log the updated value
        currentConfig = Module._get_config_sa();
        saveConfig();
    });

    normalRadio.addEventListener('change', () => {
        if (normalRadio.checked) {
            Module._set_config_safire(Module._get_config_safire() & ~3);
            saveConfig();
        }
    });

    fireRadio.addEventListener('change', () => {
        if (fireRadio.checked) {
            Module._set_config_safire((Module._get_config_safire() & ~3) | 1);
            saveConfig();
        }
    });

    lineRadio.addEventListener('change', () => {
        if (lineRadio.checked) {
            Module._set_config_safire((Module._get_config_safire() & ~3) | 2);
            saveConfig();
        }
    });

    thinRadio.addEventListener('change', () => {
        if (thinRadio.checked) {
            Module._set_config_safire(Module._get_config_safire() | (1 << 5));
            saveConfig();
        }
    });

    thickRadio.addEventListener('change', () => {
        if (thickRadio.checked) {
            Module._set_config_safire(Module._get_config_safire() & ~(1 << 5));
            saveConfig();
        }
    });

    sapeaksCheckbox.addEventListener('change', () => {
        Module._set_config_sa_peaks(sapeaksCheckbox.checked ? 1 : 0);
        saveConfig();
    });

    falloffSpeedRange.addEventListener('input', () => {
        Module._set_config_safalloff(parseInt(falloffSpeedRange.value));
        saveConfig();
    });

    peakFalloffSpeedRange.addEventListener('input', () => {
        Module._set_config_sa_peak_falloff(parseInt(peakFalloffSpeedRange.value));
        saveConfig();
    });

    dotsRadio.addEventListener('change', () => {
        if (dotsRadio.checked) {
            Module._set_config_safire(Module._get_config_safire() & ~(3 << 2));
            saveConfig();
        }
    });

    linesRadio.addEventListener('change', () => {
        if (linesRadio.checked) {
            Module._set_config_safire((Module._get_config_safire() & ~(3 << 2)) | (1 << 2));
            saveConfig();
        }
    });

    solidRadio.addEventListener('change', () => {
        if (solidRadio.checked) {
            Module._set_config_safire((Module._get_config_safire() & ~(3 << 2)) | (2 << 2));
            saveConfig();
        }
    });

    normalvuRadio.addEventListener('change', () => {
        if (normalvuRadio.checked) {
            Module._set_config_vufire(0);
            saveConfig();
        }
    });

    firevuRadio.addEventListener('change', () => {
        if (firevuRadio.checked) {
            Module._set_config_vufire(1);
            saveConfig();
        }
    });

    linevuRadio.addEventListener('change', () => {
        if (linevuRadio.checked) {
            Module._set_config_vufire(2);
            saveConfig();
        }
    });

    vupeaksCheckbox.addEventListener('change', () => {
        Module._set_config_vu_peaks(vupeaksCheckbox.checked ? 1 : 0);
        saveConfig();
    });

    vuPeakFalloffSpeedRange.addEventListener('input', () => {
        Module._set_config_vu_peak_falloff(parseInt(vuPeakFalloffSpeedRange.value));
        saveConfig();
    });

    function updateAndRender() {
        // Get both channels
        analyserLeft.getByteTimeDomainData(leftData);
        analyserRight.getByteTimeDomainData(rightData);
    
        // Allocate for stereo (twice the size)
        const dataArray16 = new Int16Array(leftData.length * 2); // *2 for stereo

        // Interleave left and right channels
        for (let i = 0; i < leftData.length; i++) {
            dataArray16[i * 2] = (leftData[i] - 128) << 8;     // Left channel
            dataArray16[i * 2 + 1] = (rightData[i] - 128) << 8; // Right channel

            if (numberOfChannels === 1){
                dataArray16[i * 2 + 1] = (leftData[i] - 128) << 8;
            }
        }

        Module.HEAP16.set(dataArray16, values >> 1);
        Module._set_playtime(audioPlayer.currentTime * 1000);
        //console.log(values);
        //Module._sa_addpcmdata(values, 2, 16, Module._in_getouttime());
        //console.log(Module._in_getouttime());
        
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

        let i = 0;
        for (let y = specDataHeight - 1; y >= 0; y--)
        for (let x = 0; x < specDataWidth; x++) {
            const brightness = specData[i]; // Pixel brightness from specData
            imageData.data[(((y * specDataWidth) + x) * 4)    ] = getColor(brightness, 0);   // Red channel
            imageData.data[(((y * specDataWidth) + x) * 4) + 1] = getColor(brightness, 1);   // Green channel
            imageData.data[(((y * specDataWidth) + x) * 4) + 2] = getColor(brightness, 2);   // Blue channel
            imageData.data[(((y * specDataWidth) + x) * 4) + 3] = 255;                                      // Alpha (fully opaque)
            i++;
        }
        ctx.putImageData(imageData, 0, 0);

        // Loop the function
        requestAnimationFrame(updateAndRender)
    }

    // Start the loop
    updateAndRender();

    // Clean up memory on unload (optional but good practice)
    window.addEventListener("beforeunload", () => {
        Module._free(values);
    });
};
