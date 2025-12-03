#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"

/** Program L: Recall Override Channel - Stop-Signal Task
 *  All trials start as GO - prepare to click SEND
 *  On some trials, a STOP signal appears after a delay - must inhibit the response
 *  The challenge is canceling an already-initiated action */
class ELEVATOR_API FStopSignalVisualProgram : public FTrialProgramBase
{
public:
    FStopSignalVisualProgram();
protected:
    // FTrialProgramBase interface
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual TSharedRef<SWidget> BuildTrialFooter() override;
    virtual void OnTrialTimerExpired() override;
    virtual void GenerateNewTrial() override;

    // FScreenProgramBase interface
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;

private:
    void ClickSend();
    bool IsStopSignalVisible() const;
    
    bool bIsStopTrial = false;        // Will this trial have a stop signal?
    float StopSignalDelay = 0.4f;     // When stop signal appears (varies)
    bool bSendHovered = false;
    FKey ActivePointerKey = EKeys::Invalid;
};
