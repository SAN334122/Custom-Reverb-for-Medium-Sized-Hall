/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include "CombFilter.h"
#include "AllPassFilter.h"
#include "EarlyReflections.h"

//==============================================================================
class CustomFreeverbAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    CustomFreeverbAudioProcessor();
    ~CustomFreeverbAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    // Left Channel DSP Chain
    EarlyReflections earlyReflectionsL;
    CombFilter lbcfL[8];
    AllPassFilter apfL[4];
    
    // Right Channel DSP Chain
    EarlyReflections earlyReflectionsR;
    CombFilter lbcfR[8];
    AllPassFilter apfR[4];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomFreeverbAudioProcessor)
};