#include "ElevatorGameMode.h"

#include "FirstPersonCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AElevatorGameMode::AElevatorGameMode()
{
    DefaultPawnClass = AFirstPersonCharacter::StaticClass();
    PlayerSpawnTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector);
    bUseCustomSpawnTransform = true;
}

void AElevatorGameMode::BeginPlay()
{
    Super::BeginPlay();
    SpawnPlayerAtConfiguredTransform();
}

void AElevatorGameMode::SetPlayerSpawnTransform(const FTransform& NewTransform)
{
    PlayerSpawnTransform = NewTransform;
    bUseCustomSpawnTransform = true;
}

void AElevatorGameMode::RespawnPlayer()
{
    SpawnPlayerAtConfiguredTransform();
}

void AElevatorGameMode::MovePlayerToLocation(const FVector& TargetLocation, float Duration)
{
    if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (AFirstPersonCharacter* Character = Cast<AFirstPersonCharacter>(PlayerController->GetPawn()))
        {
            Character->SmoothMoveTo(TargetLocation, Duration);
        }
    }
}

void AElevatorGameMode::SpawnPlayerAtConfiguredTransform()
{
    if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (APawn* ExistingPawn = PlayerController->GetPawn())
        {
            ExistingPawn->Destroy();
        }

        FTransform SpawnTransform = FTransform::Identity;

        if (bUseCustomSpawnTransform)
        {
            SpawnTransform = PlayerSpawnTransform;
        }
        else if (AActor* StartSpot = FindPlayerStart(PlayerController))
        {
            SpawnTransform = StartSpot->GetActorTransform();
        }

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        if (UWorld* World = GetWorld())
        {
            if (APawn* SpawnedPawn = World->SpawnActor<APawn>(DefaultPawnClass, SpawnTransform, SpawnParameters))
            {
                PlayerController->Possess(SpawnedPawn);
            }
        }
    }
}
