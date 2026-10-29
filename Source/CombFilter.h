#pragma once
#include <vector>

class LowPassFeedbackCombFilter {
public:
    void prepare(int delaySamples, float feedback, float damping) {
        buffer.assign(delaySamples, 0.0f);
        delayLen = delaySamples;
        g = feedback;
        d = damping;
        writePtr = 0;
        filterState = 0.0f;
    }
    
    // Dynamic feedback update method for Room Size parameter
    void setFeedback(float newG) {
        g = newG;
    }

    inline float processSample(float input) {
        if (delayLen == 0) return input;
        
        float delayed = buffer[writePtr];
        
        // 1-Pole Low-pass filter in feedback path
        filterState = (delayed * (1.0f - d)) + (filterState * d);
        
        // Write to circular delay buffer
        buffer[writePtr] = input + (filterState * g);
        writePtr = (writePtr + 1) % delayLen;
        
        return delayed;
    }

private:
    std::vector<float> buffer;
    int delayLen = 0;
    int writePtr = 0;
    float g = 0.0f;
    float d = 0.0f;
    float filterState = 0.0f;
};

using CombFilter = LowPassFeedbackCombFilter;
