#include "Params.h"

namespace psy
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
#include "generated/ParamsGenerated.inc"
    return layout;
}
}
