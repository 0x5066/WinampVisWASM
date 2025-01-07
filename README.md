# Winamp Visualizer in WebAssembly

This is a project to study and understand how Winamp gets the audio for visualization.

Through this, the web browser acts as both the "Input plugin" and "Winamp" at the same time, as the "Input plugin" portion of this project (although in a *very* crude form, as the call to ``sa_addpcmdata`` is called in a thread to simulate Winamp calling the ``play()`` function in a plugin) provides "Winamp" (also the browser) with the audio data needed to process and visualize, thanks to the Web Audio API.

The "Winamp" portion is the simple audio player controls, visualizer and upload button, aka the frontend.

# Local hosting (and development)

To get this running, you'll need:
- Visual Studio Code
- The [Live Preview](https://marketplace.visualstudio.com/items?itemName=ms-vscode.live-server) extension and setting the ``Cross-Origin-Embedder-Policy: require-corp`` and ``Cross-Origin-Opener-Policy: same-origin`` headers in its settings (alternatively you can spin up a local server, though you won't have the live reloading aspect of it)
- Emscripten (to compile the source files to WASM)

Build instructions:
- For now, follow the rough instructions you'll find in ``main.h``, this however should be turned into a dedicated build script.
- Only compile the ``pffft`` part if you changed something about it.
