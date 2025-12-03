#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"
#include "Widgets/SLeafWidget.h"

class FIowaGamblingTaskProgram;

/** Custom widget to render the vendor cards and histogram */
class SVendorSandboxWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SVendorSandboxWidget)
        : _LineThickness(2.0f)
    {}
        SLATE_ATTRIBUTE(FSlateColor, PrimaryColor)
        SLATE_ATTRIBUTE(FSlateColor, DimColor)
        SLATE_ATTRIBUTE(float, LineThickness)
        SLATE_ARGUMENT(FIowaGamblingTaskProgram*, Program)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    TAttribute<FSlateColor> PrimaryColor;
    TAttribute<FSlateColor> DimColor;
    TAttribute<float> LineThickness;
    FIowaGamblingTaskProgram* Program = nullptr;
};

/** Program O: Vendor Sandbox - Iowa Gambling / Cambridge Gambling hybrid */
class ELEVATOR_API FIowaGamblingTaskProgram : public FTrialProgramBase
{
    friend class SVendorSandboxWidget;
public:
    struct FDeckOutcome
    {
        int32 Reward = 0;
        int32 Penalty = 0;
    };

        int32 StartingDraws = 0;
    FIowaGamblingTaskProgram();
protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void GenerateNewTrial() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
        TArray<float> DeckLastChange;
private:
    void SelectDeck(int32 DeckIndex);
    int32 GetDeckAtPosition(const FVector2D& Position, const FVector2D& WidgetSize) const;
    void InvalidateWidget() const;

    FString GetDeckLabel(int32 DeckIndex) const;
    FDeckOutcome ConsumeOutcome(int32 DeckIndex);
    FVector2D GetDeckCardTopLeft(int32 DeckIndex, const FVector2D& WidgetSize) const;

    // Trial state
    int32 Score = 0;
    int32 QuotaGoal = 0;
    int32 DrawsRemaining = 0;
    int32 LastReward = 0;
    int32 LastPenalty = 0;
    int32 LastNet = 0;
    int32 StartingScore = 0;
    FString StatusLine;

    TArray<TArray<FDeckOutcome>> DeckOutcomeSequences;
    TArray<int32> DeckOutcomeIndices;
    TArray<int32> DeckDrawCounts;
    TArray<int32> DeckConsecutiveCounts;
    TArray<float> DeckPositiveTotals;
    TArray<float> DeckNegativeTotals;

    int32 HoveredDeck = -1;
    TWeakPtr<SWidget> ProgramRootWidget;
    TWeakPtr<SWidget> SandboxWidget;
};
