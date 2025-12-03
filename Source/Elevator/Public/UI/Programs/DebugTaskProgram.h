#pragma once

#include "CoreMinimal.h"
#include "UI/ScreenProgramBase.h"

/**
 * Simple debug program with just a button to complete the task.
 * Useful for testing day transitions and other systems.
 */
class ELEVATOR_API FDebugTaskProgram : public FScreenProgramBase
{
public:
    FDebugTaskProgram();
    virtual ~FDebugTaskProgram() override = default;

protected:
    virtual TSharedRef<SWidget> BuildProgramWidget() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;

private:
    bool bButtonHovered = false;
    bool bButtonPressed = false;
    FKey ActivePointerKey = EKeys::Invalid;
};
