/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class CustomFreeverbAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    CustomFreeverbAudioProcessorEditor (CustomFreeverbAudioProcessor&);
    ~CustomFreeverbAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    CustomFreeverbAudioProcessor& audioProcessor;
    
    // Sliders & Labels
    juce::Slider dryWetSlider;
    juce::Label dryWetLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;

    juce::Slider roomSizeSlider;
    juce::Label roomSizeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> roomSizeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomFreeverbAudioProcessorEditor)
};
