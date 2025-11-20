#pragma once

#include "CoreMinimal.h"
#include "UI/IScreenProgram.h"

/**
 * Simple program with a task button and an exit button.
 * Task can be completed by clicking the task button. Exit is always allowed.
 */
class ELEVATOR_API FSimpleButtonProgram : public IScreenProgram, public TSharedFromThis<FSimpleButtonProgram>
{
public:
    FSimpleButtonProgram();
    virtual ~FSimpleButtonProgram() override = default;

    virtual TSharedRef<SWidget> CreateWidget(const FVector2D& Size) override;
    virtual void UpdateCursor(const FVector2D& NormalizedPosition) override;
    virtual bool HandleClick() override;
    virtual bool IsTaskComplete() const override { return bTaskComplete; }
    virtual bool ShouldExit() const override 
    { 
        if (bExitRequested)
        {
            bExitRequested = false;
            return true;
        }
        return false;
    }
    virtual void OnScreenResized(const FVector2D& NewSize) override;

protected:
    void RequestExit() { bExitRequested = true; }

private:
    FSlateColor GetTaskButtonColor() const;
    FSlateColor GetExitButtonColor() const;
    FText GetTaskStatusText() const;

    // Fixed screen size for the program.
    // This is acceptable because the program is rendered to a Render Target
    // which maintains a constant resolution regardless of the world-space display size.
    FVector2D ScreenSize = FVector2D(1920.0f, 1080.0f);
    FVector2D CursorPosition = FVector2D(0.5f, 0.5f);
    
    bool bTaskComplete = false;
    mutable bool bExitRequested = false;
    bool bTaskButtonHovered = false;
    bool bExitButtonHovered = false;

    // Button layout constants
    static constexpr float TaskButtonWidth = 200.0f;
    static constexpr float TaskButtonHeight = 100.0f;
    static constexpr float ExitButtonWidth = 150.0f;
    static constexpr float ExitButtonHeight = 80.0f;
    static constexpr float ButtonSpacing = 50.0f;

    void GetButtonRects(const FVector2D& InScreenSize, FBox2D& OutTaskRect, FBox2D& OutExitRect) const;
};
