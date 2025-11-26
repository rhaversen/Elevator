#pragma once

#include "CoreMinimal.h"
#include "Math/Box2D.h"
#include "UI/IScreenProgram.h"

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
};
