#pragma once

#include <juce_core/juce_core.h>
#include <utility>
#include <vector>

namespace mf
{
constexpr int kNumBanks = 16;
constexpr int kPresetsPerBank = 16;
constexpr int kNumFactoryPresets = kNumBanks * kPresetsPerBank;

struct FactoryPreset
{
    juce::String name;
    // Plain (not normalised) values; parameters not listed stay at their defaults.
    std::vector<std::pair<juce::String, float>> values;
};

const std::vector<FactoryPreset>& getFactoryPresets();
const juce::StringArray& getBankNames();

// e.g. "C07 Glass Choir"
juce::String getFactoryPresetDisplayName (int index);
} // namespace mf
