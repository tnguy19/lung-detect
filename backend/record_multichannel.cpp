#include <iostream>
#include <fstream>
#include <portaudio.h>

#define SAMPLE_RATE 44100       // Sample rate in Hz
#define NUM_CHANNELS 4          // Number of channels (example: 4-channel audio)
#define FRAMES_PER_BUFFER 256   // Frames per buffer (adjust for latency/performance)

// File output streams for each channel
std::ofstream channelFiles[NUM_CHANNELS];

// PortAudio callback function to handle real-time audio input
static int audioCallback(const void *inputBuffer, 
                         void *outputBuffer, 
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData) {

    // Cast the input buffer to a float array (assuming 32-bit float input format)
    const float *in = static_cast<const float*>(inputBuffer);

    // Split the interleaved audio data into separate channels and write to files
    for (unsigned long frame = 0; frame < framesPerBuffer; ++frame) {
        for (int channel = 0; channel < NUM_CHANNELS; ++channel) {
            // Write each channel's sample to the corresponding file
            channelFiles[channel].write(reinterpret_cast<const char*>(&in[frame * NUM_CHANNELS + channel]), sizeof(float));
        }
    }

    return paContinue;  // Keep the stream running
}

int main() {
    PaStream *stream;
    PaError err;

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

    // Open output files for each audio channel
    for (int i = 0; i < NUM_CHANNELS; ++i) {
        std::string fileName = "channel_" + std::to_string(i) + ".raw";  // Output file for each channel
        channelFiles[i].open(fileName, std::ios::binary);
        if (!channelFiles[i].is_open()) {
            std::cerr << "Failed to open file for channel " << i << std::endl;
            return -1;
        }
    }

    // Set up input stream parameters
    PaStreamParameters inputParameters;
    inputParameters.device = Pa_GetDefaultInputDevice();  // Use default audio input device
    if (inputParameters.device == paNoDevice) {
        std::cerr << "No default input device found" << std::endl;
        return -1;
    }
    inputParameters.channelCount = NUM_CHANNELS;          // Number of input channels (4 in this case)
    inputParameters.sampleFormat = paFloat32;             // Use 32-bit floating point audio format
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;  // No additional API-specific info

    // Open the audio stream with input parameters
    err = Pa_OpenStream(&stream, 
                        &inputParameters, 
                        nullptr,       // No output stream (input only)
                        SAMPLE_RATE,   // Sample rate
                        FRAMES_PER_BUFFER, // Frames per buffer (adjustable for latency)
                        paClipOff,     // No clipping
                        audioCallback, // Our callback function to handle audio input
                        nullptr);      // No user data

    if (err != paNoError) {
        std::cerr << "Failed to open stream: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

    // Start the audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "Failed to start stream: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

    std::cout << "Recording... Press Enter to stop." << std::endl;
    std::cin.get();  // Wait for user input to stop recording

    // Stop the audio stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "Failed to stop stream: " << Pa_GetErrorText(err) << std::endl;
    }

    // Close the audio stream
    Pa_CloseStream(stream);
    Pa_Terminate();  // Clean up PortAudio

    // Close all the output files for the channels
    for (int i = 0; i < NUM_CHANNELS; ++i) {
        channelFiles[i].close();
    }

    std::cout << "Recording stopped." << std::endl;
    return 0;
}
