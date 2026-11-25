#include "Presets.h"

namespace psy
{
const std::vector<PresetDef>& getPresets()
{
    static const std::vector<PresetDef> presets = {
#include "generated/PresetData.inc"
    };
    return presets;
}
}
