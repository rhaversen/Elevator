#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorkstationInteractionComponent.generated.h"

class AActor;
class AFirstPersonCharacter;
class APlayerController;

/**
 * Handles workstation camera transitions, cursor toggling, and input locking for the owning character.
 */
UCLASS(ClassGroup = (Interaction), Blueprintable, meta = (BlueprintSpawnableComponent))
class ELEVATOR_API UWorkstationInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWorkstationInteractionComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void BeginInteraction(const FTransform& TargetTransform, float TravelTime, float ArcHeight, float CurveBias, AActor* WorkstationActor);
    void CancelInteraction();

    bool IsTransitionActive() const { return bIsTransitionActive; }
    bool IsInteractionLocked() const { return bIsInteractionLocked; }
    bool IsExiting() const { return bIsExiting; }

    AActor* GetCurrentWorkstationActor() const { return WorkstationActor.Get(); }

private:
    void EnsureOwnerReferences();
    void StartEntry(const FTransform& TargetTransform, float TravelTime, float ArcHeight, float CurveBias, AActor* TargetActor);
    void StartExit();
    void ConfigureControlPoints(const FVector& Direction, float Distance);
    FVector EvaluateBezier(float T) const;
    void FinalizeEntry();
    void FinalizeExit();
    void ToggleCursor(bool bEnable);
    void UpdateCachedController();

    TWeakObjectPtr<AFirstPersonCharacter> CachedCharacter;
    TWeakObjectPtr<APlayerController> CachedController;

    FVector EntryOriginLocation = FVector::ZeroVector;
    FQuat EntryOriginQuat = FQuat::Identity;

    FVector StartLocation = FVector::ZeroVector;
    FVector TargetLocation = FVector::ZeroVector;
    FVector ControlPointA = FVector::ZeroVector;
    FVector ControlPointB = FVector::ZeroVector;
    FQuat StartQuat = FQuat::Identity;
    FQuat TargetQuat = FQuat::Identity;

    float TravelDuration = 0.0f;
    float ElapsedTime = 0.0f;
    float ArcHeight = 0.0f;
    float CurveBias = 0.0f;

    bool bIsTransitionActive = false;
    bool bIsInteractionLocked = false;
    bool bIsExiting = false;

    TWeakObjectPtr<AActor> WorkstationActor;
};
