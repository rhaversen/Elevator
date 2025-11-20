#include "FirstPersonCharacter.h"

#include "Interactable.h"
#include "InteractionFocusProvider.h"
#include "Procedural/ProceduralElevator.h"
#include "Camera/CameraComponent.h"
#include "Interaction/WorkstationInteractionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
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

    WorkstationInteractionComponent = CreateDefaultSubobject<UWorkstationInteractionComponent>(TEXT("WorkstationInteractionComponent"));

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

    if (WorkstationInteractionComponent && WorkstationInteractionComponent->IsTransitionActive())
    {
        return;
    }

    if (WorkstationInteractionComponent && WorkstationInteractionComponent->IsInteractionLocked())
    {
        if (APlayerController* PC = Cast<APlayerController>(Controller))
        {
            FHitResult Hit;
            if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
            {
                if (AActor* WorkstationActor = WorkstationInteractionComponent->GetCurrentWorkstationActor())
                {
                    if (Hit.GetActor() == WorkstationActor && WorkstationActor->Implements<UInteractable>())
                    {
                        IInteractable::Execute_OnInteractionHover(WorkstationActor, Hit);
                    }
                }
            }
        }
        return;
    }

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
            const FVector TraceEnd = CameraLocation + (CameraForward * InteractionTraceDistance);

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
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AFirstPersonCharacter::HandleWorkstationPointerPressed);
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AFirstPersonCharacter::HandleWorkstationPointerReleased);
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
    if (WorkstationInteractionComponent && WorkstationInteractionComponent->IsTransitionActive())
    {
        return;
    }

    if (WorkstationInteractionComponent && WorkstationInteractionComponent->IsInteractionLocked())
    {
        // Don't allow regular interact to cancel workstation - only Escape key should do that
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

void AFirstPersonCharacter::BeginWorkstationInteraction(const FTransform& TargetTransform, float TravelTime, float ArcHeight, float CurveBias, AActor* WorkstationActor)
{
    if (!WorkstationInteractionComponent)
    {
        return;
    }

    if (WorkstationInteractionComponent->IsTransitionActive() || WorkstationInteractionComponent->IsInteractionLocked())
    {
        return;
    }

    bIsSmoothMoving = false;
    UpdateInteractionHighlight(nullptr);
    CurrentInteractable = nullptr;
    CurrentHighlightedComponent = nullptr;
    TimeSinceLastInteractionCheck = 0.0f;

    WorkstationInteractionComponent->BeginInteraction(TargetTransform, TravelTime, ArcHeight, CurveBias, WorkstationActor);
}

void AFirstPersonCharacter::CancelWorkstationInteraction()
{
    if (!WorkstationInteractionComponent)
    {
        return;
    }

    WorkstationInteractionComponent->CancelInteraction();
}

void AFirstPersonCharacter::HandleWorkstationPointerPressed()
{
    if (!WorkstationInteractionComponent || !WorkstationInteractionComponent->IsInteractionLocked())
    {
        return;
    }

    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        FHitResult Hit;
        if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
        {
            if (AActor* WorkstationActor = WorkstationInteractionComponent->GetCurrentWorkstationActor())
            {
                if (Hit.GetActor() == WorkstationActor && WorkstationActor->Implements<UInteractable>())
                {
                    IInteractable::Execute_OnInteractionPointerPressed(WorkstationActor, this, Hit, EKeys::LeftMouseButton);
                }
            }
        }
    }
}

void AFirstPersonCharacter::HandleWorkstationPointerReleased()
{
    if (!WorkstationInteractionComponent || !WorkstationInteractionComponent->IsInteractionLocked())
    {
        return;
    }

    if (AActor* WorkstationActor = WorkstationInteractionComponent->GetCurrentWorkstationActor())
    {
        if (WorkstationActor->Implements<UInteractable>())
        {
            FHitResult Hit;
            if (APlayerController* PC = Cast<APlayerController>(Controller))
            {
                PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit);
            }

            IInteractable::Execute_OnInteractionPointerReleased(WorkstationActor, this, Hit, EKeys::LeftMouseButton);
        }
    }
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

void AFirstPersonCharacter::HandleWorkstationExitComplete()
{
    TimeSinceLastInteractionCheck = InteractionCheckInterval;
}

void AFirstPersonCharacter::LockLookInput(bool bLock)
{
    bIsLookInputLocked = bLock;

    if (Controller)
    {
        Controller->SetIgnoreLookInput(bLock);
    }
}
