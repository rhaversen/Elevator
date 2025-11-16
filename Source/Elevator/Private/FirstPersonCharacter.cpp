#include "FirstPersonCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

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
    AddControllerYawInput(Value);
}

void AFirstPersonCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

void AFirstPersonCharacter::StartJump()
{
    Jump();
}

void AFirstPersonCharacter::StopJump()
{
    StopJumping();
}
