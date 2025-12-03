#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"

/** Program K: Proxy Handshake Verifier - Eriksen Flanker Task */
class ELEVATOR_API FEriksenFlankerTaskProgram : public FTrialProgramBase
{
public:
    FEriksenFlankerTaskProgram();
protected:
    // FTrialProgramBase interface
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual TSharedRef<SWidget> BuildTrialFooter() override;
    virtual void GenerateNewTrial() override;

    // FScreenProgramBase interface
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;

private:
    void SelectResponse(bool bLeft);
    bool bTargetLeft = true;  // Target arrow direction
    bool bCongruent = true;   // Flankers match target
    int32 TargetPosition = 2; // Which chevron is the target (0-4)
    int32 HoveredSocket = -1; // -1 = none, 0 = left, 1 = right
    FKey ActivePointerKey = EKeys::Invalid;
};
