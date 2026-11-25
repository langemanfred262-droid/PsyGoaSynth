#include "PluginEditor.h"

namespace
{
juce::Font boldFont (float h) { return juce::Font (juce::FontOptions (h, juce::Font::bold)); }
juce::Font plainFont (float h) { return juce::Font (juce::FontOptions (h)); }
}

// ================================================================================================================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff0b0b14));
    setColour (juce::Label::textColourId, juce::Colour (0xffc8c8dc));
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe8e8ff));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1c1c2b));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffe8e8ff));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3a3a55));
    setColour (juce::ComboBox::arrowColourId, juce::Colour (0xff9aa0ff));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff14141f));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffe8e8ff));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff3b2a6b));
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1c1c2b));
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe8e8ff));
}

void NeonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float a0, float a1, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (5.0f);
    const float d = juce::jmin (b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const float r = d * 0.5f;
    const auto col = s.findColour (juce::Slider::rotarySliderFillColourId);

    g.setColour (juce::Colour (0xff171724));
    g.fillEllipse (c.x - r * 0.76f, c.y - r * 0.76f, r * 1.52f, r * 1.52f);

    juce::Path track;
    track.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, a0, a1, true);
    g.setColour (juce::Colour (0xff2b2b3f));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float ang = a0 + pos * (a1 - a0);
    juce::Path val;
    val.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, a0, ang, true);
    g.setColour (col.withAlpha (0.25f));
    g.strokePath (val, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (col);
    g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path ptr;
    ptr.addRoundedRectangle (-1.5f, -r * 0.72f, 3.0f, r * 0.38f, 1.5f);
    g.setColour (juce::Colours::white);
    g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void NeonLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (2.0f);
    const bool on = b.getToggleState();
    const auto col = b.findColour (juce::ToggleButton::tickColourId);
    g.setColour (on ? col.withAlpha (0.92f) : juce::Colour (0xff25253a));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (highlighted ? juce::Colours::white.withAlpha (0.6f) : col.withAlpha (0.7f));
    g.drawRoundedRectangle (r, 5.0f, 1.2f);
    g.setColour (on ? juce::Colours::black : juce::Colour (0xffb8b8d0));
    g.setFont (boldFont (11.0f));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

// ================================================================================================================
KnobComp::KnobComp (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, juce::Colour c)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 14);
    slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, c);
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (plainFont (11.5f));
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
    attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
}
void KnobComp::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    slider.setBounds (r);
}

ComboComp::ComboComp (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, juce::Colour)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
        box.addItemList (p->choices, 1);
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (plainFont (11.5f));
    addAndMakeVisible (box);
    addAndMakeVisible (label);
    attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, box);
}
void ComboComp::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    box.setBounds (r.removeFromTop (26).translated (0, 14).reduced (2, 0));
}

ToggleComp::ToggleComp (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, juce::Colour c)
{
    button.setButtonText (text);
    button.setColour (juce::ToggleButton::tickColourId, c);
    addAndMakeVisible (button);
    attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, button);
}
void ToggleComp::resized() { button.setBounds (getLocalBounds().withSizeKeepingCentre (getWidth(), 28).translated (0, 6)); }

// ================================================================================================================
StepGrid::StepGrid (PsyGoaProcessor& p, juce::Colour c) : proc (p), colour (c)
{
    patternParam = p.apvts.getParameter ("gate_pattern");
    lenParam = p.apvts.getParameter ("gate_len");
    startTimerHz (20);
}

void StepGrid::paint (juce::Graphics& g)
{
    const int pattern = (int) std::lround (patternParam->convertFrom0to1 (patternParam->getValue()));
    const int len = (int) std::lround (lenParam->convertFrom0to1 (lenParam->getValue()));
    const bool gateOn = proc.apvts.getRawParameterValue ("gate_on")->load() > 0.5f;
    const int playing = proc.uiGateStep.load();
    const float gap = 4.0f;
    const float cw = ((float) getWidth() - 15.0f * gap) / 16.0f;

    for (int i = 0; i < 16; ++i)
    {
        auto r = juce::Rectangle<float> ((float) i * (cw + gap), 4.0f, cw, (float) getHeight() - 8.0f);
        const bool active = i < len;
        const bool on = (pattern >> i) & 1;
        auto fill = on ? colour : juce::Colour (0xff22222f);
        g.setColour (active ? fill : fill.withAlpha (0.25f));
        g.fillRoundedRectangle (r, 4.0f);
        if (i % 4 == 0)
        {
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            g.fillRect (r.getX(), r.getBottom() - 3.0f, cw, 2.0f);
        }
        if (gateOn && active && i == playing)
        {
            g.setColour (juce::Colours::white);
            g.drawRoundedRectangle (r.expanded (1.0f), 4.0f, 2.0f);
        }
    }
}

int StepGrid::stepAt (juce::Point<int> p) const
{
    const float gap = 4.0f;
    const float cw = ((float) getWidth() - 15.0f * gap) / 16.0f;
    return juce::jlimit (0, 15, (int) ((float) p.x / (cw + gap)));
}

void StepGrid::setStep (int step, bool on)
{
    int pattern = (int) std::lround (patternParam->convertFrom0to1 (patternParam->getValue()));
    pattern = on ? (pattern | (1 << step)) : (pattern & ~(1 << step));
    patternParam->setValueNotifyingHost (patternParam->convertTo0to1 ((float) pattern));
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    const int s = stepAt (e.getPosition());
    const int pattern = (int) std::lround (patternParam->convertFrom0to1 (patternParam->getValue()));
    paintValue = ! ((pattern >> s) & 1);
    lastPainted = s;
    patternParam->beginChangeGesture();
    setStep (s, paintValue);
}
void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    const int s = stepAt (e.getPosition());
    if (s != lastPainted) { lastPainted = s; setStep (s, paintValue); }
}
void StepGrid::mouseUp (const juce::MouseEvent&) { patternParam->endChangeGesture(); lastPainted = -1; }

// ================================================================================================================
InfoComp::InfoComp()
{
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (boldFont (15.0f));
    label.setColour (juce::Label::textColourId, juce::Colour (0xffe8e8ff));
    addAndMakeVisible (label);
}
void InfoComp::setText (const juce::String& t) { label.setText (t, juce::dontSendNotification); }

void PanelCanvas::paint (juce::Graphics& g)
{
    // title
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff00e5ff), 14.0f, 0.0f,
                                             juce::Colour (0xffff2bd6), 270.0f, 0.0f, false));
    g.setFont (boldFont (26.0f));
    g.drawText ("PSYGOA SYNTH", 14, 6, 280, 32, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff8a8aa8));
    g.setFont (plainFont (11.5f));
    g.drawText ("psytrance  /  goa  /  full-on   -   FM lead + acid filter + trance gate + FX", 16, 36, 420, 16,
                juce::Justification::centredLeft);

    for (const auto& p : panels)
    {
        const auto r = p.bounds.toFloat();
        g.setColour (juce::Colour (0xff10101c));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (p.colour.withAlpha (0.55f));
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.2f);
        g.setColour (p.colour);
        g.setFont (boldFont (12.5f));
        g.drawText (p.title.toUpperCase(), p.bounds.getX() + 12, p.bounds.getY() + 6, p.bounds.getWidth() - 24, 18,
                    juce::Justification::centredLeft);
    }
}

// ================================================================================================================
PsyGoaEditor::PsyGoaEditor (PsyGoaProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (canvas);
    buildUi();

    // preset browser
    const auto& presets = psy::getPresets();
    juce::String lastCat;
    for (size_t i = 0; i < presets.size(); ++i)
    {
        if (lastCat != presets[i].category)
        {
            lastCat = presets[i].category;
            presetBox.addSectionHeading (lastCat);
        }
        presetBox.addItem (presets[i].name, (int) i + 1);
    }
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    prevButton.onClick = [this]
    {
        const int n = proc.getNumPrograms();
        proc.setCurrentProgram ((proc.getCurrentProgram() + n - 1) % n);
    };
    nextButton.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    canvas.addAndMakeVisible (presetBox);
    canvas.addAndMakeVisible (prevButton);
    canvas.addAndMakeVisible (nextButton);
    prevButton.setBounds (330, 14, 30, 30);
    presetBox.setBounds (366, 14, 400, 30);
    nextButton.setBounds (772, 14, 30, 30);

    setResizable (true, true);
    setResizeLimits (660, 345, 1980, 1035);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setSize (1100, 575);
    startTimerHz (10);
}

PsyGoaEditor::~PsyGoaEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

PanelInfo& PsyGoaEditor::addPanel (const juce::String& title, juce::Colour c, juce::Rectangle<int> bounds)
{
    panels.push_back ({ title, c, bounds, {} });
    return panels.back();
}

void PsyGoaEditor::knob (PanelInfo& p, const char* id, const char* label)
{
    auto k = std::make_unique<KnobComp> (proc.apvts, id, label, p.colour);
    canvas.addAndMakeVisible (*k);
    p.items.push_back ({ k.get(), 66, 84 });
    owned.push_back (std::move (k));
}
void PsyGoaEditor::combo (PanelInfo& p, const char* id, const char* label, int width)
{
    auto c = std::make_unique<ComboComp> (proc.apvts, id, label, p.colour);
    canvas.addAndMakeVisible (*c);
    p.items.push_back ({ c.get(), width, 84 });
    owned.push_back (std::move (c));
}
void PsyGoaEditor::toggle (PanelInfo& p, const char* id, const char* label)
{
    auto t = std::make_unique<ToggleComp> (proc.apvts, id, label, p.colour);
    canvas.addAndMakeVisible (*t);
    p.items.push_back ({ t.get(), 64, 84 });
    owned.push_back (std::move (t));
}

void PsyGoaEditor::layoutPanel (PanelInfo& p)
{
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.flexWrap = juce::FlexBox::Wrap::wrap;
    fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    fb.alignContent = juce::FlexBox::AlignContent::flexStart;
    fb.alignItems = juce::FlexBox::AlignItems::flexStart;
    for (auto& it : p.items)
        fb.items.add (juce::FlexItem (*it.comp).withWidth ((float) it.w).withHeight ((float) it.h).withFlex (0.0f, 0.0f));
    fb.performLayout (p.bounds.reduced (8, 0).withTrimmedTop (28).toFloat());
}

void PsyGoaEditor::buildUi()
{
    const auto cyan = juce::Colour (0xff00e5ff), lime = juce::Colour (0xffb6ff00), magenta = juce::Colour (0xffff2bd6),
               orange = juce::Colour (0xffff9100), violet = juce::Colour (0xff9d6bff), teal = juce::Colour (0xff00ffa8),
               pink = juce::Colour (0xffff5c8a), blue = juce::Colour (0xff4d8dff), white = juce::Colour (0xffe0e0ff);
    panels.reserve (16);            // PanelInfo references must stay valid while we fill them

    // ---- row 1 --------------------------------------------------------------------------------------------
    auto& syn = addPanel ("FM Synth  |  Oriental Scales", cyan, { 12, 60, 560, 200 });
    knob (syn, "fm_ratio", "Ratio");   knob (syn, "fm_index", "Index");   knob (syn, "fm_decay", "FM Decay");
    knob (syn, "fm_fb", "Feedback");   knob (syn, "fm_level", "FM Level"); knob (syn, "saw_mix", "Saw");
    knob (syn, "saw_detune", "Detune"); knob (syn, "sub_mix", "Sub");
    knob (syn, "transpose", "Transpose");
    combo (syn, "scale", "Scale", 130);  combo (syn, "root", "Root", 70);

    auto& amp = addPanel ("Envelope / Input", lime, { 580, 60, 300, 200 });
    knob (amp, "amp_a", "Attack");  knob (amp, "amp_d", "Decay");  knob (amp, "amp_s", "Sustain");
    knob (amp, "amp_r", "Release"); knob (amp, "glide", "Glide");  knob (amp, "in_level", "FX In");
    toggle (amp, "mono", "MONO");

    auto& gat = addPanel ("Trance Gate  |  DAW synced", magenta, { 888, 60, 420, 200 });
    toggle (gat, "gate_on", "GATE");  combo (gat, "gate_div", "Rate", 80);
    knob (gat, "gate_depth", "Depth"); knob (gat, "gate_smooth", "Smooth"); knob (gat, "gate_len", "Steps");
    {
        auto grid = std::make_unique<StepGrid> (proc, magenta);
        canvas.addAndMakeVisible (*grid);
        gat.items.push_back ({ grid.get(), 400, 56 });
        owned.push_back (std::move (grid));
    }

    // ---- row 2 --------------------------------------------------------------------------------------------
    auto& flt = addPanel ("Acid Filter  |  303 style", orange, { 12, 268, 560, 200 });
    toggle (flt, "filt_on", "ACID");  combo (flt, "filt_type", "Type", 130);
    knob (flt, "cutoff", "Cutoff");   knob (flt, "reso", "Resonance"); knob (flt, "filt_env", "Env Amt");
    knob (flt, "filt_decay", "Env Decay"); knob (flt, "filt_drive", "Drive");
    knob (flt, "lfo_depth", "LFO Amt"); knob (flt, "lfo_rate", "LFO Hz");
    toggle (flt, "lfo_sync", "SYNC");  combo (flt, "lfo_div", "LFO Div", 80); combo (flt, "lfo_shape", "Shape", 90);

    auto& pha = addPanel ("Phaser", violet, { 580, 268, 360, 200 });
    toggle (pha, "ph_on", "PHASER");  knob (pha, "ph_mix", "Mix");  knob (pha, "ph_rate", "Rate");
    knob (pha, "ph_depth", "Depth");  knob (pha, "ph_fb", "Feedback");
    combo (pha, "ph_stages", "Stages", 70); knob (pha, "ph_center", "Center");
    toggle (pha, "ph_sync", "SYNC");  combo (pha, "ph_div", "Div", 80);

    auto& fla = addPanel ("Flanger", teal, { 948, 268, 360, 200 });
    toggle (fla, "fl_on", "FLANGER");  knob (fla, "fl_mix", "Mix");  knob (fla, "fl_rate", "Rate");
    knob (fla, "fl_depth", "Depth");   knob (fla, "fl_fb", "Feedback");
    knob (fla, "fl_delay", "Delay");   toggle (fla, "fl_sync", "SYNC"); combo (fla, "fl_div", "Div", 80);

    // ---- row 3 --------------------------------------------------------------------------------------------
    auto& del = addPanel ("Ping-Pong Delay  |  tempo synced", pink, { 12, 476, 560, 200 });
    toggle (del, "dl_on", "DELAY");  knob (del, "dl_mix", "Mix");
    toggle (del, "dl_sync", "SYNC"); combo (del, "dl_div", "Time", 80);  knob (del, "dl_time", "Free ms");
    knob (del, "dl_fb", "Feedback"); knob (del, "dl_cross", "Ping-Pong");
    knob (del, "dl_lowcut", "Low Cut"); knob (del, "dl_hicut", "Hi Cut");

    auto& rev = addPanel ("Reverb", blue, { 580, 476, 360, 200 });
    toggle (rev, "rv_on", "REVERB");  knob (rev, "rv_mix", "Mix");  knob (rev, "rv_size", "Size");
    knob (rev, "rv_damp", "Damping"); knob (rev, "rv_width", "Width");

    auto& mst = addPanel ("Master  |  Transport", white, { 948, 476, 360, 200 });
    knob (mst, "master_db", "Master");
    {
        auto inf = std::make_unique<InfoComp>();
        canvas.addAndMakeVisible (*inf);
        info = inf.get();
        mst.items.push_back ({ inf.get(), 230, 40 });
        owned.push_back (std::move (inf));
    }

    for (auto& p : panels) layoutPanel (p);
}

void PsyGoaEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b0b14), 0.0f, 0.0f,
                                             juce::Colour (0xff1a0b2a), 0.0f, (float) getHeight(), false));
    g.fillAll();
}

void PsyGoaEditor::resized()
{
    const float s = (float) getWidth() / (float) kW;
    canvas.setTransform (juce::AffineTransform::scale (s));
    canvas.setBounds (0, 0, kW, kH);
}

void PsyGoaEditor::timerCallback()
{
    if (info != nullptr)
        info->setText (juce::String ("BPM ") + juce::String (proc.uiBpm.load(), 1)
                       + (proc.uiHostSync.load() ? (proc.uiHostPlaying.load() ? "   PLAY  (host sync)" : "   STOP  (free run)")
                                                 : "   no host tempo"));
    const int want = proc.getCurrentProgram() + 1;
    if (presetBox.getSelectedId() != want)
        presetBox.setSelectedId (want, juce::dontSendNotification);
}
