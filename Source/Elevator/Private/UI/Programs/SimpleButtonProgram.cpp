#include "UI/Programs/SimpleButtonProgram.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"

FSimpleButtonProgram::FSimpleButtonProgram()
{
}

namespace
{
    // Helper function to create an outlined button with text and hover indicator
    TSharedRef<SWidget> CreateOutlinedButton(
        float Width,
        float Height,
        const FText& ButtonText,
        int32 FontSize,
        TAttribute<FSlateColor> MainColor,
        TAttribute<bool> IsHovered)
    {
        const float BorderThickness = 2.0f;

        return SNew(SBox)
            .WidthOverride(Width)
            .HeightOverride(Height)
            [
                SNew(SOverlay)
                // Background fill (visible on hover)
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Fill)
                [
                    SNew(SImage)
                    .Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .ColorAndOpacity(MainColor)
                    .Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([IsHovered]() {
                        return IsHovered.Get() ? EVisibility::Visible : EVisibility::Hidden;
                    })))
                ]
                // Top border
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Top)
                [
                    SNew(SBox)
                    .HeightOverride(BorderThickness)
                    [
                        SNew(SImage)
                        .Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .ColorAndOpacity(MainColor)
                    ]
                ]
                // Bottom border
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Bottom)
                [
                    SNew(SBox)
                    .HeightOverride(BorderThickness)
                    [
                        SNew(SImage)
                        .Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .ColorAndOpacity(MainColor)
                    ]
                ]
                // Left border
                + SOverlay::Slot()
                .HAlign(HAlign_Left)
                .VAlign(VAlign_Fill)
                [
                    SNew(SBox)
                    .WidthOverride(BorderThickness)
                    [
                        SNew(SImage)
                        .Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .ColorAndOpacity(MainColor)
                    ]
                ]
                // Right border
                + SOverlay::Slot()
                .HAlign(HAlign_Right)
                .VAlign(VAlign_Fill)
                [
                    SNew(SBox)
                    .WidthOverride(BorderThickness)
                    [
                        SNew(SImage)
                        .Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .ColorAndOpacity(MainColor)
                    ]
                ]
                // Centered text
                + SOverlay::Slot()
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(ButtonText)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", FontSize))
                    .ColorAndOpacity(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda([MainColor, IsHovered]() {
                        return IsHovered.Get() ? FSlateColor(FLinearColor::Black) : MainColor.Get();
                    })))
                ]
            ];
    }
}

TSharedRef<SWidget> FSimpleButtonProgram::CreateWidget(const FVector2D& Size)
{
    ScreenSize = Size;

    return SNew(SOverlay)
        // Background (MUST be completely black)
        + SOverlay::Slot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SNew(SImage)
            .ColorAndOpacity(FLinearColor::Black)
        ]
        // Buttons container (Centered)
        + SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SHorizontalBox)
            // Task button
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                CreateOutlinedButton(
                    TaskButtonWidth,
                    TaskButtonHeight,
                    FText::FromString(TEXT("Complete Task")),
                    16,
                    TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateSP(this, &FSimpleButtonProgram::GetTaskButtonColor)),
                    TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([this](){ return bTaskButtonHovered; }))
                )
            ]
            // Spacing
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBox)
                .WidthOverride(ButtonSpacing)
            ]
            // Exit button
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                CreateOutlinedButton(
                    ExitButtonWidth,
                    ExitButtonHeight,
                    FText::FromString(TEXT("Exit")),
                    14,
                    TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateSP(this, &FSimpleButtonProgram::GetExitButtonColor)),
                    TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([this](){ return bExitButtonHovered; }))
                )
            ]
        ]
        // Task status indicator (bottom center)
        + SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Bottom)
        .Padding(FMargin(0.0f, 0.0f, 0.0f, 30.0f))
        [
            SNew(STextBlock)
            .Text(this, &FSimpleButtonProgram::GetTaskStatusText)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
            .ColorAndOpacity(FLinearColor::Green)
        ];
}

void FSimpleButtonProgram::GetButtonRects(const FVector2D& InScreenSize, FBox2D& OutTaskRect, FBox2D& OutExitRect) const
{
    const float TotalWidth = TaskButtonWidth + ButtonSpacing + ExitButtonWidth;
    const float StartX = (InScreenSize.X - TotalWidth) * 0.5f;
    const float CenterY = InScreenSize.Y * 0.5f;

    // Task Button
    FVector2D TaskMin(StartX, CenterY - TaskButtonHeight * 0.5f);
    FVector2D TaskMax(StartX + TaskButtonWidth, CenterY + TaskButtonHeight * 0.5f);
    OutTaskRect = FBox2D(TaskMin, TaskMax);

    // Exit Button
    FVector2D ExitMin(StartX + TaskButtonWidth + ButtonSpacing, CenterY - ExitButtonHeight * 0.5f);
    FVector2D ExitMax(ExitMin.X + ExitButtonWidth, ExitMin.Y + ExitButtonHeight);
    OutExitRect = FBox2D(ExitMin, ExitMax);
}

void FSimpleButtonProgram::UpdateCursor(const FVector2D& NormalizedPosition)
{
    CursorPosition = NormalizedPosition;

    const FVector2D PixelPos = CursorPosition * ScreenSize;
    
    FBox2D TaskRect, ExitRect;
    GetButtonRects(ScreenSize, TaskRect, ExitRect);

    bTaskButtonHovered = TaskRect.IsInside(PixelPos);
    bExitButtonHovered = ExitRect.IsInside(PixelPos);
}

bool FSimpleButtonProgram::HandleClick()
{
    bool bHandled = false;

    if (bTaskButtonHovered)
    {
        bTaskComplete = true;
        bHandled = true;
    }

    if (bExitButtonHovered)
    {
        RequestExit();
        bHandled = true;
    }

    return bHandled;
}

void FSimpleButtonProgram::OnScreenResized(const FVector2D& NewSize)
{
    ScreenSize.X = FMath::Max(NewSize.X, 1.0f);
    ScreenSize.Y = FMath::Max(NewSize.Y, 1.0f);
    UpdateCursor(CursorPosition);
}

FSlateColor FSimpleButtonProgram::GetTaskButtonColor() const
{
    return FSlateColor(FLinearColor::Green); // Always green
}

FSlateColor FSimpleButtonProgram::GetExitButtonColor() const
{
    return FSlateColor(FLinearColor::Green); // Always green
}

FText FSimpleButtonProgram::GetTaskStatusText() const
{
    return bTaskComplete ? FText::FromString(TEXT("Task Complete")) : FText::FromString(TEXT("Task Pending"));
}
