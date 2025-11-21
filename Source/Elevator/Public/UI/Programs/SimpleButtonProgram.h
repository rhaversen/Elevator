#pragma once

#include "CoreMinimal.h"
#include "Math/Box2D.h"
#include "UI/IScreenProgram.h"

/**
 * File selection task program: open a folder and select a specific file.
 */
class ELEVATOR_API FSimpleButtonProgram : public IScreenProgram, public TSharedFromThis<FSimpleButtonProgram>
{
public:
    FSimpleButtonProgram();
    virtual ~FSimpleButtonProgram() override = default;

    virtual TSharedRef<SWidget> CreateWidget(const FVector2D& Size, const FScreenProgramStyle& Style) override;
    virtual void OnPointerMoved(const FScreenPointerEvent& Event) override;
    virtual void OnPointerPressed(const FScreenPointerEvent& Event) override;
    virtual void OnPointerReleased(const FScreenPointerEvent& Event) override;
    virtual bool IsTaskComplete() const override { return bTaskCompleted; }
    virtual void OnScreenResized(const FVector2D& NewSize) override;
    virtual EMouseCursor::Type GetCursorType() const override { return ActiveCursor; }

private:
    enum class EInteractionState
    {
        Idle,
        Dragging,
        Resizing
    };

    struct FWindow
    {
        FVector2D Position;
        FVector2D Size;
        FString Title;
    };

    // Window state getters for Slate attributes
    FVector2D GetWindowPosition() const;
    FVector2D GetWindowSize() const;
    
    // Helper to check hit against window parts
    bool IsCursorOverTitleBar() const;
    bool IsCursorOverResizeHandle() const;
    bool IsCursorOverCompleteButton() const;
    void ApplyCursorPosition(const FVector2D& NormalizedPosition);
    void UpdateCursorStyle();
    FSlateColor GetCompleteButtonBorderColor() const;
    FSlateColor GetCompleteButtonFillColor() const;
    FSlateColor GetCompleteButtonTextColor() const;

    // The program is rendered inside a bounded area defined by the host interface.
    FVector2D ProgramSize = FVector2D(1920.0f, 1080.0f);
    FVector2D CursorPosition = FVector2D(0.5f, 0.5f);

    // Window State
    FWindow Window;
    EInteractionState CurrentState = EInteractionState::Idle;
    FVector2D DragOffset; // Offset from window top-left to cursor when drag started
    FKey ActivePointerKey = EKeys::Invalid;
    EMouseCursor::Type ActiveCursor = EMouseCursor::Default;

    // Layout constants
    static constexpr float TitleBarHeight = 30.0f;
    static constexpr float ResizeHandleSize = 20.0f;
    static constexpr float MinWindowSize = 100.0f;

    // Complete button state
    bool bTaskCompleted = false;
    bool bCompleteButtonHovered = false;
    bool bCompleteButtonPressed = false;
    static constexpr float CompleteButtonWidth = 200.0f;
    static constexpr float CompleteButtonHeight = 60.0f;
};
