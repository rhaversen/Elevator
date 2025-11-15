#include "ProceduralOfficeGenerator.Helpers.h"

namespace ProceduralOffice
{
    namespace Utils
    {
        FVector2D CalculateUnitDirection(const FVector2D &Start, const FVector2D &End, float &OutLength)
        {
            const FVector2D Delta = End - Start;
            OutLength = Delta.Size();
            return (OutLength > KINDA_SMALL_NUMBER) ? Delta / OutLength : FVector2D::ZeroVector;
        }

        float DirectionToYawDegrees(const FVector2D &Direction)
        {
            if (Direction.IsNearlyZero())
            {
                return 0.0f;
            }

            return FMath::Atan2(Direction.Y, Direction.X) * (180.0f / PI);
        }

        float ComputeWallHeight(float FloorHeight, float CeilingHeight, float Minimum)
        {
            return FMath::Max(CeilingHeight - FloorHeight, Minimum);
        }

        float ClampPositive(float Value, float DefaultValue)
        {
            return (Value > KINDA_SMALL_NUMBER) ? Value : DefaultValue;
        }
    }
}
