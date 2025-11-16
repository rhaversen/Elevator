#pragma once

#include "CoreMinimal.h"

namespace ProceduralOffice
{
    namespace Utils
    {
        /**
         * Calculate the unit direction from Start to End. Returns ZeroVector when the span is below tolerance.
         */
        FVector2D CalculateUnitDirection(const FVector2D &Start, const FVector2D &End, float &OutLength);

        /**
         * Convert a 2D direction to a yaw angle in degrees. Returns 0 when the direction is nearly zero.
         */
        float DirectionToYawDegrees(const FVector2D &Direction);

        /**
         * Compute the wall height given the floor and ceiling heights, enforcing a minimum span to avoid zero-scaled meshes.
         */
        float ComputeWallHeight(float FloorHeight, float CeilingHeight, float Minimum = 1.0f);

        /**
         * Clamp a value to be positive, falling back to DefaultValue when the incoming value is too small.
         */
        float ClampPositive(float Value, float DefaultValue = KINDA_SMALL_NUMBER);
    }
}
