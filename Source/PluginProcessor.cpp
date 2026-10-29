/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
                       )
#endif
{
}

CustomFreeverbAudioProcessor::~CustomFreeverbAudioProcessor()
{
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

    // Scaling delay lengths for Left and Right channels
    for(int i = 0; i < 8; ++i)
    {
        int scaledCombL = static_cast<int>(std::round(baseCombLengths[i] * scaleFactor));
        int scaledCombR = static_cast<int>(std::round((baseCombLengths[i] + baseStereoSpread) * scaleFactor));
        
        lbcfL[i].prepare(scaledCombL, 0.84f, 0.20f);
        lbcfR[i].prepare(scaledCombR, 0.84f, 0.20f);
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
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void CustomFreeverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto* leftChannel = buffer.getWritePointer(0);
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
        
        // Output Mixing (70% Dry, 15% Early Reflections, 15% Late Tail)
        float dryGain = 0.70f;
        float erGain = 0.15f;
        float tailGain = 0.15f;
        
        float wetL = (erL * erGain) + (apfOutL * tailGain);
        float wetR = (erR * erGain) + (apfOutR * tailGain);
        
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
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void CustomFreeverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CustomFreeverbAudioProcessor();
}
