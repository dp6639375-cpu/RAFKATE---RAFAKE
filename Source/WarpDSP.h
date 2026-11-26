#pragma once
#include <cmath>
#include <algorithm>
#include <vector>

namespace warp
{
    // ------------------------------------------------------------------ 30 waveshaping algorithms
    inline const char* const shapeNames[] = {
        "Soft", "Hard", "Fold", "Crush", "Asym", "Sigmoid", "Rectify", "Cubic", "Fuzz", "Cheby",
        "Triangle", "Stairs", "Tube", "Diode", "Rational", "Arctan", "Exp", "Cheby5", "Double Fold", "Decimate",
        "Ring", "Gate", "Parabolic", "Zigzag", "Half-Wave", "Cube Gate", "Soft Fold", "Hyper", "Spike", "Glitch" };
    constexpr int numShapes = 30;

    // Input already includes drive and bias. Every shape satisfies shape(0) == 0.
    inline float shape (float x, int type) noexcept
    {
        constexpr float pi = 3.14159265f, hp = 1.5707963f, twoOverPi = 0.63661977f;
        switch (type)
        {
            case 0:  return std::tanh (x);
            case 1:  return std::clamp (x, -1.0f, 1.0f);
            case 2:  return std::sin (x * hp);
            case 3:  return std::round (std::clamp (x, -1.0f, 1.0f) * 8.0f) / 8.0f;
            case 4:  return x > 0.0f ? std::tanh (x) : 0.6f * std::tanh (x / 0.6f);
            case 5:  return x / std::sqrt (1.0f + x * x);
            case 6:  return std::tanh (1.5f * std::abs (x));
            case 7:  { const float v = std::clamp (x, -1.0f, 1.0f); return 1.5f * v - 0.5f * v * v * v; }
            case 8:  return (x < 0.0f ? -1.0f : 1.0f) * (1.0f - std::exp (-2.0f * std::abs (x)));
            case 9:  { const float v = std::clamp (x, -1.0f, 1.0f); return 4.0f * v * v * v - 3.0f * v; }
            case 10: return std::asin (std::sin (x * hp)) * twoOverPi;
            case 11: { const float v = x * 4.0f, f = std::floor (v), fr = v - f;
                       const float s = f + 0.5f * (1.0f + std::tanh (6.0f * (fr - 0.5f)) / std::tanh (3.0f));
                       return std::clamp (s * 0.25f, -1.2f, 1.2f); }
            case 12: { const float q = 0.3f * x * x; return std::tanh (x + q) - std::tanh (q); }
            case 13: return x >= 0.0f ? 1.0f - std::exp (-x) : -0.25f * (1.0f - std::exp (2.0f * x));
            case 14: return x / (1.0f + std::abs (x));
            case 15: return twoOverPi * std::atan (x * hp);
            case 16: return (x < 0.0f ? -1.0f : 1.0f) * (1.0f - std::exp (-0.7f * std::abs (x))) * 1.2f;
            case 17: { const float v = std::clamp (x, -1.0f, 1.0f), v2 = v * v; return v * (16.0f * v2 * v2 - 20.0f * v2 + 5.0f); }
            case 18: return std::sin (x * pi);
            case 19: return std::round (std::clamp (x, -1.0f, 1.0f) * 3.0f) / 3.0f;
            case 20: { const float v = std::clamp (x, -1.0f, 1.0f); return v * std::sin (v * 8.0f); }
            case 21: return std::tanh (3.0f * x) * std::tanh (8.0f * x * x);
            case 22: { const float v = std::clamp (x, -1.0f, 1.0f); return 2.0f * v - v * std::abs (v); }
            case 23: return std::asin (std::sin (x * pi)) * twoOverPi;
            case 24: return x > 0.0f ? std::tanh (2.0f * x) : 0.0f;
            case 25: return std::tanh (2.0f * x * x * x);
            case 26: return std::tanh (x - 0.5f * std::sin (2.0f * x));
            case 27: return std::tanh (std::sinh (std::clamp (x * 1.5f, -4.0f, 4.0f)));
            case 28: return 2.3f * x * std::exp (-2.0f * x * x);
            case 29: { const float v = std::clamp (x, -1.0f, 1.0f); return std::round (v * 5.0f) / 5.0f + 0.15f * std::sin (v * 25.0f); }
            default: return x;
        }
    }

    // Cosine-interpolated 8-point looping curve; phase in [0,1).
    inline float curveValue (const float* pts, float phase) noexcept
    {
        const float pos = phase * 8.0f;
        const int i = (int) pos;
        const float f = pos - (float) i;
        const float t = 0.5f - 0.5f * std::cos (3.14159265f * f);
        const float a = pts[i & 7], b = pts[(i + 1) & 7];
        return a + (b - a) * t;
    }

    inline float snapHz (float f) noexcept
    {
        const float semis = std::round (12.0f * std::log2 (f / 440.0f));
        return 440.0f * std::exp2 (semis / 12.0f);
    }

    // ------------------------------------------------------------------ 24 filter types
    inline const char* const filterNames[] = {
        "Lowpass 12", "Lowpass 24", "Highpass 12", "Highpass 24", "Bandpass", "Notch", "Allpass", "Peak",
        "Morph LP-BP-HP", "Morph LP-Notch-HP", "Tilt", "Formant",
        "Low Shelf", "High Shelf", "Dual Peak", "Wide Bell", "Double Notch", "Scoop",
        "Comb +", "Comb -", "Resonant Comb +", "Resonant Comb -", "Flange Comb", "Odd Harmonics" };
    constexpr int numFilters = 24;

    struct SVFStage   // TPT state-variable filter stage (Zavalishin)
    {
        float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, k = 1.0f;
        float s1[2] {}, s2[2] {};
        void reset() { for (int c = 0; c < 2; ++c) s1[c] = s2[c] = 0.0f; }
        void set (float fc, float Q, float fs)
        {
            fc = std::clamp (fc, 16.0f, 0.45f * fs);
            const float g = std::tan (3.14159265f * fc / fs);
            k = 1.0f / Q;
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }
        void run (int c, float x, float& lp, float& bp, float& hp)   // bp is the raw (non-normalised) band output
        {
            const float v3 = x - s2[c];
            const float v1 = a1 * s1[c] + a2 * v3;
            const float v2 = s2[c] + a2 * s1[c] + a3 * v3;
            s1[c] = 2.0f * v1 - s1[c];
            s2[c] = 2.0f * v2 - s2[c];
            lp = v2; bp = v1; hp = x - k * v1 - v2;
        }
    };

    struct FilterFx
    {
        void prepare (double sampleRate)
        {
            fs = (float) sampleRate;
            cbSize = (int) (fs / 20.0f) + 8;
            for (auto& b : cb) b.assign ((size_t) cbSize, 0.0f);
            cw[0] = cw[1] = 0;
            A.reset(); B.reset();
        }

        void set (int t, float cutoff, float res, float morph)
        {
            type = t; m = morph; gain = res * 4.0f;
            const float Q = 0.5f + res * res * 12.0f;
            switch (t)
            {
                case 1: case 3:  A.set (cutoff, Q, fs); B.set (cutoff, 0.707f, fs); break;
                case 11:         A.set (cutoff * (0.6f + 0.8f * morph), Q + 2.0f, fs);
                                 B.set (cutoff * (2.0f + 2.5f * morph), Q + 2.0f, fs); break;
                case 14:         A.set (cutoff, Q, fs); B.set (cutoff * 2.0f, Q, fs); break;
                case 15:         A.set (cutoff, 0.7f, fs); break;
                case 16:         A.set (cutoff, Q, fs); B.set (cutoff * 1.5f, Q, fs); break;
                default:         A.set (cutoff, Q, fs); break;
            }
            if (t >= 18)
            {
                delay = fs / std::clamp (cutoff, 20.0f, fs * 0.25f);
                if (t == 23) delay *= 0.5f;
                if (t == 22) delay *= (0.5f + morph);
                delay = std::clamp (delay, 2.0f, (float) cbSize - 4.0f);
                g = (t == 20 || t == 21) ? res * 0.92f : res * 0.9f;
            }
        }

        float process (int c, float x)
        {
            float lp = 0.f, bp = 0.f, hp = 0.f, lp2 = 0.f, bp2 = 0.f, hp2 = 0.f;
            const float k = A.k;

            if (type < 18)
            {
                A.run (c, x, lp, bp, hp);
                switch (type)
                {
                    case 0:  return lp;
                    case 1:  B.run (c, lp, lp2, bp2, hp2); return lp2;
                    case 2:  return hp;
                    case 3:  B.run (c, hp, lp2, bp2, hp2); return hp2;
                    case 4:  return bp * k;
                    case 5:  return x - k * bp;
                    case 6:  return x - 2.0f * k * bp;
                    case 7:  return x + gain * k * bp;
                    case 8:  { const float b = bp * k;
                               return m < 0.5f ? lp + (b - lp) * (m * 2.0f) : b + (hp - b) * ((m - 0.5f) * 2.0f); }
                    case 9:  { const float n = x - k * bp;
                               return m < 0.5f ? lp + (n - lp) * (m * 2.0f) : n + (hp - n) * ((m - 0.5f) * 2.0f); }
                    case 10: return x + (2.0f * m - 1.0f) * (lp - hp);
                    case 11: B.run (c, x, lp2, bp2, hp2); return 1.5f * (bp * k + 0.7f * bp2 * B.k);
                    case 12: return x + gain * lp;
                    case 13: return x + gain * hp;
                    case 14: B.run (c, x, lp2, bp2, hp2); return x + 0.5f * gain * (k * bp + B.k * bp2);
                    case 15: return x + gain * k * bp;
                    case 16: { const float n1 = x - k * bp; B.run (c, n1, lp2, bp2, hp2); return n1 - B.k * bp2; }
                    case 17: return x - std::min (0.95f, gain * 0.25f) * k * bp;
                    default: return x;
                }
            }

            // comb family
            auto& buf = cb[c];
            float rd = (float) cw[c] - delay;
            if (rd < 0.0f) rd += (float) cbSize;
            const int i0 = (int) rd;
            int i1 = i0 + 1; if (i1 >= cbSize) i1 = 0;
            const float del = buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * (rd - (float) i0);

            float y;
            switch (type)
            {
                case 18: y = x + g * del; buf[(size_t) cw[c]] = x; break;
                case 19: y = x - g * del; buf[(size_t) cw[c]] = x; break;
                case 20: y = x + g * del; buf[(size_t) cw[c]] = std::tanh (y); y *= (1.0f - 0.5f * g); break;
                case 21: y = x - g * del; buf[(size_t) cw[c]] = std::tanh (y); y *= (1.0f - 0.5f * g); break;
                case 22: y = (x + g * del) * 0.7f; buf[(size_t) cw[c]] = x; break;
                default: y = x - g * del; buf[(size_t) cw[c]] = x; break;
            }
            if (++cw[c] >= cbSize) cw[c] = 0;
            return y;
        }

        SVFStage A, B;
        std::vector<float> cb[2];
        int cw[2] { 0, 0 }, cbSize = 1, type = 0;
        float fs = 48000.0f, m = 0.0f, gain = 0.0f, delay = 2.0f, g = 0.0f;
    };
}
