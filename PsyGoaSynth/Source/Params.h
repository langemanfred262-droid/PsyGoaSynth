#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace psy
{
// Parameter layout is generated from tools/generate.py (Source/generated/ParamsGenerated.inc)
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
}
