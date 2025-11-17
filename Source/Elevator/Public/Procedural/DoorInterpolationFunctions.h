#pragma once

#include "CoreMinimal.h"
#include "Math/UnrealMathUtility.h"

/**
 * Collection of interpolation functions for door motion.
 * All functions take an alpha in [0, 1] and return an interpolated value in [0, 1].
 */
namespace DoorInterpolation
{
    /** Linear interpolation - no easing */
    inline float Linear(float Alpha)
    {
        return FMath::Clamp(Alpha, 0.0f, 1.0f);
    }

    /** Smooth ease in and out using cubic curve */
    inline float EaseInOut(float Alpha)
    {
        return FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 3.0f);
    }

    // Add more interpolation functions here as needed:
    // inline float glitchy(float Alpha) { ... }
    // inline float stuck(float Alpha) { ... }
    // etc.
}
