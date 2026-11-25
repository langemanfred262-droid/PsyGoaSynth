#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "Params.h"
#include "Presets.h"
#include "dsp/FMSynth.h"
#include "dsp/AcidFilter.h"
#include "dsp/TranceGate.h"
#include "dsp/ModFx.h"
#include "dsp/PingPongDelay.h"

class PsyGoaProcessor : public juce::AudioProcessor
{
public:
    PsyGoaProcessor();
    ~PsyGoaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PsyGoa Synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return (int) psy::getPresets().size(); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override { applyPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void applyPreset (int index);

    juce::AudioProcessorValueTreeState apvts;

    // read by the GUI
    std::atomic<double> uiBpm { 145.0 };
    std::atomic<bool> uiHostPlaying { false };
    std::atomic<bool> uiHostSync { false };
    std::atomic<int> uiGateStep { 0 };

private:
    float pv (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void updateParams();
    void handleMidi (const juce::MidiMessage&);

    psy::Synth synth;
    psy::AcidFilter filter;
    psy::TranceGate gate;
    psy::Phaser phaser;
    psy::Flanger flanger;
    psy::PingPongDelay delay;
    juce::Reverb reverb;
    juce::AudioBuffer<float> wetBuffer;
    bool reverbWasOn = false;

    double sampleRate = 44100.0;
    double freeBeat = 0.0;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PsyGoaProcessor)
};
