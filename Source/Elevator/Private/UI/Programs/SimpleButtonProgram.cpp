#include "UI/Programs/SimpleButtonProgram.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/CoreStyle.h"

FSimpleButtonProgram::FSimpleButtonProgram()
{
    // Initialize a default window
    Window.Position = FVector2D(400.0f, 300.0f);
    Window.Size = FVector2D(600.0f, 400.0f);
    Window.Title = TEXT("My Computer");
}

namespace
{
    // Helper to create the window content
    TSharedRef<SWidget> CreateMockWindow(const FString& Title)
    {
        const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
        const FLinearColor Black = FLinearColor::Black;
        const FLinearColor Green = FLinearColor::Green;

        return SNew(SBorder)
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
                            .Text(FText::FromString(Title))
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
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("Welcome to Mock OS v0.1\n\n- Drag title bar to move\n- Drag bottom-right to resize")))
                            .ColorAndOpacity(Green)
                        ]
                    ]
                ]
            ];
    }
}

TSharedRef<SWidget> FSimpleButtonProgram::CreateWidget(const FVector2D& Size)
{
    ProgramSize = FVector2D(FMath::Max(Size.X, 1.0f), FMath::Max(Size.Y, 1.0f));
    ApplyCursorPosition(CursorPosition);

    return SNew(SCanvas)
        // The Window
        + SCanvas::Slot()
        .Position(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(this, &FSimpleButtonProgram::GetWindowPosition)))
        .Size(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(this, &FSimpleButtonProgram::GetWindowSize)))
        [
            CreateMockWindow(Window.Title)
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

    if (IsCursorOverResizeHandle())
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
    if (CurrentState == EInteractionState::Resizing || IsCursorOverResizeHandle())
    {
        ActiveCursor = EMouseCursor::ResizeSouthEast;
    }
    else if (CurrentState == EInteractionState::Dragging)
    {
        ActiveCursor = EMouseCursor::GrabHand;
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
