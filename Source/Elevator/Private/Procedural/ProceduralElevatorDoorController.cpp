#include "Procedural/ProceduralElevatorDoorController.h"
#include "Procedural/ProceduralElevator.h"
#include "Procedural/DoorInterpolationFunctions.h"
#include "Math/UnrealMathUtility.h"

static_assert(FProceduralElevatorDoorController::DoorSlotCount == AProceduralElevator::DoorSlotCount, "Door slot definitions must stay in sync.");

void FProceduralElevatorDoorController::Initialize(AProceduralElevator *InOwner)
{
    Owner = InOwner;
    for (FDoorMotion &Motion : DoorMotions)
    {
        Motion.CurrentFraction = 0.0f;
        Motion.StartFraction = 0.0f;
        Motion.TargetFraction = 0.0f;
        Motion.ProgressAlpha = 1.0f;
        Motion.Duration = 0.0f;
    }
}

void FProceduralElevatorDoorController::SetParameters(const FProceduralElevatorDoorControllerParameters &InParameters)
{
    Parameters = InParameters;
    Parameters.SlideSpeed = FMath::Max(Parameters.SlideSpeed, 0.01f);
}

void FProceduralElevatorDoorController::Tick(float DeltaSeconds)
{
    if (!Owner || DeltaSeconds <= 0.0f)
    {
        return;
    }

    for (int32 Index = 0; Index < DoorSlotCount; ++Index)
    {
        FDoorMotion &Motion = DoorMotions[Index];
        
        if (Motion.Duration <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        Motion.ProgressAlpha = FMath::Clamp(Motion.ProgressAlpha + DeltaSeconds / Motion.Duration, 0.0f, 1.0f);
        
        const float InterpolatedAlpha = ApplyInterpolation(Motion.ProgressAlpha);
        Motion.CurrentFraction = FMath::Lerp(Motion.StartFraction, Motion.TargetFraction, InterpolatedAlpha);

        if (Motion.ProgressAlpha >= 1.0f)
        {
            Motion.Duration = 0.0f;
        }

        Owner->ApplyDoorOffset(static_cast<EProceduralElevatorDoorSlot>(Index));
    }
}

void FProceduralElevatorDoorController::OpenDoors()
{
    SetTargetFraction(1.0f);
}

void FProceduralElevatorDoorController::CloseDoors()
{
    SetTargetFraction(0.0f);
}

void FProceduralElevatorDoorController::SetDoorFractionImmediate(EProceduralElevatorDoorSlot Slot, float Fraction)
{
    const int32 Index = static_cast<int32>(Slot);
    if (Index < 0 || Index >= DoorSlotCount)
    {
        return;
    }

    const float Clamped = FMath::Clamp(Fraction, 0.0f, 1.0f);
    FDoorMotion &Motion = DoorMotions[Index];
    Motion.CurrentFraction = Clamped;
    Motion.StartFraction = Clamped;
    Motion.TargetFraction = Clamped;
    Motion.ProgressAlpha = 1.0f;
    Motion.Duration = 0.0f;

    if (Owner)
    {
        Owner->ApplyDoorOffset(Slot);
    }
}

float FProceduralElevatorDoorController::GetDoorFraction(EProceduralElevatorDoorSlot Slot) const
{
    const int32 Index = static_cast<int32>(Slot);
    if (Index < 0 || Index >= DoorSlotCount)
    {
        return 0.0f;
    }
    return DoorMotions[Index].CurrentFraction;
}

void FProceduralElevatorDoorController::SetTargetFraction(float Target)
{
    const float ClampedTarget = FMath::Clamp(Target, 0.0f, 1.0f);
    
    for (int32 Index = 0; Index < DoorSlotCount; ++Index)
    {
        FDoorMotion &Motion = DoorMotions[Index];
        Motion.StartFraction = Motion.CurrentFraction;
        Motion.TargetFraction = ClampedTarget;
        Motion.Duration = FMath::Abs(ClampedTarget - Motion.StartFraction) / Parameters.SlideSpeed;
        Motion.ProgressAlpha = 0.0f;

        if (Owner)
        {
            Owner->ApplyDoorOffset(static_cast<EProceduralElevatorDoorSlot>(Index));
        }
    }
}

float FProceduralElevatorDoorController::ApplyInterpolation(float Alpha) const
{
    if (Parameters.InterpolationFunction)
    {
        return Parameters.InterpolationFunction(Alpha);
    }
    return DoorInterpolation::Linear(Alpha);
}
