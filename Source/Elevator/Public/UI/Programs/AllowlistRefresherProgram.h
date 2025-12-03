#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"

/** Program D: Allowlist Refresher - Go/No-Go Task
 *  Benign symbols: Click APPROVE quickly before timer expires
 *  Hazard symbols: Do NOT click - wait for timer to advance */
class ELEVATOR_API FAllowlistRefresherProgram : public FTrialProgramBase
{
public:
    FAllowlistRefresherProgram();
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
    void Approve();
    FString GetCurrentIcon() const;
    
    bool bCurrentTrialIsBenign = true;
    bool bTrialApproved = false;  // Track if user approved this trial (wait for timer)
    bool bApproveHovered = false;
    bool bApprovePressed = false;
    FKey ActivePointerKey = EKeys::Invalid;
};
