#include "Interaction/WorkstationInteractionComponent.h"

#include "FirstPersonCharacter.h"
#include "Interactable.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"

UWorkstationInteractionComponent::UWorkstationInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UWorkstationInteractionComponent::BeginPlay()
{
    Super::BeginPlay();
    EnsureOwnerReferences();
    SetComponentTickEnabled(false);
}

void UWorkstationInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsTransitionActive || !CachedCharacter.IsValid())
    {
        return;
    }

    ElapsedTime += DeltaTime;
    const float DurationSafe = FMath::Max(TravelDuration, KINDA_SMALL_NUMBER);
    const float Alpha = FMath::Clamp(ElapsedTime / DurationSafe, 0.0f, 1.0f);
    const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

    const FVector NewLocation = EvaluateBezier(SmoothAlpha);
    CachedCharacter->SetActorLocation(NewLocation, false);

    UpdateCachedController();
    const FQuat NewQuat = FQuat::Slerp(StartQuat, TargetQuat, SmoothAlpha).GetNormalized();
    const FRotator NewRotator = NewQuat.Rotator();

    if (CachedController.IsValid())
    {
        CachedController->SetControlRotation(NewRotator);
    }
    CachedCharacter->SetActorRotation(FRotator(0.0f, NewRotator.Yaw, 0.0f));

    if (Alpha >= 1.0f)
    {
        CachedCharacter->SetActorLocation(TargetLocation, false);
        const FRotator TargetRotator = TargetQuat.Rotator();
        if (CachedController.IsValid())
        {
            CachedController->SetControlRotation(TargetRotator);
        }
        CachedCharacter->SetActorRotation(FRotator(0.0f, TargetRotator.Yaw, 0.0f));

        if (bIsExiting)
        {
            FinalizeExit();
        }
        else
        {
            FinalizeEntry();
        }
    }
}

void UWorkstationInteractionComponent::BeginInteraction(const FTransform& TargetTransform, float TravelTime, float ArcHeightValue, float CurveBiasValue, AActor* TargetActor)
{
    EnsureOwnerReferences();
    if (!CachedCharacter.IsValid())
    {
        return;
    }

    if (bIsTransitionActive || bIsInteractionLocked)
    {
        return;
    }

    StartEntry(TargetTransform, TravelTime, ArcHeightValue, CurveBiasValue, TargetActor);
}

void UWorkstationInteractionComponent::CancelInteraction()
{
    EnsureOwnerReferences();
    if (!CachedCharacter.IsValid())
    {
        return;
    }

    if (!bIsTransitionActive && !bIsInteractionLocked)
    {
        return;
    }

    if (bIsTransitionActive && !bIsExiting)
    {
        return;
    }

    ToggleCursor(false);

    if (AActor* Actor = WorkstationActor.Get())
    {
        if (Actor->Implements<UInteractable>())
        {
            IInteractable::Execute_OnInteractionCanceled(Actor, CachedCharacter.Get());
        }
    }

    StartExit();
}

void UWorkstationInteractionComponent::EnsureOwnerReferences()
{
    if (!CachedCharacter.IsValid())
    {
        CachedCharacter = Cast<AFirstPersonCharacter>(GetOwner());
        if (!CachedCharacter.IsValid())
        {
            return;
        }
    }

    UpdateCachedController();
}

void UWorkstationInteractionComponent::UpdateCachedController()
{
    if (!CachedCharacter.IsValid())
    {
        CachedController.Reset();
        return;
    }

    if (!CachedController.IsValid())
    {
        CachedController = Cast<APlayerController>(CachedCharacter->GetController());
    }
}

void UWorkstationInteractionComponent::StartEntry(const FTransform& TargetTransform, float TravelTime, float ArcHeightValue, float CurveBiasValue, AActor* TargetActor)
{
    EnsureOwnerReferences();
    if (!CachedCharacter.IsValid())
    {
        return;
    }

    WorkstationActor = TargetActor;

    StartLocation = CachedCharacter->GetActorLocation();
    EntryOriginLocation = StartLocation;

    UpdateCachedController();
    const FRotator StartRotator = CachedController.IsValid() ? CachedController->GetControlRotation() : CachedCharacter->GetActorRotation();
    StartQuat = StartRotator.Quaternion();
    EntryOriginQuat = StartQuat;

    FQuat RawTargetQuat = TargetTransform.GetRotation();
    TargetQuat = RawTargetQuat.IsNormalized() ? RawTargetQuat : RawTargetQuat.GetNormalized();

    TargetLocation = TargetTransform.GetLocation();
    if (UCameraComponent* Camera = CachedCharacter->GetFirstPersonCameraComponent())
    {
        TargetLocation -= Camera->GetRelativeLocation();
    }

    FVector Direction = TargetLocation - StartLocation;
    const float Distance = Direction.Size();
    if (Distance > KINDA_SMALL_NUMBER)
    {
        Direction /= Distance;
    }
    else if (UCameraComponent* Camera = CachedCharacter->GetFirstPersonCameraComponent())
    {
        Direction = Camera->GetForwardVector();
    }
    else
    {
        Direction = CachedCharacter->GetActorForwardVector();
    }

    TravelDuration = FMath::Max(TravelTime, 0.01f);
    ElapsedTime = 0.0f;
    ArcHeight = ArcHeightValue;
    CurveBias = CurveBiasValue;

    ConfigureControlPoints(Direction, Distance);

    bIsTransitionActive = true;
    bIsInteractionLocked = false;
    bIsExiting = false;

    CachedCharacter->LockMovementInput(true);
    CachedCharacter->LockLookInput(true);
    CachedCharacter->SetActorRotation(FRotator(0.0f, StartRotator.Yaw, 0.0f));

    ToggleCursor(false);

    SetComponentTickEnabled(true);
}

void UWorkstationInteractionComponent::StartExit()
{
    EnsureOwnerReferences();
    if (!CachedCharacter.IsValid())
    {
        return;
    }

    UpdateCachedController();

    const FVector CurrentLocation = CachedCharacter->GetActorLocation();
    const FRotator CurrentRotator = CachedController.IsValid() ? CachedController->GetControlRotation() : CachedCharacter->GetActorRotation();

    StartLocation = CurrentLocation;
    TargetLocation = EntryOriginLocation;

    StartQuat = CurrentRotator.Quaternion();
    TargetQuat = EntryOriginQuat;

    FVector Direction = TargetLocation - StartLocation;
    const float Distance = Direction.Size();
    if (Distance > KINDA_SMALL_NUMBER)
    {
        Direction /= Distance;
    }
    else if (UCameraComponent* Camera = CachedCharacter->GetFirstPersonCameraComponent())
    {
        Direction = Camera->GetForwardVector();
    }
    else
    {
        Direction = CachedCharacter->GetActorForwardVector();
    }

    ConfigureControlPoints(Direction, Distance);

    ElapsedTime = 0.0f;
    bIsTransitionActive = true;
    bIsInteractionLocked = false;
    bIsExiting = true;

    ToggleCursor(false);

    SetComponentTickEnabled(true);
}

void UWorkstationInteractionComponent::ConfigureControlPoints(const FVector& Direction, float Distance)
{
    const float Bias = FMath::Clamp(CurveBias, 0.0f, 0.49f);
    const float DistanceBias = Distance * Bias;
    const FVector UpOffset = FVector::UpVector * ArcHeight;

    if (Distance > KINDA_SMALL_NUMBER)
    {
        ControlPointA = StartLocation + Direction * DistanceBias + UpOffset;
        ControlPointB = TargetLocation - Direction * DistanceBias + UpOffset;
    }
    else
    {
        ControlPointA = StartLocation + UpOffset;
        ControlPointB = TargetLocation + UpOffset;
    }
}

FVector UWorkstationInteractionComponent::EvaluateBezier(float T) const
{
    const FVector A = FMath::Lerp(StartLocation, ControlPointA, T);
    const FVector B = FMath::Lerp(ControlPointA, ControlPointB, T);
    const FVector C = FMath::Lerp(ControlPointB, TargetLocation, T);

    const FVector D = FMath::Lerp(A, B, T);
    const FVector E = FMath::Lerp(B, C, T);

    return FMath::Lerp(D, E, T);
}

void UWorkstationInteractionComponent::FinalizeEntry()
{
    bIsTransitionActive = false;
    bIsInteractionLocked = true;
    bIsExiting = false;

    ToggleCursor(true);
    SetComponentTickEnabled(false);
}

void UWorkstationInteractionComponent::FinalizeExit()
{
    bIsTransitionActive = false;
    bIsInteractionLocked = false;
    bIsExiting = false;

    ToggleCursor(false);

    if (CachedCharacter.IsValid())
    {
        CachedCharacter->LockMovementInput(false);
        CachedCharacter->LockLookInput(false);
        CachedCharacter->HandleWorkstationExitComplete();
    }

    WorkstationActor.Reset();

    SetComponentTickEnabled(false);
}

void UWorkstationInteractionComponent::ToggleCursor(bool bEnable)
{
    UpdateCachedController();
    if (CachedController.IsValid())
    {
        CachedController->bShowMouseCursor = bEnable;
        CachedController->bEnableClickEvents = bEnable;
        CachedController->bEnableMouseOverEvents = bEnable;
        
        if (bEnable)
        {
            // Enable mouse for UI interaction
            FInputModeGameAndUI InputMode;
            InputMode.SetHideCursorDuringCapture(false);
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
            CachedController->SetInputMode(InputMode);

            if (ULocalPlayer* LocalPlayer = CachedController->GetLocalPlayer())
            {
                if (UGameViewportClient* ViewportClient = LocalPlayer->ViewportClient)
                {
                    ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
                }
            }
        }
        else
        {
            // Return to game-only mode with captured mouse
            FInputModeGameOnly InputMode;
            InputMode.SetConsumeCaptureMouseDown(false);
            CachedController->SetInputMode(InputMode);

            if (ULocalPlayer* LocalPlayer = CachedController->GetLocalPlayer())
            {
                if (UGameViewportClient* ViewportClient = LocalPlayer->ViewportClient)
                {
                    ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently);
                    ViewportClient->SetMouseLockMode(EMouseLockMode::LockOnCapture);
                    ViewportClient->SetHideCursorDuringCapture(true);
                }
            }
        }
    }
}
