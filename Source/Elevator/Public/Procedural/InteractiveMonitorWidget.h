#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SInteractiveMonitorWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SInteractiveMonitorWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void UpdateCursorPosition(const FVector2D& NewPosition);
    void SetWidgetSize(const FVector2D& NewSize);

private:
    FSlateColor GetButtonColor() const;

    bool bIsHovered = false;
    FVector2D CursorPosition = FVector2D(0.5f, 0.5f); // Normalized 0-1
    FVector2D WidgetSize = FVector2D(1920.0f, 1080.0f);
};
