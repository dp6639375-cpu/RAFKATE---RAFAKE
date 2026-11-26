#pragma once
#include <cmath>
#include <algorithm>

namespace warp
{
    inline const char* const shapeNames[] = { "Soft", "Hard", "Fold", "Crush", "Asym", "Sigmoid", "Rectify" };
    constexpr int numShapes = 7;

    // Transfer function: input already includes drive and bias.
    inline float shape (float x, int type) noexcept
    {
        switch (type)
        {
            case 0:  return std::tanh (x);
            case 1:  return std::clamp (x, -1.0f, 1.0f);
            case 2:  return std::sin (x * 1.5707963f);
            case 3:  return std::round (std::clamp (x, -1.0f, 1.0f) * 8.0f) / 8.0f;
            case 4:  return x > 0.0f ? std::tanh (x) : 0.6f * std::tanh (x / 0.6f);
            case 5:  return x / std::sqrt (1.0f + x * x);
            case 6:  return std::tanh (1.5f * std::abs (x)) - 0.5f;
            default: return x;
        }
    }
}
