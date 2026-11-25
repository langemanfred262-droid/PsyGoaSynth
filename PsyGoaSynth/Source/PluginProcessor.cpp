#include "PluginProcessor.h"
#include "PluginEditor.h"

PsyGoaProcessor::PsyGoaProcessor()
    : AudioProcessor (BusesProperties()
                          // Optional audio input: lets you run pads / vocals through gate + filter + FX
                          .withInput ("FX Input", juce::AudioChannelSet::stereo(), false)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", psy::createParameterLayout())
{
    applyPreset (0);
}

bool PsyGoaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    const auto in = layouts.getMainInputChannelSet();
    return in.isDisabled() || in == juce::AudioChannelSet::stereo();
}

void PsyGoaProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    synth.prepare (sr);
    filter.prepare (sr);
    gate.prepare (sr);
    phaser.prepare (sr);
    flanger.prepare (sr);
    delay.prepare (sr);
    reverb.setSampleRate (sr);
    reverb.reset();
    wetBuffer.setSize (2, juce::jmax (samplesPerBlock, 8192), false, false, true);
    updateParams();
}

const juce::String PsyGoaProcessor::getProgramName (int index)
{
    const auto& p = psy::getPresets();
    return juce::isPositiveAndBelow (index, (int) p.size()) ? juce::String (p[(size_t) index].name) : juce::String();
}

void PsyGoaProcessor::applyPreset (int index)
{
    const auto& presets = psy::getPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentPreset = index;
    for (const auto& e : presets[(size_t) index].entries)
        if (auto* prm = apvts.getParameter (e.id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (e.value));
}

void PsyGoaProcessor::updateParams()
{
    psy::SynthParams sp;
    sp.ratio = pv ("fm_ratio");      sp.index = pv ("fm_index");     sp.fmDecayMs = pv ("fm_decay");
    sp.fmFb = pv ("fm_fb");          sp.fmLevel = pv ("fm_level");   sp.sawMix = pv ("saw_mix");
    sp.sawDetune = pv ("saw_detune"); sp.subMix = pv ("sub_mix");
    sp.attackMs = pv ("amp_a");      sp.decayMs = pv ("amp_d");      sp.sustain = pv ("amp_s");
    sp.releaseMs = pv ("amp_r");     sp.glideMs = pv ("glide");      sp.mono = pv ("mono") > 0.5f;
    sp.transpose = (int) std::lround (pv ("transpose"));
    sp.scale = (int) std::lround (pv ("scale"));
    sp.root = (int) std::lround (pv ("root"));
    synth.setParams (sp);

    psy::AcidParams ap;
    ap.on = pv ("filt_on") > 0.5f;   ap.type = (int) std::lround (pv ("filt_type"));
    ap.cutoff = pv ("cutoff");       ap.reso = pv ("reso");          ap.envAmt = pv ("filt_env");
    ap.decayMs = pv ("filt_decay");  ap.drive = pv ("filt_drive");   ap.lfoDepth = pv ("lfo_depth");
    ap.lfoRate = pv ("lfo_rate");    ap.lfoSync = pv ("lfo_sync") > 0.5f;
    ap.lfoDiv = (int) std::lround (pv ("lfo_div"));
    ap.lfoShape = (int) std::lround (pv ("lfo_shape"));
    filter.setParams (ap);

    psy::GateParams gp;
    gp.on = pv ("gate_on") > 0.5f;   gp.div = (int) std::lround (pv ("gate_div"));
    gp.pattern = (int) std::lround (pv ("gate_pattern"));
    gp.len = (int) std::lround (pv ("gate_len"));
    gp.depth = pv ("gate_depth");    gp.smoothMs = pv ("gate_smooth");
    gate.setParams (gp);

    psy::PhaserParams pp;
    pp.on = pv ("ph_on") > 0.5f;     pp.mix = pv ("ph_mix");         pp.rateHz = pv ("ph_rate");
    pp.sync = pv ("ph_sync") > 0.5f; pp.div = (int) std::lround (pv ("ph_div"));
    pp.depth = pv ("ph_depth");      pp.fb = pv ("ph_fb");           pp.stages = (int) std::lround (pv ("ph_stages"));
    pp.center = pv ("ph_center");
    phaser.setParams (pp);

    psy::FlangerParams fp;
    fp.on = pv ("fl_on") > 0.5f;     fp.mix = pv ("fl_mix");         fp.rateHz = pv ("fl_rate");
    fp.sync = pv ("fl_sync") > 0.5f; fp.div = (int) std::lround (pv ("fl_div"));
    fp.depth = pv ("fl_depth");      fp.fb = pv ("fl_fb");           fp.delayMs = pv ("fl_delay");
    flanger.setParams (fp);

    psy::DelayParams dp;
    dp.on = pv ("dl_on") > 0.5f;     dp.mix = pv ("dl_mix");         dp.sync = pv ("dl_sync") > 0.5f;
    dp.div = (int) std::lround (pv ("dl_div"));
    dp.timeMs = pv ("dl_time");      dp.fb = pv ("dl_fb");           dp.cross = pv ("dl_cross");
    dp.lowCut = pv ("dl_lowcut");    dp.hiCut = pv ("dl_hicut");
    delay.setParams (dp);
}

void PsyGoaProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        synth.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        filter.triggerEnv (m.getFloatVelocity());      // note-on re-triggers the acid envelope
    }
    else if (m.isNoteOff())                                synth.noteOff (m.getNoteNumber());
    else if (m.isAllNotesOff() || m.isAllSoundOff())       synth.allNotesOff();
    else if (m.isPitchWheel())
        synth.setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f);
}

void PsyGoaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    if (numCh < 2 || numSamples == 0) return;

    // optional audio input (sidechain-style FX use); otherwise start from silence
    auto* inBus = getBus (true, 0);
    if (inBus != nullptr && inBus->isEnabled()) buffer.applyGain (pv ("in_level"));
    else buffer.clear();
    for (int ch = 2; ch < numCh; ++ch) buffer.clear (ch, 0, numSamples);

    updateParams();

    // ---- DAW sync: tempo + song position --------------------------------------------------------------
    double bpm = 145.0, hostPpq = 0.0;
    bool playing = false, havePpq = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())         bpm = *b;
            if (auto q = pos->getPpqPosition()) { hostPpq = *q; havePpq = true; }
            playing = pos->getIsPlaying();
        }
    if (bpm < 20.0 || bpm > 400.0) bpm = 145.0;

    const double beatsPerSample = bpm / 60.0 / sampleRate;
    // host playing -> lock to the song position; stopped -> free-running clock so effects keep moving
    const double beat0 = (playing && havePpq) ? hostPpq : freeBeat;
    freeBeat += numSamples * beatsPerSample;
    if (freeBeat > 1.0e6) freeBeat = std::fmod (freeBeat, 64.0);

    uiBpm = bpm;
    uiHostPlaying = playing;
    uiHostSync = havePpq;
    {
        const int len = juce::jlimit (1, 16, (int) std::lround (pv ("gate_len")));
        const double sb = psy::gateBeats ((int) std::lround (pv ("gate_div")));
        const long long idx = (long long) std::floor (beat0 / sb);
        uiGateStep = (int) (((idx % len) + len) % len);
    }

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    // ---- sample-accurate MIDI + per-segment chain ---------------------------------------------------------
    int pos = 0;
    auto renderTo = [&] (int end)
    {
        const int n = end - pos;
        if (n <= 0) return;
        const double b = beat0 + pos * beatsPerSample;
        synth.render (L + pos, R + pos, n);
        filter.process (L + pos, R + pos, n, b, beatsPerSample);
        gate.process (L + pos, R + pos, n, b, beatsPerSample);
        phaser.process (L + pos, R + pos, n, b, beatsPerSample);
        flanger.process (L + pos, R + pos, n, b, beatsPerSample);
        delay.process (L + pos, R + pos, n, bpm);
        pos = end;
    };
    for (const auto metadata : midi)
    {
        renderTo (juce::jlimit (pos, numSamples, metadata.samplePosition));
        handleMidi (metadata.getMessage());
    }
    renderTo (numSamples);

    // ---- reverb (whole block) ---------------------------------------------------------------------------
    const bool rvOn = pv ("rv_on") > 0.5f;
    if (rvOn && numSamples <= wetBuffer.getNumSamples())
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = pv ("rv_size");
        rp.damping = pv ("rv_damp");
        rp.width = pv ("rv_width");
        rp.wetLevel = 0.33f;
        rp.dryLevel = 0.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);

        wetBuffer.copyFrom (0, 0, L, numSamples);
        wetBuffer.copyFrom (1, 0, R, numSamples);
        reverb.processStereo (wetBuffer.getWritePointer (0), wetBuffer.getWritePointer (1), numSamples);

        const float mix = pv ("rv_mix");
        const float* wl = wetBuffer.getReadPointer (0);
        const float* wr = wetBuffer.getReadPointer (1);
        for (int i = 0; i < numSamples; ++i)
        {
            L[i] = L[i] * (1.0f - 0.4f * mix) + wl[i] * mix;
            R[i] = R[i] * (1.0f - 0.4f * mix) + wr[i] * mix;
        }
    }
    else if (reverbWasOn && ! rvOn)
        reverb.reset();
    reverbWasOn = rvOn;

    // ---- master: gain + soft clip ---------------------------------------------------------------------------
    const float g = juce::Decibels::decibelsToGain (pv ("master_db"));
    for (int i = 0; i < numSamples; ++i)
    {
        L[i] = std::tanh (psy::sanitize (L[i]) * g);
        R[i] = std::tanh (psy::sanitize (R[i]) * g);
    }
}

juce::AudioProcessorEditor* PsyGoaProcessor::createEditor() { return new PsyGoaEditor (*this); }

void PsyGoaProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void PsyGoaProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPreset = juce::jlimit (0, getNumPrograms() - 1, (int) vt.getProperty ("preset", 0));
            apvts.replaceState (vt);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PsyGoaProcessor(); }
