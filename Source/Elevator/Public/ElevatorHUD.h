#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ElevatorHUD.generated.h"

UCLASS()
class ELEVATOR_API AElevatorHUD : public AHUD
{
    GENERATED_BODY()

public:
    AElevatorHUD();

    virtual void DrawHUD() override;

    /** Size of the crosshair dot in pixels */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
    float CrosshairSize = 2.0f;

    /** Color of the crosshair */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
    FLinearColor CrosshairColor = FLinearColor::White;

    /** Opacity of the crosshair */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CrosshairOpacity = 0.8f;

    /** Whether to show the crosshair */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
    bool bShowCrosshair = true;
};
