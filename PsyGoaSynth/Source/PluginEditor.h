#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

// ------------------------------------------------------------------------------------------------------------
class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle,
                           juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

// ------------------------------------------------------------------------------------------------------------
class KnobComp : public juce::Component
{
public:
    KnobComp (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& text, juce::Colour);
    void resized() override;
private:
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
};

class ComboComp : public juce::Component
{
public:
    ComboComp (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& text, juce::Colour);
    void resized() override;
private:
    juce::ComboBox box;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
};

class ToggleComp : public juce::Component
{
public:
    ToggleComp (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& text, juce::Colour);
    void resized() override;
private:
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attach;
};

// 16-step trance gate pattern editor (click / drag to paint steps)
class StepGrid : public juce::Component, private juce::Timer
{
public:
    StepGrid (PsyGoaProcessor&, juce::Colour);
    ~StepGrid() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    void timerCallback() override { repaint(); }
    int stepAt (juce::Point<int>) const;
    void setStep (int step, bool on);
    PsyGoaProcessor& proc;
    juce::Colour colour;
    juce::RangedAudioParameter* patternParam = nullptr;
    juce::RangedAudioParameter* lenParam = nullptr;
    int lastPainted = -1;
    bool paintValue = true;
};

class InfoComp : public juce::Component
{
public:
    InfoComp();
    void setText (const juce::String&);
    void resized() override { label.setBounds (getLocalBounds()); }
private:
    juce::Label label;
};

// ------------------------------------------------------------------------------------------------------------
struct PanelInfo
{
    struct Item { juce::Component* comp; int w, h; };
    juce::String title;
    juce::Colour colour;
    juce::Rectangle<int> bounds;
    std::vector<Item> items;
};

class PanelCanvas : public juce::Component
{
public:
    explicit PanelCanvas (std::vector<PanelInfo>& p) : panels (p) {}
    void paint (juce::Graphics&) override;
private:
    std::vector<PanelInfo>& panels;
};

// ------------------------------------------------------------------------------------------------------------
class PsyGoaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PsyGoaEditor (PsyGoaProcessor&);
    ~PsyGoaEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildUi();
    PanelInfo& addPanel (const juce::String& title, juce::Colour c, juce::Rectangle<int> bounds);
    void knob (PanelInfo&, const char* id, const char* label);
    void combo (PanelInfo&, const char* id, const char* label, int width);
    void toggle (PanelInfo&, const char* id, const char* label);
    void layoutPanel (PanelInfo&);

    PsyGoaProcessor& proc;
    NeonLookAndFeel laf;
    std::vector<PanelInfo> panels;
    PanelCanvas canvas { panels };
    std::vector<std::unique_ptr<juce::Component>> owned;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    InfoComp* info = nullptr;

    static constexpr int kW = 1320, kH = 690;
};
