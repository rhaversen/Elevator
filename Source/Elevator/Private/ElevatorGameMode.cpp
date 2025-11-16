#include "ElevatorGameMode.h"

#include "FirstPersonCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AElevatorGameMode::AElevatorGameMode()
{
    DefaultPawnClass = AFirstPersonCharacter::StaticClass();
    PlayerSpawnTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector);
    bUseCustomSpawnTransform = false;
    PreferredPlayerStartTag = FName(TEXT("ProceduralSpawn"));
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
        else
        {
            AActor* StartSpot = FindPreferredPlayerStart(PlayerController);
            if (!StartSpot)
            {
                StartSpot = FindPlayerStart(PlayerController);
            }

            if (StartSpot)
            {
                SpawnTransform = StartSpot->GetActorTransform();
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("No PlayerStart found for controller %s. Falling back to configured spawn transform."), *PlayerController->GetName());
                SpawnTransform = PlayerSpawnTransform;
            }
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

AActor* AElevatorGameMode::FindPreferredPlayerStart(AController* PlayerController) const
{
    if (PreferredPlayerStartTag.IsNone())
    {
        return nullptr;
    }

    TArray<AActor*> PlayerStarts;
    UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
    for (AActor* Actor : PlayerStarts)
    {
        if (APlayerStart* PlayerStart = Cast<APlayerStart>(Actor))
        {
            if (PlayerStart->PlayerStartTag == PreferredPlayerStartTag)
            {
                return PlayerStart;
            }
        }
    }

    return nullptr;
}
