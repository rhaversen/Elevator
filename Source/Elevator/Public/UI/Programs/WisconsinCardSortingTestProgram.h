#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"
#include "Widgets/SLeafWidget.h"

class FWisconsinCardSortingTestProgram;

/** Custom widget that draws the card sorting interface with shapes */
class SPolicyCardWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SPolicyCardWidget)
        : _LineThickness(2.0f)
    {}
        SLATE_ATTRIBUTE(FSlateColor, PrimaryColor)
        SLATE_ATTRIBUTE(FSlateColor, DimColor)
        SLATE_ATTRIBUTE(float, LineThickness)
        SLATE_ARGUMENT(FWisconsinCardSortingTestProgram*, Program)
    SLATE_END_ARGS()
    
    void Construct(const FArguments& InArgs);
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    
private:
    TAttribute<FSlateColor> PrimaryColor;
    TAttribute<FSlateColor> DimColor;
    TAttribute<float> LineThickness;
    FWisconsinCardSortingTestProgram* Program = nullptr;
};

/** Program I: Policy Schema Update - Wisconsin Card Sort Test */
class ELEVATOR_API FWisconsinCardSortingTestProgram : public FTrialProgramBase
{
    friend class SPolicyCardWidget;
public:
    FWisconsinCardSortingTestProgram();
protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
private:
    virtual void GenerateNewTrial() override;
    void GenerateCard();
    void DropOnPile(int32 PileIndex);
    int32 GetPileAtPosition(const FVector2D& NormalizedPos) const;
    bool IsCorrectPile(int32 PileIndex) const;
    void InvalidateWidget() const;
    void ProcessPendingAdvance();
    
    enum class ESortRule { Shape, Fill, Number };
    struct FCard { int32 Shape; int32 Fill; int32 Number; };  // Shape: 0-3, Fill: 0-2, Number: 1-4
    
    // Exemplar cards for each pile (fixed)
    struct FExemplarCard { int32 Shape; int32 Fill; int32 Number; };
    TArray<FExemplarCard> ExemplarCards;
    
    FCard CurrentCard;
    ESortRule CurrentRule = ESortRule::Shape;
    ESortRule PendingNextRule = ESortRule::Shape;
    int32 CorrectStreak = 0;
    int32 HoveredPile = -1;
    bool bReadjusting = false;
    double PendingAdvanceTime = 0.0;
    TWeakPtr<SWidget> CardWidget;
};
