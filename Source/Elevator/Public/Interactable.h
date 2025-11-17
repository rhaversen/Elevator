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
};
