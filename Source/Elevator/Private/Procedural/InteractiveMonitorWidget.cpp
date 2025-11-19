#include "Procedural/InteractiveMonitorWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"

void SInteractiveMonitorWidget::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SNew(SImage)
            .ColorAndOpacity(FLinearColor::Black)
        ]
        + SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .WidthOverride(200)
            .HeightOverride(100)
            [
                SNew(SImage)
                .ColorAndOpacity(this, &SInteractiveMonitorWidget::GetButtonColor)
            ]
        ]
    ];
}

void SInteractiveMonitorWidget::UpdateCursorPosition(const FVector2D& NewPosition)
{
    CursorPosition = NewPosition;
    
    // Simple hit test logic
    // Button is centered, 200x100
    // Screen is WidgetSize (dynamic)
    // Center is WidgetSize * 0.5
    
    const FVector2D PixelPos = CursorPosition * WidgetSize;
    const FVector2D Center = WidgetSize * 0.5f;
    const FVector2D HalfButtonSize(100.0f, 50.0f);
    
    if (PixelPos.X >= Center.X - HalfButtonSize.X && PixelPos.X <= Center.X + HalfButtonSize.X &&
        PixelPos.Y >= Center.Y - HalfButtonSize.Y && PixelPos.Y <= Center.Y + HalfButtonSize.Y)
    {
        bIsHovered = true;
    }
    else
    {
        bIsHovered = false;
    }
}

void SInteractiveMonitorWidget::SetWidgetSize(const FVector2D& NewSize)
{
    WidgetSize.X = FMath::Max(NewSize.X, 1.0f);
    WidgetSize.Y = FMath::Max(NewSize.Y, 1.0f);
}

FSlateColor SInteractiveMonitorWidget::GetButtonColor() const
{
    return bIsHovered ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Red);
}
