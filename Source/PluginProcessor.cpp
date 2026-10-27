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
    // Initialize Early Reflections ring buffer
    earlyReflections.prepare(sampleRate);
    // Classic Jezar Freeverb delay lengths calibrated for 44.1 kHz
    const int baseLbcfDelays[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    const int baseApfDelays[4]  = {225, 341, 441, 556};

    // Scale delay buffer lengths proportionally if running at 48kHz, 88.2kHz, or 96kHz
    const float scale = static_cast<float>(sampleRate) / 44100.0f;

    // Initialize 8 LBCFs (feedback ~0.84, damping ~0.2)
    for (int i = 0; i < 8; ++i) {
        int delay = static_cast<int>(baseLbcfDelays[i] * scale);
        lbcf[i].prepare(delay, 0.84f, 0.20f);
    }

    // Initialize 4 APFs (feedback = 0.5)
    for (int i = 0; i < 4; ++i) {
        int delay = static_cast<int>(baseApfDelays[i] * scale);
        apf[i].prepare(delay, 0.5f);
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
        float inSample = leftChannel[sample];
        
        // 1. Scaling down the audio entering the reverb network to prevent gain buildup
        // Jezar's classic Freeverb input tuning factor is 0.015
        float reverbInput = inSample * 0.015f;
        
        // 2. Process Discrete Early Reflections
        float erOut = earlyReflections.processSample(reverbInput);

        // 3. Parallel LBCF Summation
        float lbcfSum = 0.0f;
        for (int i = 0; i < 8; ++i) {
            lbcfSum += lbcf[i].processSample(reverbInput);
        }

        // 4. Series APF Diffusion Pass
        float apfOut = lbcfSum;
        for (int i = 0; i < 4; ++i) {
            apfOut = apf[i].processSample(apfOut);
        }

        // 5. Dry/Wet Mix (70% Dry, 15% Early Reflections ,15% Reverb Tail)
        float outputSample = (inSample * 0.79f) +(erOut * 0.15f) + (apfOut * 0.15f);

        leftChannel[sample] = outputSample;
        if (rightChannel != nullptr) {
            rightChannel[sample] = outputSample;
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
