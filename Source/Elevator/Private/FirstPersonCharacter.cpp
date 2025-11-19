#include "FirstPersonCharacter.h"

#include "Interactable.h"
#include "InteractionFocusProvider.h"
#include "Procedural/ProceduralElevator.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"

AFirstPersonCharacter::AFirstPersonCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(55.0f, 96.0f);

    bUseControllerRotationYaw = true;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bOrientRotationToMovement = false;
        MoveComp->MaxWalkSpeed = 600.0f;
    }

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;

    SmoothMoveDuration = 0.0f;
    SmoothMoveElapsed = 0.0f;
    bIsSmoothMoving = false;
    CurrentInteractable = nullptr;
    CurrentHighlightedComponent = nullptr;
    TimeSinceLastInteractionCheck = 0.0f;
}

void AFirstPersonCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AFirstPersonCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bIsSmoothMoving)
    {
        SmoothMoveElapsed += DeltaSeconds;
        const float DurationSafe = FMath::Max(SmoothMoveDuration, KINDA_SMALL_NUMBER);
        const float Alpha = FMath::Clamp(SmoothMoveElapsed / DurationSafe, 0.0f, 1.0f);
        const float EasedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
        SetActorLocation(FMath::Lerp(SmoothMoveStart, SmoothMoveTarget, EasedAlpha));

        if (Alpha >= 1.0f)
        {
            bIsSmoothMoving = false;
        }
    }

    if (bIsWorkstationTransitionActive)
    {
        WorkstationElapsedTime += DeltaSeconds;
        const float DurationSafe = FMath::Max(WorkstationTravelDuration, KINDA_SMALL_NUMBER);
        const float Alpha = FMath::Clamp(WorkstationElapsedTime / DurationSafe, 0.0f, 1.0f);
        const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

        const FVector NewLocation = EvaluateWorkstationBezier(SmoothAlpha);
        SetActorLocation(NewLocation, false);

        const FQuat NewQuat = FQuat::Slerp(WorkstationStartQuat, WorkstationTargetQuat, SmoothAlpha).GetNormalized();
        const FRotator NewRotator = NewQuat.Rotator();
        if (Controller)
        {
            Controller->SetControlRotation(NewRotator);
        }
        SetActorRotation(FRotator(0.0f, NewRotator.Yaw, 0.0f));

        if (Alpha >= 1.0f)
        {
            bIsWorkstationTransitionActive = false;
            WorkstationElapsedTime = DurationSafe;
            SetActorLocation(WorkstationTargetLocation, false);
            const FRotator TargetRotator = WorkstationTargetQuat.Rotator();
            if (Controller)
            {
                Controller->SetControlRotation(TargetRotator);
            }
            SetActorRotation(FRotator(0.0f, TargetRotator.Yaw, 0.0f));

            if (bIsExitingWorkstation)
            {
                // Fully exited - unlock controls
                bIsExitingWorkstation = false;
                bIsWorkstationLocked = false;
                LockMovementInput(false);
                LockLookInput(false);
                TimeSinceLastInteractionCheck = InteractionCheckInterval;
            }
            else
            {
                // Fully entered - lock in place
                bIsWorkstationLocked = true;
            }
        }
    }

    if (bIsWorkstationTransitionActive || bIsWorkstationLocked)
    {
        return;
    }

    // Check for interactables periodically
    TimeSinceLastInteractionCheck += DeltaSeconds;
    if (TimeSinceLastInteractionCheck >= InteractionCheckInterval)
    {
        TimeSinceLastInteractionCheck = 0.0f;

        CurrentInteractable = nullptr;
        UPrimitiveComponent* NewHighlightComponent = nullptr;

        if (FirstPersonCamera && Controller)
        {
            const FVector CameraLocation = FirstPersonCamera->GetComponentLocation();
            const FVector CameraForward = FirstPersonCamera->GetForwardVector();
            FVector TraceEnd = CameraLocation + (CameraForward * InteractionTraceDistance);

            FHitResult HitResult;
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(this);

            if (GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, TraceEnd, ECC_Visibility, QueryParams))
            {
                if (AActor* HitActor = HitResult.GetActor())
                {
                    if (HitActor->Implements<UInteractionFocusProvider>())
                    {
                        UPrimitiveComponent* CandidateHighlight = nullptr;
                        const bool bHasFocus = IInteractionFocusProvider::Execute_EvaluateInteractionFocus(HitActor, this, HitResult, InteractionAssistRadius, CandidateHighlight);

                        if (bHasFocus && HitActor->Implements<UInteractable>())
                        {
                            IInteractable* Interactable = Cast<IInteractable>(HitActor);
                            if (Interactable && Interactable->Execute_CanInteract(HitActor, this))
                            {
                                CurrentInteractable = HitActor;
                                NewHighlightComponent = CandidateHighlight;
                            }
                        }
                    }

                    if (!CurrentInteractable && HitActor->Implements<UInteractable>())
                    {
                        IInteractable* Interactable = Cast<IInteractable>(HitActor);
                        if (Interactable && Interactable->Execute_CanInteract(HitActor, this))
                        {
                            CurrentInteractable = HitActor;
                        }
                    }
                }
            }
        }

        UpdateInteractionHighlight(NewHighlightComponent);
    }
}

void AFirstPersonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &AFirstPersonCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AFirstPersonCharacter::MoveRight);
    PlayerInputComponent->BindAxis("Turn", this, &AFirstPersonCharacter::Turn);
    PlayerInputComponent->BindAxis("LookUp", this, &AFirstPersonCharacter::LookUp);

    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AFirstPersonCharacter::StartJump);
    PlayerInputComponent->BindAction("Jump", IE_Released, this, &AFirstPersonCharacter::StopJump);
    PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AFirstPersonCharacter::Interact);
    PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AFirstPersonCharacter::CancelWorkstationInteraction);
}

void AFirstPersonCharacter::SmoothMoveTo(const FVector& TargetLocation, float Duration)
{
    SmoothMoveStart = GetActorLocation();
    SmoothMoveTarget = TargetLocation;
    SmoothMoveDuration = FMath::Max(Duration, 0.0f);
    SmoothMoveElapsed = 0.0f;

    if (SmoothMoveDuration <= KINDA_SMALL_NUMBER)
    {
        SetActorLocation(SmoothMoveTarget);
        bIsSmoothMoving = false;
        return;
    }

    bIsSmoothMoving = true;
}

void AFirstPersonCharacter::MoveForward(float Value)
{
    if (bIsMovementInputLocked)
    {
        return;
    }

    if (Controller && !FMath::IsNearlyZero(Value))
    {
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        AddMovementInput(Direction, Value);
    }
}

void AFirstPersonCharacter::MoveRight(float Value)
{
    if (bIsMovementInputLocked)
    {
        return;
    }

    if (Controller && !FMath::IsNearlyZero(Value))
    {
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(Direction, Value);
    }
}

void AFirstPersonCharacter::Turn(float Value)
{
    if (bIsLookInputLocked)
    {
        return;
    }

    AddControllerYawInput(Value);
}

void AFirstPersonCharacter::LookUp(float Value)
{
    if (bIsLookInputLocked)
    {
        return;
    }

    AddControllerPitchInput(Value);
}

void AFirstPersonCharacter::StartJump()
{
    if (bIsMovementInputLocked)
    {
        return;
    }

    Jump();
}

void AFirstPersonCharacter::StopJump()
{
    if (bIsMovementInputLocked)
    {
        return;
    }

    StopJumping();
}

void AFirstPersonCharacter::Interact()
{
    if (bIsWorkstationTransitionActive)
    {
        return;
    }

    if (bIsWorkstationLocked)
    {
        CancelWorkstationInteraction();
        return;
    }

    if (!CurrentInteractable || !CurrentInteractable->Implements<UInteractable>())
    {
        return;
    }

    if (AProceduralElevator* Elevator = Cast<AProceduralElevator>(CurrentInteractable))
    {
        if (CurrentHighlightedComponent && Elevator->HandleButtonPressed(CurrentHighlightedComponent))
        {
            return;
        }
    }

    IInteractable::Execute_OnInteract(CurrentInteractable, this);
}

void AFirstPersonCharacter::UpdateInteractionHighlight(UPrimitiveComponent* NewComponent)
{
    if (!bEnableInteractionHighlight)
    {
        return;
    }

    // Clear old highlight
    if (CurrentHighlightedComponent && CurrentHighlightedComponent != NewComponent)
    {
        CurrentHighlightedComponent->SetRenderCustomDepth(false);
        CurrentHighlightedComponent->SetCustomDepthStencilValue(0);
    }

    // Apply new highlight
    CurrentHighlightedComponent = NewComponent;
    if (CurrentHighlightedComponent)
    {
        CurrentHighlightedComponent->SetRenderCustomDepth(true);
        CurrentHighlightedComponent->SetCustomDepthStencilValue(252); // Outline value reserved for interaction highlight

    }
}

void AFirstPersonCharacter::BeginWorkstationInteraction(const FTransform& TargetTransform, float TravelTime, float ArcHeight, float CurveBias)
{
    if (bIsWorkstationTransitionActive || bIsWorkstationLocked)
    {
        return;
    }

    WorkstationStartLocation = GetActorLocation();
    WorkstationTargetLocation = TargetTransform.GetLocation();

    // Adjust target location so the camera ends up at the target transform location
    if (FirstPersonCamera)
    {
        WorkstationTargetLocation -= FirstPersonCamera->GetRelativeLocation();
    }

    const FRotator StartRotator = Controller ? Controller->GetControlRotation() : GetActorRotation();
    WorkstationStartQuat = StartRotator.Quaternion();
    const FQuat RawTargetQuat = TargetTransform.GetRotation();
    WorkstationTargetQuat = RawTargetQuat.IsNormalized() ? RawTargetQuat : RawTargetQuat.GetNormalized();

    FVector PathDirection = WorkstationTargetLocation - WorkstationStartLocation;
    const float Distance = PathDirection.Size();
    if (Distance > KINDA_SMALL_NUMBER)
    {
        PathDirection /= Distance;
    }
    else
    {
        PathDirection = FirstPersonCamera ? FirstPersonCamera->GetForwardVector() : GetActorForwardVector();
    }

    const float Bias = FMath::Clamp(CurveBias, 0.0f, 0.49f);
    const float DistanceBias = Distance * Bias;
    const FVector UpOffset = FVector::UpVector * ArcHeight;

    if (Distance > KINDA_SMALL_NUMBER)
    {
        WorkstationControlPointA = WorkstationStartLocation + PathDirection * DistanceBias + UpOffset;
        WorkstationControlPointB = WorkstationTargetLocation - PathDirection * DistanceBias + UpOffset;
    }
    else
    {
        WorkstationControlPointA = WorkstationStartLocation + UpOffset;
        WorkstationControlPointB = WorkstationTargetLocation + UpOffset;
    }

    WorkstationTravelDuration = FMath::Max(TravelTime, 0.01f);
    WorkstationElapsedTime = 0.0f;
    WorkstationArcHeight = ArcHeight;
    WorkstationCurveBias = CurveBias;

    bIsWorkstationTransitionActive = true;
    bIsWorkstationLocked = false;
    bIsExitingWorkstation = false;

    LockMovementInput(true);
    LockLookInput(true);
    bIsSmoothMoving = false;

    UpdateInteractionHighlight(nullptr);
    CurrentInteractable = nullptr;
    CurrentHighlightedComponent = nullptr;
    TimeSinceLastInteractionCheck = 0.0f;

    SetActorRotation(FRotator(0.0f, StartRotator.Yaw, 0.0f));
}

void AFirstPersonCharacter::CancelWorkstationInteraction()
{
    if (!bIsWorkstationTransitionActive && !bIsWorkstationLocked)
    {
        return;
    }

    if (bIsWorkstationTransitionActive && !bIsExitingWorkstation)
    {
        // If already transitioning in, ignore
        return;
    }

    // Begin exit transition - swap start and target
    const FVector CurrentLocation = GetActorLocation();
    const FRotator CurrentRotator = Controller ? Controller->GetControlRotation() : GetActorRotation();

    // Swap locations and rotations for return journey
    const FVector TempLocation = WorkstationStartLocation;
    WorkstationStartLocation = CurrentLocation;
    WorkstationTargetLocation = TempLocation;

    const FQuat TempQuat = WorkstationStartQuat;
    WorkstationStartQuat = CurrentRotator.Quaternion();
    WorkstationTargetQuat = TempQuat;

    // Recalculate control points for the return arc
    FVector PathDirection = WorkstationTargetLocation - WorkstationStartLocation;
    const float Distance = PathDirection.Size();
    if (Distance > KINDA_SMALL_NUMBER)
    {
        PathDirection /= Distance;
    }
    else
    {
        PathDirection = FirstPersonCamera ? FirstPersonCamera->GetForwardVector() : GetActorForwardVector();
    }

    const float Bias = FMath::Clamp(WorkstationCurveBias, 0.0f, 0.49f);
    const float DistanceBias = Distance * Bias;
    const FVector UpOffset = FVector::UpVector * WorkstationArcHeight;

    if (Distance > KINDA_SMALL_NUMBER)
    {
        WorkstationControlPointA = WorkstationStartLocation + PathDirection * DistanceBias + UpOffset;
        WorkstationControlPointB = WorkstationTargetLocation - PathDirection * DistanceBias + UpOffset;
    }
    else
    {
        WorkstationControlPointA = WorkstationStartLocation + UpOffset;
        WorkstationControlPointB = WorkstationTargetLocation + UpOffset;
    }

    WorkstationElapsedTime = 0.0f;
    bIsWorkstationTransitionActive = true;
    bIsWorkstationLocked = false;
    bIsExitingWorkstation = true;
}

FVector AFirstPersonCharacter::EvaluateWorkstationBezier(float T) const
{
    const FVector P0 = WorkstationStartLocation;
    const FVector P1 = WorkstationControlPointA;
    const FVector P2 = WorkstationControlPointB;
    const FVector P3 = WorkstationTargetLocation;

    const FVector A = FMath::Lerp(P0, P1, T);
    const FVector B = FMath::Lerp(P1, P2, T);
    const FVector C = FMath::Lerp(P2, P3, T);

    const FVector D = FMath::Lerp(A, B, T);
    const FVector E = FMath::Lerp(B, C, T);

    return FMath::Lerp(D, E, T);
}

void AFirstPersonCharacter::LockMovementInput(bool bLock)
{
    bIsMovementInputLocked = bLock;

    if (Controller)
    {
        Controller->SetIgnoreMoveInput(bLock);
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        if (bLock)
        {
            MoveComp->StopMovementImmediately();
            MoveComp->DisableMovement();
        }
        else
        {
            MoveComp->SetMovementMode(MOVE_Walking);
        }
    }
}

void AFirstPersonCharacter::LockLookInput(bool bLock)
{
    bIsLookInputLocked = bLock;

    if (Controller)
    {
        Controller->SetIgnoreLookInput(bLock);
    }
}
