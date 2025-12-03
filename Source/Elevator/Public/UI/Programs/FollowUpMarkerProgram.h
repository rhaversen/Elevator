#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"

class FFollowUpMarkerProgram;

/** Custom canvas widget for Follow-Up Marker program */
class SFollowUpMarkerWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SFollowUpMarkerWidget) {}
        SLATE_ARGUMENT(FFollowUpMarkerProgram*, Program)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
        const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    FFollowUpMarkerProgram* Program = nullptr;
};

/** Program F: Follow-Up Marker - N-Back Task
 *  
 *  Conveyor: [n+1] [n+0] [n-1] [n-2] moving right
 *  - n+1, n+0 show their code (visible)
 *  - n-1, n-2 show "?" (hidden)
 *  - Candidate code shown separately, must match n-2
 *  - MATCH button above n-2 slot
 *  
 *  Scoring:
 *  - Click MATCH when candidate == n-2: progress +1 trial
 *  - Click MATCH when candidate != n-2: back -1 trial  
 *  - Don't click: no change
 */
class ELEVATOR_API FFollowUpMarkerProgram : public FTrialProgramBase
{
    friend class SFollowUpMarkerWidget;
public:
    FFollowUpMarkerProgram();
protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void GenerateNewTrial() override;
    virtual void OnTick(float DeltaTime) override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
private:
    void GenerateSequence();
    void GenerateNextItem();
    void AdvanceSequence();
    void InvalidateWidget() const;
    
    // Slot positions (0=n+1/left, 1=n+0, 2=n-1, 3=n-2/right)
    FVector2D GetSlotPosition(int32 SlotIndex, float AnimProgress) const;
    FVector2D GetSlotSize() const;
    FVector2D GetMatchButtonCenter() const;
    FVector2D GetMatchButtonSize() const;
    bool IsPointInRect(const FVector2D& Point, const FVector2D& Center, const FVector2D& Size) const;
    
    // Sequence data
    TArray<FString> Sequence;
    int32 TrialIndex = 0;           // Current trial (n)
    static constexpr int32 TrialsRequired = 10;      // Complete after this many successful trials
    int32 MatchesGenerated = 0;     // Number of n-back matches injected into sequence
    int32 NonMatchesGenerated = 0;  // Number of non-matching entries generated
    
    // Animation
    double TrialStartTime = 0.0;
    static constexpr double TimePerTrial = 3.0;
    float GetAnimProgress() const;  // 0.0 = just started, 1.0 = fully settled
    float GetTransitionAlpha() const;  // 0 = showing text, 1 = showing ?
    
    // UI state
    bool bMatchHovered = false;
    bool bMatchPressed = false;
    bool bAlreadyResponded = false; // Prevent multiple clicks per trial
    bool bShowingFeedback = false;  // True when showing match result
    bool bLastMatchCorrect = false; // Was the last match correct?
    bool bAdvanceTrialQueued = false;
    bool bFailTrialQueued = false;
    
    FKey ActivePointerKey = EKeys::Invalid;
    TWeakPtr<SFollowUpMarkerWidget> CanvasWidget;
};
