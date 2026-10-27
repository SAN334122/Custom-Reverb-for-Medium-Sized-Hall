#pragma once
#include <vector>

class AllPassFilter {
public:
    void prepare(int delaySamples, float feedback = 0.5f) {
        buffer.assign(delaySamples, 0.0f);
        delayLen = delaySamples;
        g = feedback;
        writePtr = 0;
    }

    inline float processSample(float input) {
        if (delayLen == 0) return input;

        float delayed = buffer[writePtr];
        buffer[writePtr] = input + (delayed * g);
        float outSample = -input + delayed;
        writePtr = (writePtr + 1) % delayLen;
        
        return outSample;
    }

private:
    std::vector<float> buffer;
    int delayLen = 0;
    int writePtr = 0;
    float g = 0.5f;
};

