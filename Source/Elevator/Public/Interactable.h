#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface for actors that can be interacted with by the player.
 */
class ELEVATOR_API IInteractable
{
    GENERATED_BODY()

public:
    /**
     * Called when the player looks at this interactable.
     * @param PlayerPawn The pawn that is looking at this object
     * @return True if the object can be interacted with, false otherwise
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool CanInteract(APawn* PlayerPawn) const;
    virtual bool CanInteract_Implementation(APawn* PlayerPawn) const { return true; }

    /**
     * Called when the player presses the interact key while looking at this object.
     * @param PlayerPawn The pawn that initiated the interaction
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnInteract(APawn* PlayerPawn);
    virtual void OnInteract_Implementation(APawn* PlayerPawn) { /* Override in derived classes */ }

    /**
     * Get the interaction prompt to display to the player.
     * @return The text to display (e.g., "Press E to call elevator")
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    FText GetInteractionPrompt() const;
    virtual FText GetInteractionPrompt_Implementation() const { return FText::FromString(TEXT("Press E to interact")); }

    /**
     * Called when the player cancels an ongoing interaction (e.g. leaves a workstation).
     * @param PlayerPawn The pawn that canceled the interaction
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnInteractionCanceled(APawn* PlayerPawn);
    virtual void OnInteractionCanceled_Implementation(APawn* PlayerPawn) { /* Override in derived classes */ }

    /**
     * Called when the player provides input while interacting (e.g. moving mouse in workstation view).
     * @param PlayerPawn The pawn providing input
     * @param InputDelta The input delta (X = Yaw/Turn, Y = Pitch/LookUp)
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnInteractionInput(APawn* PlayerPawn, FVector2D InputDelta);
    virtual void OnInteractionInput_Implementation(APawn* PlayerPawn, FVector2D InputDelta) { /* Override in derived classes */ }

    /**
     * Called when the player hovers over the interactable with a hardware cursor.
     * @param Hit The hit result from the cursor trace
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnInteractionHover(const FHitResult& Hit);
    virtual void OnInteractionHover_Implementation(const FHitResult& Hit) { /* Override in derived classes */ }

    /**
     * Called when the player clicks while interacting with the object (e.g. mouse button press in workstation view).
     * @param PlayerPawn The pawn that triggered the click
     * @param Hit The hit result from the click trace
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void OnInteractionClick(APawn* PlayerPawn, const FHitResult& Hit);
    virtual void OnInteractionClick_Implementation(APawn* PlayerPawn, const FHitResult& Hit) { /* Override in derived classes */ }
};
