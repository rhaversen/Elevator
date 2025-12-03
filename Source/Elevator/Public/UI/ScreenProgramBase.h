#pragma once

#include "CoreMinimal.h"
#include "Math/Box2D.h"
#include "UI/IScreenProgram.h"
#include "Types/SlateEnums.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/SWidget.h"

class STextBlock;

/**
 * Base class for screen programs that handles common book-keeping such as style/state management
 * while delegating actual UI construction to subclasses.
 */
class ELEVATOR_API FScreenProgramBase : public IScreenProgram, public TSharedFromThis<FScreenProgramBase>
{
public:
    virtual ~FScreenProgramBase() override = default;

    // IScreenProgram interface
    virtual TSharedRef<SWidget> CreateWidget(const FVector2D& Size, const FScreenProgramStyle& Style) override final;
    virtual void OnPointerMoved(const FScreenPointerEvent& Event) override final;
    virtual void OnPointerPressed(const FScreenPointerEvent& Event) override final;
    virtual void OnPointerReleased(const FScreenPointerEvent& Event) override final;
    virtual void OnScreenResized(const FVector2D& NewSize) override final;
    virtual EMouseCursor::Type GetCursorType() const override;
    virtual bool IsTaskComplete() const override { return bTaskComplete; }

protected:
    FScreenProgramBase();

    /** Build the program's root widget. Called every time the program is initialized. */
    virtual TSharedRef<SWidget> BuildProgramWidget() = 0;

    /** Called every frame to update the program. */
    virtual void OnTick(float DeltaTime);

    /** Optional pre-processing hook for pointer moves. Return true if the event was fully handled. */
    virtual bool PreHandlePointerMoved(const FScreenPointerEvent& Event);
    /** Optional pre-processing hook for pointer presses. Return true if the event was fully handled. */
    virtual bool PreHandlePointerPressed(const FScreenPointerEvent& Event);
    /** Optional pre-processing hook for pointer releases. Return true if the event was fully handled. */
    virtual bool PreHandlePointerReleased(const FScreenPointerEvent& Event);

    /** Derived classes override these to implement custom pointer handling. */
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor);
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor);
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor);
    virtual void HandleScreenResized(const FVector2D& NewSize);
    virtual void HandleTaskCompletionChanged(bool bCompleted);

    void SetTaskComplete(bool bCompleted);
    bool GetTaskComplete() const { return bTaskComplete; }

    void SetCursorOverride(TOptional<EMouseCursor::Type> CursorOverride);
    void ClearCursorOverride() { CursorOverride.Reset(); }
    void SetBaseCursor(EMouseCursor::Type Cursor) { BaseCursor = Cursor; }
    EMouseCursor::Type GetBaseCursor() const { return BaseCursor; }

    // -------------------------------------------------------------------------
    // Progress Bar Utilities
    // -------------------------------------------------------------------------

    /**
     * Build a progress bar string using filled/empty squares.
     * @param Current Current progress value
     * @param Total Total number of steps
     * @return String like "■■■□□□□" showing progress
     */
    static FString BuildProgressBarString(int32 Current, int32 Total);

    /**
     * Build a progress bar widget showing current/total progress.
     * Uses GetProgramStyle() for font and color.
     * @param CurrentGetter Lambda returning current progress value
     * @param Total Total number of steps
     */
    TSharedRef<SWidget> BuildProgressBarWidget(TFunction<int32()> CurrentGetter, int32 Total) const;

    /**
     * Build a timer bar string using pipes and dots (e.g., "||||......")
     * @param Progress Progress value from 0.0 to 1.0
     * @param Width Number of characters in the bar (default 10)
     * @return String like "||||......" showing elapsed time
     */
    static FString BuildTimerBarString(float Progress, int32 Width = 10);

    /**
     * Build a timer bar widget showing elapsed progress.
     * @param ProgressGetter Lambda returning progress value (0.0 to 1.0)
     * @param ColorGetter Optional lambda returning color (defaults to primary color)
     * @param Width Number of characters in the bar (default 10)
     */
    TSharedRef<SWidget> BuildTimerBarWidget(TFunction<float()> ProgressGetter, TFunction<FSlateColor()> ColorGetter = nullptr, int32 Width = 10) const;

    // -------------------------------------------------------------------------
    // Typography Utilities - Consistent font creation across programs
    // -------------------------------------------------------------------------

    /** Build a Slate font using the program style as a base size. */
    FSlateFontInfo MakeFont(const FString& Typeface, int32 Size) const;

    /** Build a Slate font using a multiplier applied to Style.TextSize. */
    FSlateFontInfo MakeScaledFont(const FString& Typeface, float SizeMultiplier) const;

    /** Convenience for building a bold, centered title label. */
    TSharedRef<STextBlock> BuildTitleWidget(const FText& TitleText, float SizeMultiplier = 2.0f) const;

    /** Convenience for building a section header label. */
    TSharedRef<STextBlock> BuildSectionLabel(const FText& LabelText, float SizeMultiplier = 1.0f) const;

    /** General purpose styled text helper. */
    TSharedRef<STextBlock> BuildStyledText(const FText& Text,
        const FString& Typeface,
        float SizeMultiplier,
        TFunction<FSlateColor()> ColorGetter = nullptr,
        TOptional<ETextJustify::Type> Justification = TOptional<ETextJustify::Type>()) const;

    /** Standardized header block with centered title. */
    TSharedRef<SWidget> BuildHeader(const FText& Title) const;

    /** Standardized button styling helper. */
    TSharedRef<SWidget> BuildButton(const FText& Label, TAttribute<bool> IsHovered, TAttribute<bool> IsPressed) const;

    /** Card-style container with consistent padding and border. */
    TSharedRef<SWidget> BuildCard(TSharedRef<SWidget> Content) const;

    // -------------------------------------------------------------------------
    // Cursor Convenience Methods - Standard patterns for hover/drag states
    // -------------------------------------------------------------------------

    /** Set cursor to Hand (for hoverable elements) */
    void SetCursorHand() { SetCursorOverride(EMouseCursor::Hand); }

    /** Set cursor to GrabHand (for dragging) */
    void SetCursorGrabbing() { SetCursorOverride(EMouseCursor::GrabHand); }

    /** Set cursor to Crosshairs (for marquee selection) */
    void SetCursorCrosshairs() { SetCursorOverride(EMouseCursor::Crosshairs); }

    /** Reset cursor to default */
    void ResetCursor() { CursorOverride.Reset(); }

    /**
     * Standard cursor update pattern for hover/drag states.
     * @param bDragging True if currently dragging
     * @param bHovering True if hovering over an interactive element
     */
    void UpdateCursorForState(bool bDragging, bool bHovering)
    {
        if (GetTaskComplete())
        {
            ResetCursor();
            return;
        }

        if (bDragging)
        {
            SetCursorGrabbing();
        }
        else if (bHovering)
        {
            SetCursorHand();
        }
        else
        {
            ResetCursor();
        }
    }

    // -------------------------------------------------------------------------
    // Hit Testing Utilities
    // -------------------------------------------------------------------------

    /** Check if a point is inside a rectangle defined by position and size */
    static bool IsPointInRect(const FVector2D& Point, const FVector2D& RectPos, const FVector2D& RectSize)
    {
        return Point.X >= RectPos.X && Point.X <= RectPos.X + RectSize.X &&
               Point.Y >= RectPos.Y && Point.Y <= RectPos.Y + RectSize.Y;
    }

    /** Check if a point is inside a FBox2D */
    static bool IsPointInBox(const FVector2D& Point, const FBox2D& Box)
    {
        return Box.IsInside(Point);
    }

    const FVector2D& GetProgramSize() const { return ProgramSize; }
    const FScreenProgramStyle& GetProgramStyle() const { return ActiveStyle; }
    FVector2D GetLastPointerPixel() const { return LastPointerPixel; }
    FVector2D GetLastPointerNormalized() const { return LastPointerNormalized; }

private:
    TSharedPtr<SWidget> RootWidget;
    FVector2D ProgramSize = FVector2D::ZeroVector;
    FScreenProgramStyle ActiveStyle;
    TOptional<EMouseCursor::Type> CursorOverride;
    bool bTaskComplete = false;
    EMouseCursor::Type BaseCursor = EMouseCursor::Default;
    FVector2D LastPointerNormalized = FVector2D(0.5f, 0.5f);
    FVector2D LastPointerPixel = FVector2D::ZeroVector;

    EActiveTimerReturnType HandleTick(double InCurrentTime, float InDeltaTime);
};
