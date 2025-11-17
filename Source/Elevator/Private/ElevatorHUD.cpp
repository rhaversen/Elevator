#include "ElevatorHUD.h"
#include "Engine/Canvas.h"

AElevatorHUD::AElevatorHUD()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AElevatorHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!bShowCrosshair || !Canvas)
    {
        return;
    }

    const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
    const FVector2D CrosshairDrawPosition(Center.X - CrosshairSize * 0.5f, Center.Y - CrosshairSize * 0.5f);

    FLinearColor DrawColor = CrosshairColor;
    DrawColor.A *= CrosshairOpacity;

    FCanvasTileItem TileItem(CrosshairDrawPosition, FVector2D(CrosshairSize, CrosshairSize), DrawColor);
    TileItem.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(TileItem);
}
