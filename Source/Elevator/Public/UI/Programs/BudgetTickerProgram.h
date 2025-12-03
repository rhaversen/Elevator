#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"
#include "Widgets/SLeafWidget.h"

class FBudgetTickerProgram;

/** Custom widget for Budget Ticker display */
class SBudgetTickerWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SBudgetTickerWidget)
        : _LineThickness(2.0f)
    {}
        SLATE_ATTRIBUTE(FSlateColor, PrimaryColor)
        SLATE_ATTRIBUTE(FSlateColor, DimColor)
        SLATE_ATTRIBUTE(float, LineThickness)
        SLATE_ARGUMENT(FBudgetTickerProgram*, Program)
    SLATE_END_ARGS()
    
    void Construct(const FArguments& InArgs);
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    
private:
    TAttribute<FSlateColor> PrimaryColor;
    TAttribute<FSlateColor> DimColor;
    TAttribute<float> LineThickness;
    FBudgetTickerProgram* Program = nullptr;
};

/** Program M: Budget Ticker - PASAT (Working Memory) */
class ELEVATOR_API FBudgetTickerProgram : public FTrialProgramBase
{
    friend class SBudgetTickerWidget;
public:
    FBudgetTickerProgram();
protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void GenerateNewTrial() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
private:
    void EnterSum(int32 Sum);
    int32 GetSumButtonAtPosition(const FVector2D& LocalPos, const FVector2D& WidgetSize) const;
    void InvalidateWidget() const;
    void AdvanceToNextNumber();  // Shifts history and adds new number
    
    TArray<int32> NumberHistory;  // Full history of numbers on the ticker (rightmost is newest)
    TArray<int32> SumOptions;     // 5 answer options
    int32 HoveredOptionIndex = -1;
    FKey ActivePointerKey = EKeys::Invalid;
    TWeakPtr<SWidget> TickerWidget;
};
