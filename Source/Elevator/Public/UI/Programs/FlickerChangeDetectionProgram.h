#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"
#include "Widgets/SLeafWidget.h"

class FFlickerChangeDetectionProgram;

/** Custom widget that draws the A/B flicker grid */
class SProofDiffWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SProofDiffWidget)
        : _LineThickness(2.0f)
    {}
        SLATE_ATTRIBUTE(FSlateColor, PrimaryColor)
        SLATE_ATTRIBUTE(FSlateColor, DimColor)
        SLATE_ATTRIBUTE(float, LineThickness)
        SLATE_ARGUMENT(FFlickerChangeDetectionProgram*, Program)
    SLATE_END_ARGS()
    
    void Construct(const FArguments& InArgs);
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    
private:
    TAttribute<FSlateColor> PrimaryColor;
    TAttribute<FSlateColor> DimColor;
    TAttribute<float> LineThickness;
    FFlickerChangeDetectionProgram* Program = nullptr;
};

/** Program J: Proof Diff Viewer - Flicker/Change Blindness Test */
class ELEVATOR_API FFlickerChangeDetectionProgram : public FTrialProgramBase
{
    friend class SProofDiffWidget;
public:
    FFlickerChangeDetectionProgram();
protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
private:
    virtual void GenerateNewTrial() override;
    void GenerateDifference();
    int32 GetCellAtPosition(const FVector2D& NormalizedPos) const;
    void InvalidateWidget() const;
    void UpdateFlicker();
    double GetBlankDuration(bool bBeforeB) const;
    
    static constexpr int32 GridCols = 6;
    static constexpr int32 GridRows = 5;
    static constexpr int32 GridSize = GridCols * GridRows;
    
    // Content for each cell
    struct FCellContent
    {
        int32 BaseSymbol;   // Index into symbol set (0-5)
        int32 AltSymbol;    // Alternative symbol for version B (-1 if same)
    };
    TArray<FCellContent> CellContents;
    
    int32 DifferenceIndex = 0;
    int32 PreviousDifferenceIndex = -1;
    int32 HoveredCell = -1;
    int32 SelectedCell = -1;
    
    // Flicker state: A -> Blank -> B -> Blank -> A ...
    enum class EViewState { ShowingA, BlankBeforeB, ShowingB, BlankBeforeA };
    EViewState ViewState = EViewState::ShowingA;
    double LastStateChangeTime = 0.0;
    double ViewDurationA = 0.6; // How long version A stays visible
    double ViewDurationB = 0.5; // How long version B stays visible
    double BlankDurationBeforeB = 0.22; // Pause between A and B
    double BlankDurationBeforeA = 0.14; // Pause between B and A
    
    TWeakPtr<SWidget> DiffWidget;
};
