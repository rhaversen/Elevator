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
    void Interact();

private:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UCameraComponent* FirstPersonCamera;

    UPROPERTY(EditAnywhere, Category = "Interaction")
    float InteractionTraceDistance = 150.0f;

    UPROPERTY(EditAnywhere, Category = "Interaction")
    float InteractionCheckInterval = 0.05f; // 20 times per second

    UPROPERTY(EditAnywhere, Category = "Interaction", meta = (ClampMin = "0.0"))
    float ButtonSearchRadius = 10.0f;

    UPROPERTY(EditAnywhere, Category = "Interaction")
    bool bEnableButtonHighlight = true;

    AActor* CurrentInteractable;
    UPrimitiveComponent* CurrentHighlightedComponent;
    float TimeSinceLastInteractionCheck;

    void UpdateButtonHighlight(UPrimitiveComponent* NewComponent);

    FVector SmoothMoveStart;
    FVector SmoothMoveTarget;
    float SmoothMoveDuration;
    float SmoothMoveElapsed;
    bool bIsSmoothMoving;
};
