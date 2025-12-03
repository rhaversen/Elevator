#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "UI/ScreenProgramBase.h"

/** Program M: Tab Restore Order - Corsi Block-Tapping Test */
class ELEVATOR_API FTabRestoreOrderProgram : public FScreenProgramBase
{
public:
    FTabRestoreOrderProgram();
protected:
    virtual TSharedRef<SWidget> BuildProgramWidget() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
private:
    enum class EPhase
    {
        ShowingSequence,
        AwaitingInput,
        Feedback
    };

    struct FTabDescriptor
    {
        FText Label;
    };

    void GenerateSequence();
    void UpdateState();
    void StartInputPhase();
    void StartFeedback(bool bSuccess, int32 ErrorBlock = -1, TOptional<FText> CustomMessage = TOptional<FText>());
    void TapBlock(int32 BlockIndex);
    int32 GetBlockAtPosition(const FVector2D& Position) const;
    FText GetStatusText() const;
    float GetResponseProgress() const;
    FSlateColor GetBlockTextColor(int32 BlockIndex) const;
    const FTabDescriptor& GetTabDescriptor(int32 BlockIndex) const;
    static const TArray<FTabDescriptor>& GetTabDescriptors();

    static constexpr int32 NumBlocks = 9;
    static constexpr float GridAnchorLeft = 0.2f;
    static constexpr float GridAnchorRight = 0.8f;
    static constexpr float GridAnchorTop = 0.35f;
    static constexpr float GridAnchorBottom = 0.75f;
    static constexpr double IntroPauseDuration = 0.5;
    static constexpr double HighlightDuration = 0.55;
    static constexpr double InterStimulusDuration = 0.25;
    static constexpr double FeedbackDuration = 1.0;
    static constexpr double ResponseTimePerItem = 1.50;

    TArray<int32> TargetSequence;
    TArray<int32> PlayerSequence;
    int32 SequenceLength = 1;
    int32 MaxLength = 5;

    EPhase Phase = EPhase::ShowingSequence;
    int32 HoveredBlock = -1;
    int32 HighlightedBlock = -1;
    int32 FeedbackBlock = -1;
    int32 ShowingIndex = -1;
    bool bFlashVisible = false;
    bool bPendingSequenceReset = false;

    double NextPhaseTime = 0.0;
    double ResponseStartTime = 0.0;
    double ResponseDuration = 0.0;

    FText StatusMessage;
    TWeakPtr<SWidget> GridWidget;
    FKey ActivePointerKey = EKeys::Invalid;
};
