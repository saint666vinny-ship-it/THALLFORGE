# THALLFORGE

Windows x64 guitar processor prototype, with pitch shifting as the first priority.

## Prototype features
- Pitch shift from -24 to +24 semitones
- Fine tuning from -100 to +100 cents
- Wet/dry mix
- Basic saturation, noise gate and output level
- Standalone JUCE desktop application

## Important limitations
This is an early prototype, not a finished commercial amp simulator. SoundTouch pitch shifting adds latency and may produce artifacts; it is not a zero-latency polyphonic whammy pedal. Cabinet IR loading, modeled amp/cab processing, preset management, and expression-style whammy control are not implemented yet.

## Get the Windows build
Open the repository's Actions tab, select **Build THALLFORGE for Windows**, then select **Run workflow**. When the run finishes, download the `THALLFORGE-Windows-x64` artifact and extract the ZIP. Run `THALLFORGE.exe`.

Use an ASIO driver for your interface where available. Start with a 128- or 256-sample buffer and raise it if you hear crackles.
