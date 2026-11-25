#pragma once
#include "Common.h"

namespace psy
{
struct AcidParams
{
    bool on = true;
    int type = 0;                 // 0 Ladder LP24 (303), 1 SVF LP12, 2 SVF BP, 3 SVF HP
    float cutoff = 1200.0f, reso = 0.4f, envAmt = 0.5f, decayMs = 250.0f, drive = 0.2f;
    float lfoDepth = 0.0f;
    int lfoShape = 0;
    bool lfoSync = true;
    int lfoDiv = 6;               // 1/8
    float lfoRate = 1.0f;         // Hz when not synced
};

class AcidFilter
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        smoothCoef = msToCoef (6.0f, sr);
        reset();
    }
    void reset()
    {
        for (int c = 0; c < 2; ++c) { lad[c] = {}; svf[c] = {}; }
        env = 0.0f; lfoPhase = 0.0;
        cutSm = p.cutoff; resoSm = p.reso;
    }
    void setParams (const AcidParams& np)
    {
        p = np;
        envCoef = decayCoef (p.decayMs, sampleRate);
    }
    void triggerEnv (float velocity) { env = 0.55f + 0.45f * velocity; }   // velocity acts as 303 "accent"

    void process (float* l, float* r, int n, double beat0, double beatInc)
    {
        if (! p.on) return;
        const double nyq = 0.45 * sampleRate;
        const float dg = 1.0f + p.drive * 6.0f;
        const float norm = 1.0f / std::sqrt (dg);

        for (int i = 0; i < n; ++i)
        {
            cutSm  += smoothCoef * (p.cutoff - cutSm);
            resoSm += smoothCoef * (p.reso - resoSm);

            double ph;
            if (p.lfoSync) ph = frac ((beat0 + i * beatInc) / divBeats (p.lfoDiv));
            else { lfoPhase += (double) p.lfoRate / sampleRate; lfoPhase -= std::floor (lfoPhase); ph = lfoPhase; }
            const float lfo = lfoShape (p.lfoShape, ph);

            const float oct = p.envAmt * env * 5.0f + p.lfoDepth * lfo * 3.0f;
            env = flushDenormal (env * envCoef);
            const double fc = std::clamp ((double) cutSm * std::exp2 ((double) oct), 20.0, std::min (18000.0, nyq));

            if (p.type == 0)
            {
                const float g = (float) (1.0 - std::exp (-kTwoPi * fc / (2.0 * sampleRate)));   // 2x oversampled loop
                const float k = resoSm * 3.95f;
                const float makeup = (1.0f + 0.3f * k) * norm;
                l[i] = sanitize (ladder (lad[0], l[i] * dg, g, k) * makeup);
                r[i] = sanitize (ladder (lad[1], r[i] * dg, g, k) * makeup);
            }
            else
            {
                const float g = (float) std::tan (kPi * fc / sampleRate);
                const float R = 1.0f - 0.97f * resoSm;            // damping, smaller = more resonance
                const float h = 1.0f / (1.0f + 2.0f * R * g + g * g);
                l[i] = sanitize (svfTick (svf[0], std::tanh (l[i] * dg) * norm, g, R, h));
                r[i] = sanitize (svfTick (svf[1], std::tanh (r[i] * dg) * norm, g, R, h));
            }
        }
    }

private:
    struct Ladder { float y1 = 0, y2 = 0, y3 = 0, y4 = 0; };
    struct Svf { float s1 = 0, s2 = 0; };

    static float ladder (Ladder& s, float x, float g, float k)
    {
        for (int os = 0; os < 2; ++os)
        {
            const float u = std::tanh (x - k * s.y4);
            s.y1 += g * (u - s.y1);
            s.y2 += g * (s.y1 - s.y2);
            s.y3 += g * (s.y2 - s.y3);
            s.y4 += g * (s.y3 - s.y4);
        }
        s.y1 = flushDenormal (s.y1); s.y2 = flushDenormal (s.y2);
        s.y3 = flushDenormal (s.y3); s.y4 = flushDenormal (s.y4);
        return s.y4;
    }

    float svfTick (Svf& s, float x, float g, float R, float h) const
    {
        const float hp = (x - (2.0f * R + g) * s.s1 - s.s2) * h;
        const float bp = g * hp + s.s1;
        s.s1 = flushDenormal (g * hp + bp);
        const float lp = g * bp + s.s2;
        s.s2 = flushDenormal (g * bp + lp);
        switch (p.type)
        {
            case 2:  return bp * 2.0f * R;
            case 3:  return hp;
            default: return lp;
        }
    }

    AcidParams p;
    Ladder lad[2];
    Svf svf[2];
    double sampleRate = 44100.0, lfoPhase = 0.0;
    float env = 0.0f, envCoef = 0.999f, smoothCoef = 0.01f, cutSm = 1200.0f, resoSm = 0.4f;
};
} // namespace psy
