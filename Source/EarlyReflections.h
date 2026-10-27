#pragma once
#include <vector>
#include <cmath>

class EarlyReflections {
public:
    void prepare(double sampleRate) {
        srScale = static_cast<float>(sampleRate) / 44100.0f;
        
        // 5 Taps: [Delay time in ms, Gain scale]
        // Micro-delays simulating front/side wall reflections (10ms - 45ms)
        const std::pair<float, float> baseTaps[5] = {
            { 12.7f, 0.75f },
            { 19.3f, 0.60f },
            { 27.1f, 0.45f },
            { 34.5f, 0.35f },
            { 42.2f, 0.25f }
        };

        tapDelaysSamples.resize(5);
        tapGains.resize(5);

        for (int i = 0; i < 5; ++i) {
            // Convert ms to sample offsets and scale for sample rate
            float delayInMs = baseTaps[i].first;
            tapDelaysSamples[i] = static_cast<int>((delayInMs * 0.001f * sampleRate));
            tapGains[i] = baseTaps[i].second;
        }

        // Maximum buffer depth (~100ms capacity)
        maxDelaySamples = static_cast<int>(0.100f * sampleRate);
        buffer.assign(maxDelaySamples, 0.0f);
        writePtr = 0;
    }

    inline float processSample(float input) {
        if (maxDelaySamples == 0) return 0.0f;

        // Write current dry sample to ring buffer
        buffer[writePtr] = input;

        float erSum = 0.0f;

        // Sum up discrete delay taps
        for (size_t i = 0; i < tapDelaysSamples.size(); ++i) {
            int readPtr = writePtr - tapDelaysSamples[i];
            if (readPtr < 0) {
                readPtr += maxDelaySamples;
            }
            erSum += buffer[readPtr] * tapGains[i];
        }

        writePtr = (writePtr + 1) % maxDelaySamples;
        return erSum;
    }

private:
    std::vector<float> buffer;
    std::vector<int> tapDelaysSamples;
    std::vector<float> tapGains;
    int maxDelaySamples = 0;
    int writePtr = 0;
    float srScale = 1.0f;
};
