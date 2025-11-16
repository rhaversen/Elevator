#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FirstPersonCharacter.generated.h"

class UCameraComponent;

UCLASS()
class ELEVATOR_API AFirstPersonCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AFirstPersonCharacter();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category = "Cinematic")
    void SmoothMoveTo(const FVector& TargetLocation, float Duration = 1.0f);

protected:
    virtual void BeginPlay() override;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void StartJump();
    void StopJump();

private:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UCameraComponent* FirstPersonCamera;

    FVector SmoothMoveStart;
    FVector SmoothMoveTarget;
    float SmoothMoveDuration;
    float SmoothMoveElapsed;
    bool bIsSmoothMoving;
};
