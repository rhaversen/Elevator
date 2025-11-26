#pragma once

#include "CoreMinimal.h"
#include "Math/Box2D.h"
#include "UI/ScreenProgramWindow.h"

/**
 * File selection task program: open a folder and select a specific file.
 */
class ELEVATOR_API FSimpleButtonProgram : public FWindowedScreenProgramBase
{
public:
    FSimpleButtonProgram();
    virtual ~FSimpleButtonProgram() override = default;

protected:
    virtual void BuildWindowLayout(FScreenProgramWindowBuilder& Builder) override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByChrome) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByChrome) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByChrome) override;
    virtual void HandleScreenResized(const FVector2D& NewSize) override;
    virtual void HandleTaskCompletionChanged(bool bCompleted) override;

private:
    TSharedRef<SWidget> BuildPrimaryWindowContent(const FScreenProgramWindowConfig& WindowConfig);

    bool IsCursorOverCompleteButton(const FVector2D& ProgramPixel) const;
    void UpdateCursorVisual();
    FSlateColor GetCompleteButtonBorderColor() const;
    FSlateColor GetCompleteButtonFillColor() const;
    FSlateColor GetCompleteButtonTextColor() const;

    static const FName PrimaryWindowId;

    FKey ActiveButtonPointerKey = EKeys::Invalid;
    bool bCompleteButtonHovered = false;
    bool bCompleteButtonPressed = false;
};
