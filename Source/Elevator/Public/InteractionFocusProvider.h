#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Engine/EngineTypes.h"
#include "InteractionFocusProvider.generated.h"

class APawn;
class UPrimitiveComponent;

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractionFocusProvider : public UInterface
{
    GENERATED_BODY()
};

class ELEVATOR_API IInteractionFocusProvider
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool EvaluateInteractionFocus(APawn* PlayerPawn, const FHitResult& Hit, float AssistRadius, UPrimitiveComponent*& OutHighlightComponent);
    virtual bool EvaluateInteractionFocus_Implementation(APawn* PlayerPawn, const FHitResult& Hit, float AssistRadius, UPrimitiveComponent*& OutHighlightComponent)
    {
        OutHighlightComponent = nullptr;
        return false;
    }
};
