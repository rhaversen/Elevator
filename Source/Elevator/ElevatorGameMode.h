#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ElevatorGameMode.generated.h"

UCLASS()
class ELEVATOR_API AElevatorGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AElevatorGameMode();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Spawning")
    void SetPlayerSpawnTransform(const FTransform& NewTransform);

    UFUNCTION(BlueprintCallable, Category = "Spawning")
    void RespawnPlayer();

    UFUNCTION(BlueprintCallable, Category = "Cinematic")
    void MovePlayerToLocation(const FVector& TargetLocation, float Duration = 1.0f);

protected:
    UPROPERTY(EditAnywhere, Category = "Spawning", meta = (InlineEditConditionToggle))
    bool bUseCustomSpawnTransform;

    UPROPERTY(EditAnywhere, Category = "Spawning", meta = (EditCondition = "bUseCustomSpawnTransform"))
    FTransform PlayerSpawnTransform;

    UPROPERTY(EditAnywhere, Category = "Spawning")
    FName PreferredPlayerStartTag;

private:
    void SpawnPlayerAtConfiguredTransform();
    AActor* FindPreferredPlayerStart(AController* PlayerController) const;
};
