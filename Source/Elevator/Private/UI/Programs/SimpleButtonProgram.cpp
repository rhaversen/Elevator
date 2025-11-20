#include "UI/Programs/SimpleButtonProgram.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/CoreStyle.h"

namespace
{
    TSharedRef<SWidget> CreateOutlinedButton(
        const FVector2D& Size,
        const FText& ButtonText,
        int32 FontSize,
        TAttribute<FSlateColor> BorderColorAttr,
        TAttribute<FSlateColor> FillColorAttr,
        TAttribute<FSlateColor> TextColorAttr)
    {
        const float BorderThickness = 2.0f;
        const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");

        return SNew(SBox)
            .WidthOverride(Size.X)
            .HeightOverride(Size.Y)
            [
                SNew(SOverlay)
                // Background fill
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Fill)
                [
                    SNew(SImage)
                    .Image(Brush)
                    .ColorAndOpacity(FillColorAttr)
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
                        .Image(Brush)
                        .ColorAndOpacity(BorderColorAttr)
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
                        .Image(Brush)
                        .ColorAndOpacity(BorderColorAttr)
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
                        .Image(Brush)
                        .ColorAndOpacity(BorderColorAttr)
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
                        .Image(Brush)
                        .ColorAndOpacity(BorderColorAttr)
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
                    .ColorAndOpacity(TextColorAttr)
                ]
            ];
    }
}

FSimpleButtonProgram::FSimpleButtonProgram()
{
    // Initialize a default window
    Window.Position = FVector2D(400.0f, 300.0f);
    Window.Size = FVector2D(600.0f, 400.0f);
    Window.Title = TEXT("My Computer");
}



TSharedRef<SWidget> FSimpleButtonProgram::CreateWidget(const FVector2D& Size)
{
    ProgramSize = FVector2D(FMath::Max(Size.X, 1.0f), FMath::Max(Size.Y, 1.0f));
    ApplyCursorPosition(CursorPosition);

    const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
    const FLinearColor Black = FLinearColor::Black;
    const FLinearColor Green = FLinearColor::Green;

    return SNew(SCanvas)
        // The Window
        + SCanvas::Slot()
        .Position(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(this, &FSimpleButtonProgram::GetWindowPosition)))
        .Size(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(this, &FSimpleButtonProgram::GetWindowSize)))
        [
            SNew(SBorder)
            .BorderImage(Brush)
            .BorderBackgroundColor(Green)
            .Padding(2.0f)
            [
                SNew(SBorder)
                .BorderImage(Brush)
                .BorderBackgroundColor(Black)
                .Padding(0.0f)
                [
                    SNew(SVerticalBox)
                    // Title Bar
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SBorder)
                        .BorderImage(Brush)
                        .BorderBackgroundColor(Black)
                        .Padding(FMargin(10.0f, 8.0f))
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(Window.Title))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                            .ColorAndOpacity(Green)
                        ]
                    ]
                    // Divider
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SBox)
                        .HeightOverride(2.0f)
                        [
                            SNew(SImage)
                            .Image(Brush)
                            .ColorAndOpacity(Green)
                        ]
                    ]
                    // Content Area
                    + SVerticalBox::Slot()
                    .FillHeight(1.0f)
                    [
                        SNew(SBorder)
                        .BorderImage(Brush)
                        .BorderBackgroundColor(Black)
                        .Padding(10.0f)
                        [
                            SNew(SOverlay)
                            // Instructions text
                            + SOverlay::Slot()
                            .HAlign(HAlign_Left)
                            .VAlign(VAlign_Top)
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString(TEXT("Welcome to Mock OS v0.1\n\n- Drag title bar to move\n- Drag bottom-right to resize\n- Click Complete to finish task")))
                                .ColorAndOpacity(Green)
                            ]
                            // Complete button in center
                            + SOverlay::Slot()
                            .HAlign(HAlign_Center)
                            .VAlign(VAlign_Center)
                            [
                                CreateOutlinedButton(
                                    FVector2D(CompleteButtonWidth, CompleteButtonHeight),
                                    FText::FromString(TEXT("Complete")),
                                    16,
                                    TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateSP(this, &FSimpleButtonProgram::GetCompleteButtonBorderColor)),
                                    TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateSP(this, &FSimpleButtonProgram::GetCompleteButtonFillColor)),
                                    TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateSP(this, &FSimpleButtonProgram::GetCompleteButtonTextColor)))
                            ]
                        ]
                    ]
                ]
            ]
        ];
}

void FSimpleButtonProgram::OnPointerMoved(const FScreenPointerEvent& Event)
{
    ApplyCursorPosition(Event.ProgramNormalizedPosition);
}

void FSimpleButtonProgram::OnPointerPressed(const FScreenPointerEvent& Event)
{
    ApplyCursorPosition(Event.ProgramNormalizedPosition);

    if (CurrentState != EInteractionState::Idle)
    {
        UpdateCursorStyle();
        return;
    }

    const FVector2D PixelPos = CursorPosition * ProgramSize;

    if (IsCursorOverCompleteButton())
    {
        bCompleteButtonPressed = true;
        ActivePointerKey = Event.TriggerKey;
    }
    else if (IsCursorOverResizeHandle())
    {
        CurrentState = EInteractionState::Resizing;
        ActivePointerKey = Event.TriggerKey;
    }
    else if (IsCursorOverTitleBar())
    {
        CurrentState = EInteractionState::Dragging;
        DragOffset = PixelPos - Window.Position;
        ActivePointerKey = Event.TriggerKey;
    }
    else
    {
        ActivePointerKey = EKeys::Invalid;
    }

    UpdateCursorStyle();
}

void FSimpleButtonProgram::OnPointerReleased(const FScreenPointerEvent& Event)
{
    ApplyCursorPosition(Event.ProgramNormalizedPosition);

    if (!ActivePointerKey.IsValid())
    {
        return;
    }

    if (Event.TriggerKey.IsValid() && ActivePointerKey != Event.TriggerKey)
    {
        return;
    }

    const bool bWasPressedOnComplete = bCompleteButtonPressed;
    const bool bIsReleasingOverComplete = IsCursorOverCompleteButton();

    if (bWasPressedOnComplete && bIsReleasingOverComplete && !bTaskCompleted)
    {
        bTaskCompleted = true;
    }

    bCompleteButtonPressed = false;
    CurrentState = EInteractionState::Idle;
    ActivePointerKey = EKeys::Invalid;
    UpdateCursorStyle();
}

void FSimpleButtonProgram::OnScreenResized(const FVector2D& NewSize)
{
    ProgramSize.X = FMath::Max(NewSize.X, 1.0f);
    ProgramSize.Y = FMath::Max(NewSize.Y, 1.0f);
    ApplyCursorPosition(CursorPosition);
}

FVector2D FSimpleButtonProgram::GetWindowPosition() const
{
    return Window.Position;
}

FVector2D FSimpleButtonProgram::GetWindowSize() const
{
    return Window.Size;
}

bool FSimpleButtonProgram::IsCursorOverTitleBar() const
{
    const FVector2D PixelPos = CursorPosition * ProgramSize;
    
    // Title bar rect relative to screen
    FBox2D TitleRect(Window.Position, Window.Position + FVector2D(Window.Size.X, TitleBarHeight));
    
    return TitleRect.IsInside(PixelPos);
}

bool FSimpleButtonProgram::IsCursorOverResizeHandle() const
{
    const FVector2D PixelPos = CursorPosition * ProgramSize;
    
    // Bottom right corner
    FVector2D WindowBottomRight = Window.Position + Window.Size;
    FBox2D HandleRect(WindowBottomRight - FVector2D(ResizeHandleSize, ResizeHandleSize), WindowBottomRight);
    
    return HandleRect.IsInside(PixelPos);
}

bool FSimpleButtonProgram::IsCursorOverCompleteButton() const
{
    const FVector2D PixelPos = CursorPosition * ProgramSize;
    
    // Calculate the content area (inside the window, below title bar)
    const FVector2D ContentPos = Window.Position + FVector2D(0.0f, TitleBarHeight + 2.0f);
    const FVector2D ContentSize = FVector2D(Window.Size.X, Window.Size.Y - TitleBarHeight - 2.0f);
    
    // Button is centered in content area
    const FVector2D ButtonPos(
        ContentPos.X + (ContentSize.X - CompleteButtonWidth) * 0.5f,
        ContentPos.Y + (ContentSize.Y - CompleteButtonHeight) * 0.5f
    );
    
    FBox2D ButtonRect(ButtonPos, ButtonPos + FVector2D(CompleteButtonWidth, CompleteButtonHeight));
    
    return ButtonRect.IsInside(PixelPos);
}

FSlateColor FSimpleButtonProgram::GetCompleteButtonBorderColor() const
{
    return bTaskCompleted ? FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)) : FSlateColor(FLinearColor::Green);
}

FSlateColor FSimpleButtonProgram::GetCompleteButtonFillColor() const
{
    if (bTaskCompleted)
    {
        return FSlateColor(FLinearColor(0.2f, 0.2f, 0.2f));
    }
    return bCompleteButtonHovered ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Transparent);
}

FSlateColor FSimpleButtonProgram::GetCompleteButtonTextColor() const
{
    if (bTaskCompleted)
    {
        return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f));
    }
    return bCompleteButtonHovered ? FSlateColor(FLinearColor::Black) : FSlateColor(FLinearColor::Green);
}

void FSimpleButtonProgram::ApplyCursorPosition(const FVector2D& NormalizedPosition)
{
    CursorPosition.X = FMath::Clamp(NormalizedPosition.X, 0.0f, 1.0f);
    CursorPosition.Y = FMath::Clamp(NormalizedPosition.Y, 0.0f, 1.0f);

    const FVector2D PixelPos = CursorPosition * ProgramSize;

    if (CurrentState == EInteractionState::Dragging)
    {
        Window.Position = PixelPos - DragOffset;

        Window.Position.X = FMath::Clamp(Window.Position.X, 0.0f, ProgramSize.X - Window.Size.X);
        Window.Position.Y = FMath::Clamp(Window.Position.Y, 0.0f, ProgramSize.Y - Window.Size.Y);
    }
    else if (CurrentState == EInteractionState::Resizing)
    {
        const FVector2D NewSize = PixelPos - Window.Position;
        Window.Size.X = FMath::Max(NewSize.X, MinWindowSize);
        Window.Size.Y = FMath::Max(NewSize.Y, MinWindowSize);
    }

    UpdateCursorStyle();
}

void FSimpleButtonProgram::UpdateCursorStyle()
{
    bCompleteButtonHovered = IsCursorOverCompleteButton();

    if (CurrentState == EInteractionState::Resizing || IsCursorOverResizeHandle())
    {
        ActiveCursor = EMouseCursor::ResizeSouthEast;
    }
    else if (CurrentState == EInteractionState::Dragging)
    {
        ActiveCursor = EMouseCursor::GrabHand;
    }
    else if (bCompleteButtonPressed && !bTaskCompleted)
    {
        ActiveCursor = EMouseCursor::GrabHand;
    }
    else if (bCompleteButtonHovered && !bTaskCompleted)
    {
        ActiveCursor = EMouseCursor::Hand;
    }
    else if (IsCursorOverTitleBar())
    {
        ActiveCursor = EMouseCursor::Hand;
    }
    else
    {
        ActiveCursor = EMouseCursor::Default;
    }
}
