/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"

CustomFreeverbAudioProcessorEditor::CustomFreeverbAudioProcessorEditor (CustomFreeverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // --- DRY / WET SLIDER ---
    dryWetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dryWetSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(dryWetSlider);

    dryWetLabel.setText("Dry/Wet", juce::dontSendNotification);
    dryWetLabel.setJustificationType(juce::Justification::centred);
    dryWetLabel.attachToComponent(&dryWetSlider, false);
    addAndMakeVisible(dryWetLabel);

    dryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "dry_wet", dryWetSlider);

    // --- ROOM SIZE SLIDER ---
    roomSizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    roomSizeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(roomSizeSlider);

    roomSizeLabel.setText("Room Size", juce::dontSendNotification);
    roomSizeLabel.setJustificationType(juce::Justification::centred);
    roomSizeLabel.attachToComponent(&roomSizeSlider, false);
    addAndMakeVisible(roomSizeLabel);

    roomSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "room_size", roomSizeSlider);

    setSize (400, 250);
}

CustomFreeverbAudioProcessorEditor::~CustomFreeverbAudioProcessorEditor() {}

void CustomFreeverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1f242d));

    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawText ("Custom Freeverb", getLocalBounds().removeFromTop(40), juce::Justification::centred, true);
}

void CustomFreeverbAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(30); // Save space for title

    auto sliderWidth = 120;
    auto sliderHeight = 140;

    dryWetSlider.setBounds(area.removeFromLeft(sliderWidth).withHeight(sliderHeight).withY(70));
    roomSizeSlider.setBounds(area.removeFromRight(sliderWidth).withHeight(sliderHeight).withY(70));
}

//==============================================================================
CustomFreeverbAudioProcessor::CustomFreeverbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
     apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    dryWetParam   = apvts.getRawParameterValue ("dry_wet");
    roomSizeParam = apvts.getRawParameterValue ("room_size");
}

CustomFreeverbAudioProcessor::~CustomFreeverbAudioProcessor()
{
}
juce::AudioProcessorValueTreeState::ParameterLayout CustomFreeverbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "dry_wet", 1 },
        "Dry / Wet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.30f // Default 30% Wet
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "room_size", 1 },
        "Room Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.50f // Default 0.50
    ));

    return layout;
}
//==============================================================================
const juce::String CustomFreeverbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CustomFreeverbAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CustomFreeverbAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CustomFreeverbAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double CustomFreeverbAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CustomFreeverbAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int CustomFreeverbAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CustomFreeverbAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String CustomFreeverbAudioProcessor::getProgramName (int index)
{
    return {};
}

void CustomFreeverbAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void CustomFreeverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
    // Classic Jezar Freeverb delay lengths calibrated for 44.1 kHz
    const int baseCombLengths[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    const int baseApfLengths[4]  = {225, 341, 441, 556};
    const int baseStereoSpread = 23;
    
    // Scale delay buffer lengths proportionally if running at 48kHz, 88.2kHz, or 96kHz
    const double scaleFactor = sampleRate / 44100.0;
    
    // Preparing Early Reflections modules
    earlyReflectionsL.prepare(sampleRate);
    earlyReflectionsR.prepare(sampleRate);
    
    float initialRoomSize = roomSizeParam != nullptr ? roomSizeParam->load() : 0.50f;
    float initialFeedback = initialRoomSize * 0.28f + 0.70f;

    // Scaling delay lengths for Left and Right channels
    for(int i = 0; i < 8; ++i)
    {
        int scaledCombL = static_cast<int>(std::round(baseCombLengths[i] * scaleFactor));
        int scaledCombR = static_cast<int>(std::round((baseCombLengths[i] + baseStereoSpread) * scaleFactor));
        
        lbcfL[i].prepare(scaledCombL, initialFeedback, 0.20f);
        lbcfR[i].prepare(scaledCombR, initialFeedback, 0.20f);
    }
    
    for(int i = 0; i < 4; ++i)
    {
        int scaledApfL = static_cast<int>(std::round(baseApfLengths[i] * scaleFactor));
        int scaledApfR = static_cast<int>(std::round((baseApfLengths[i] + baseStereoSpread) * scaleFactor));
        
        apfL[i].prepare(scaledApfL, 0.5f);
        apfR[i].prepare(scaledApfR, 0.5f);
    }
    

}

void CustomFreeverbAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CustomFreeverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
   if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
    && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

  #endif


void CustomFreeverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
        
        // --- READ GUI PARAMETER VALUES ---
        float dryWet = dryWetParam->load();
        float roomSize = roomSizeParam->load();
        
        // Freeverb room size to comb feedback mapping
       float combFeedback = (roomSize * 0.28f) + 0.70f;
       
       // Update comb filter feedback gains
    for (int i = 0; i < 8; ++i) {
        lbcfL[i].setFeedback(combFeedback);
        lbcfR[i].setFeedback(combFeedback);
    }
    
    float dryGain = 1.0f - dryWet;
    float wetGain = dryWet;
    
    auto* leftChannel  = buffer.getWritePointer(0);
    auto* rightChannel = totalNumOutputChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Reading Left and Right Channels independetly
        float dryLeft = leftChannel[sample];
        float dryRight = (rightChannel != nullptr) ? rightChannel[sample] : dryLeft;
        
        // Jezar's classic Freeverb input tuning factor is 0.015
        float inputL = dryLeft * 0.015f;
        float inputR = dryRight * 0.015f;
        
        // ---Left Channel DSP Engine---
        float erL = earlyReflectionsL.processSample(inputL);
        float lbcfSumL = 0.0f;
        for (int i = 0; i < 8; ++i) {
            lbcfSumL += lbcfL[i].processSample(inputL);
        }
        float apfOutL = lbcfSumL;
        for (int i = 0; i < 4; ++i) {
            apfOutL = apfL[i].processSample(apfOutL);
        }
        
        // --- RIGHT CHANNEL DSP ENGINE ---
        float erR = earlyReflectionsR.processSample(inputR);
        float lbcfSumR = 0.0f;
        for (int i = 0; i < 8; ++i) {
            lbcfSumR += lbcfR[i].processSample(inputR);
        }
        float apfOutR = lbcfSumR;
        for (int i = 0; i < 4; ++i) {
            apfOutR = apfR[i].processSample(apfOutR);
        }
        
        
        // --- WET SUM (Equal mix between ER and Tail) ---
        float wetL = (erL * 0.5f) + (apfOutL * 0.5f);
        float wetR = (erR * 0.5f) + (apfOutR * 0.5f);
        
        leftChannel[sample] = (dryLeft * dryGain) + wetL;
        if(rightChannel != nullptr)
        {
            rightChannel[sample] = (dryRight * dryGain) + wetR;
        }
    }
}

//==============================================================================
bool CustomFreeverbAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* CustomFreeverbAudioProcessor::createEditor()
{
    return new CustomFreeverbAudioProcessorEditor (*this);
}

//==============================================================================
void CustomFreeverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);  
}

void CustomFreeverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CustomFreeverbAudioProcessor();
}
