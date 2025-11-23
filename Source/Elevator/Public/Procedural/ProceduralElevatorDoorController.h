#pragma once

#include "CoreMinimal.h"
#include "Containers/StaticArray.h"

class AProceduralElevator;
enum class EProceduralElevatorDoorSlot : uint8;

/** Function pointer type for door interpolation. Takes alpha [0,1] and returns interpolated value [0,1]. */
using FDoorInterpolationFunction = float(*)(float);

struct FProceduralElevatorDoorControllerParameters
{
    float SlideSpeed = 1.5f;
    FDoorInterpolationFunction InterpolationFunction = nullptr;
};

class FProceduralElevatorDoorController
{
public:
    static constexpr int32 DoorSlotCount = 4;

    void Initialize(AProceduralElevator *InOwner);
    void SetParameters(const FProceduralElevatorDoorControllerParameters &InParameters);
    void Tick(float DeltaSeconds);

    void OpenDoors();
    void CloseDoors();

    void SetDoorFractionImmediate(EProceduralElevatorDoorSlot Slot, float Fraction);

    float GetDoorFraction(EProceduralElevatorDoorSlot Slot) const;

    /** Check if any door is currently animating */
    bool IsAnimating() const;

    /** Check if all doors are fully open (fraction >= 0.99) */
    bool AreDoorsFullyOpen() const;

    /** Check if all doors are fully closed (fraction <= 0.01) */
    bool AreDoorsFullyClosed() const;

    /** Maximum remaining time (seconds) until all doors reach their targets. */
    float GetMaxRemainingDuration() const;

private:
    struct FDoorMotion
    {
        float CurrentFraction = 0.0f;
        float StartFraction = 0.0f;
        float TargetFraction = 0.0f;
        float ProgressAlpha = 1.0f;
        float Duration = 0.0f;
    };

    void SetTargetFraction(float Target);
    float ApplyInterpolation(float Alpha) const;

    AProceduralElevator *Owner = nullptr;
    TStaticArray<FDoorMotion, DoorSlotCount> DoorMotions;
    FProceduralElevatorDoorControllerParameters Parameters;
};
