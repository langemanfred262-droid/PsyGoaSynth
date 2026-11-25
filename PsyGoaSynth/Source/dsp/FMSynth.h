#pragma once
#include "Common.h"
#include <vector>

namespace psy
{
struct SynthParams
{
    float ratio = 2.0f, index = 3.0f, fmDecayMs = 300.0f, fmFb = 0.0f, fmLevel = 1.0f;
    float sawMix = 0.0f, sawDetune = 12.0f, subMix = 0.0f;
    float attackMs = 2.0f, decayMs = 300.0f, sustain = 0.7f, releaseMs = 120.0f;
    float glideMs = 0.0f;
    bool mono = false;
    int transpose = 0, scale = 0, root = 0;
    // derived (filled by Synth::setParams)
    float detUp = 1.0f, detDn = 1.0f;
};

class FMVoice
{
public:
    bool held = false;
    int note = -1;
    uint32_t age = 0;

    void prepare (double sr)
    {
        invSr = 1.0 / sr;
        env.setSampleRate (sr);
        reset();
    }
    void reset()
    {
        env.reset();
        phC = phM = phS = phA = phB = 0.0;
        modPrev = 0.0f; fmEnv = 0.0f; held = false; note = -1;
    }
    bool active() const { return env.isActive(); }

    void setEnv (const SynthParams& p) { env.set (p.attackMs, p.decayMs, p.sustain, p.releaseMs); }

    void noteOn (int inNote, float vel, double targetHz, double startHz, const SynthParams& p, uint32_t a)
    {
        note = inNote; age = a; velocity = vel; target = targetHz; freq = startHz; held = true;
        if (! env.isActive()) { phC = phM = phS = phA = phB = 0.0; modPrev = 0.0f; }
        fmEnv = 1.0f;
        setEnv (p);
        env.noteOn();
    }
    void setTarget (double hz) { target = hz; }
    void noteOff() { held = false; env.noteOff(); }

    float render (const SynthParams& p, double bend, float glideCoef, float fmCoef)
    {
        freq += (target - freq) * glideCoef;
        const double f  = freq * bend;
        const double dC = f * invSr;

        // --- FM pair (modulator with feedback -> carrier) ---
        phM += dC * p.ratio;  phM -= std::floor (phM);
        const float m = (float) std::sin (kTwoPi * phM + 2.0 * p.fmFb * modPrev);
        modPrev = m;
        const float idx = p.index * fmEnv * (0.5f + 0.5f * velocity);
        fmEnv *= fmCoef;
        phC += dC;  phC -= std::floor (phC);
        const float car = (float) std::sin (kTwoPi * phC + idx * m);

        float out = car * p.fmLevel;

        // --- detuned PolyBLEP saw pair ---
        if (p.sawMix > 0.0f)
        {
            const double dA = dC * p.detUp, dB = dC * p.detDn;
            phA += dA; phA -= std::floor (phA);
            phB += dB; phB -= std::floor (phB);
            const float s = 0.5f * (saw (phA, dA) + saw (phB, dB));
            out += s * p.sawMix;
        }
        // --- sub sine, one octave down ---
        if (p.subMix > 0.0f)
        {
            phS += dC * 0.5; phS -= std::floor (phS);
            out += (float) std::sin (kTwoPi * phS) * p.subMix;
        }
        return out * env.next() * (0.4f + 0.6f * velocity);
    }

private:
    static float polyBlep (double t, double dt)
    {
        if (t < dt)       { t /= dt; return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }
    static float saw (double ph, double dt) { return (float) (2.0 * ph - 1.0) - polyBlep (ph, std::min (dt, 0.49)); }

    Adsr env;
    double invSr = 1.0 / 44100.0, freq = 440.0, target = 440.0;
    double phC = 0, phM = 0, phS = 0, phA = 0, phB = 0;
    float modPrev = 0.0f, fmEnv = 0.0f, velocity = 1.0f;
};

class Synth
{
public:
    static constexpr int kVoices = 8;

    void prepare (double sr)
    {
        sampleRate = sr;
        for (auto& v : voices) v.prepare (sr);
        stack.clear();
        stack.reserve (64);
        setParams (p);
    }

    void setParams (const SynthParams& np)
    {
        p = np;
        p.detUp = (float) std::pow (2.0, p.sawDetune / 1200.0);
        p.detDn = 1.0f / p.detUp;
        glideCoef = p.glideMs > 0.5f ? msToCoef (p.glideMs / 3.0f, sampleRate) : 1.0f;
        fmCoef = decayCoef (p.fmDecayMs, sampleRate);
        for (auto& v : voices) if (v.active()) v.setEnv (p);
    }

    void setPitchBend (float semitones) { bend = std::pow (2.0, (double) semitones / 12.0); }

    void noteOn (int inNote, float vel)
    {
        const double hz = noteToHz (inNote);
        const double startHz = p.glideMs > 0.5f ? lastHz : hz;

        if (p.mono)
        {
            stack.erase (std::remove (stack.begin(), stack.end(), inNote), stack.end());
            if (stack.size() < 64) stack.push_back (inNote);
            voices[0].noteOn (inNote, vel, hz, startHz, p, ++counter);
        }
        else
        {
            FMVoice* v = nullptr;
            for (auto& x : voices) if (x.active() && x.note == inNote) { v = &x; break; }
            if (v == nullptr) for (auto& x : voices) if (! x.active()) { v = &x; break; }
            if (v == nullptr)
            {
                v = &voices[0];
                for (auto& x : voices) if (x.age < v->age) v = &x;
            }
            v->noteOn (inNote, vel, hz, startHz, p, ++counter);
        }
        lastHz = hz;
    }

    void noteOff (int inNote)
    {
        if (p.mono)
        {
            stack.erase (std::remove (stack.begin(), stack.end(), inNote), stack.end());
            if (stack.empty()) { voices[0].noteOff(); return; }
            if (voices[0].note == inNote)               // legato fall-back to previous held key
            {
                const int top = stack.back();
                voices[0].setTarget (noteToHz (top));
                voices[0].note = top;
                lastHz = noteToHz (top);
            }
            return;
        }
        for (auto& v : voices) if (v.held && v.note == inNote) v.noteOff();
    }

    void allNotesOff()
    {
        stack.clear();
        for (auto& v : voices) v.noteOff();
    }

    // adds into l / r
    void render (float* l, float* r, int n)
    {
        for (int vi = 0; vi < kVoices; ++vi)
        {
            auto& v = voices[(size_t) vi];
            if (! v.active()) continue;
            const float pos = p.mono ? 0.0f : ((float) vi - 3.5f) / 3.5f * 0.4f;
            const float ang = (pos + 1.0f) * (float) kPi * 0.25f;
            const float gl = std::cos (ang) * 0.5f, gr = std::sin (ang) * 0.5f;
            for (int i = 0; i < n; ++i)
            {
                const float s = v.render (p, bend, glideCoef, fmCoef);
                l[i] += s * gl;
                r[i] += s * gr;
            }
        }
    }

private:
    double noteToHz (int inNote) const
    {
        const int n = std::clamp (quantizeToScale (inNote + p.transpose, p.root, p.scale), 0, 127);
        return 440.0 * std::pow (2.0, (n - 69) / 12.0);
    }

    FMVoice voices[kVoices];
    SynthParams p;
    std::vector<int> stack;
    double sampleRate = 44100.0, lastHz = 440.0, bend = 1.0;
    float glideCoef = 1.0f, fmCoef = 0.999f;
    uint32_t counter = 0;
};
} // namespace psy
