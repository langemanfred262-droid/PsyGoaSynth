#pragma once
#include "Common.h"
#include <vector>

namespace psy
{
struct PhaserParams
{
    bool on = false;
    float mix = 0.5f, rateHz = 0.4f, depth = 0.7f, fb = 0.4f, center = 1000.0f;
    int stages = 2;               // 0:4  1:6  2:8  3:12
    bool sync = false;
    int div = 1;
};

class Phaser
{
public:
    void prepare (double sr) { sampleRate = sr; reset(); }
    void reset() { for (auto& c : z) for (float& v : c) v = 0.0f; last[0] = last[1] = 0.0f; phase = 0.0; }
    void setParams (const PhaserParams& np) { p = np; }

    void process (float* l, float* r, int n, double beat0, double beatInc)
    {
        if (! p.on) return;
        static const int stageTable[4] = { 4, 6, 8, 12 };
        const int ns = stageTable[std::clamp (p.stages, 0, 3)];
        const double nyq = 0.45 * sampleRate;
        float* ch[2] = { l, r };

        for (int i = 0; i < n; ++i)
        {
            double ph;
            if (p.sync) ph = frac ((beat0 + i * beatInc) / divBeats (p.div));
            else { phase += (double) p.rateHz / sampleRate; phase -= std::floor (phase); ph = phase; }

            for (int c = 0; c < 2; ++c)
            {
                const double lfo = std::sin (kTwoPi * (ph + 0.25 * c));          // R channel 90 deg apart
                const double fc = std::clamp ((double) p.center * std::exp2 (lfo * p.depth * 2.0), 20.0, nyq);
                const float t = (float) std::tan (kPi * fc / sampleRate);
                const float a = (t - 1.0f) / (t + 1.0f);

                const float in = ch[c][i];
                float x = std::tanh (in + p.fb * last[c]);
                for (int k = 0; k < ns; ++k)
                {
                    const float y = a * x + z[c][k];
                    z[c][k] = flushDenormal (x - a * y);
                    x = y;
                }
                last[c] = flushDenormal (x);
                ch[c][i] = sanitize ((in + p.mix * x) / (1.0f + 0.5f * p.mix));
            }
        }
    }

private:
    PhaserParams p;
    float z[2][12] = {};
    float last[2] = { 0, 0 };
    double sampleRate = 44100.0, phase = 0.0;
};

// ----------------------------------------------------------------------------------------------------------
struct FlangerParams
{
    bool on = false;
    float mix = 0.5f, rateHz = 0.3f, depth = 0.6f, fb = 0.5f, delayMs = 3.0f;
    bool sync = false;
    int div = 1;
};

class Flanger
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        size = (int) (0.04 * sr) + 8;
        for (auto& b : buf) b.assign ((size_t) size, 0.0f);
        wp = 0; phase = 0.0;
    }
    void reset() { for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f); }
    void setParams (const FlangerParams& np)
    {
        if (p.on && ! np.on) reset();
        p = np;
    }

    void process (float* l, float* r, int n, double beat0, double beatInc)
    {
        if (! p.on) return;
        float* ch[2] = { l, r };
        const float fb = std::clamp (p.fb, -0.95f, 0.95f);

        for (int i = 0; i < n; ++i)
        {
            double ph;
            if (p.sync) ph = frac ((beat0 + i * beatInc) / divBeats (p.div));
            else { phase += (double) p.rateHz / sampleRate; phase -= std::floor (phase); ph = phase; }

            for (int c = 0; c < 2; ++c)
            {
                const double lfo = std::sin (kTwoPi * (ph + 0.25 * c));
                const double dMs = (double) p.delayMs * (1.0 + 0.9 * p.depth * lfo);
                const double dS = std::clamp (dMs * 0.001 * sampleRate, 1.0, (double) (size - 4));
                double rp = (double) wp - dS;
                if (rp < 0.0) rp += size;
                const int i0 = (int) rp;
                const int i1 = (i0 + 1) % size;
                const float fr = (float) (rp - i0);
                const float d = buf[c][(size_t) i0] * (1.0f - fr) + buf[c][(size_t) i1] * fr;

                const float in = ch[c][i];
                buf[c][(size_t) wp] = flushDenormal (std::tanh (in + fb * d));
                ch[c][i] = sanitize ((in + p.mix * d) / (1.0f + 0.5f * p.mix));
            }
            if (++wp >= size) wp = 0;
        }
    }

private:
    FlangerParams p;
    std::vector<float> buf[2];
    int size = 2048, wp = 0;
    double sampleRate = 44100.0, phase = 0.0;
};
} // namespace psy
