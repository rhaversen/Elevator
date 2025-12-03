#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"
#include "Widgets/SLeafWidget.h"

class FSymbolDigitModalitiesProgram;

/** Custom widget that draws the code sheet with proper alignment */
class SCodeSheetWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SCodeSheetWidget)
        : _LineThickness(2.0f)
    {}
        SLATE_ATTRIBUTE(FSlateColor, PrimaryColor)
        SLATE_ATTRIBUTE(FSlateColor, DimColor)
        SLATE_ATTRIBUTE(float, LineThickness)
        SLATE_ARGUMENT(FSymbolDigitModalitiesProgram*, Program)
    SLATE_END_ARGS()
    
    void Construct(const FArguments& InArgs);
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    
private:
    TAttribute<FSlateColor> PrimaryColor;
    TAttribute<FSlateColor> DimColor;
    TAttribute<float> LineThickness;
    FSymbolDigitModalitiesProgram* Program = nullptr;
};

/** Program L: Code Sheet Entry - Symbol Digit Modalities Test */
class ELEVATOR_API FSymbolDigitModalitiesProgram : public FTrialProgramBase
{
    friend class SCodeSheetWidget;
public:
    FSymbolDigitModalitiesProgram();
protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void GenerateNewTrial() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
private:
    void EnterDigit(int32 Digit);
    int32 GetDigitButtonAtPosition(const FVector2D& LocalPos, const FVector2D& WidgetSize) const;
    void InvalidateWidget() const;
    
    TArray<FString> KeySymbols;   // The 9 symbols in the key
    int32 CurrentSymbolIndex = 0; // Which symbol is shown for current trial
    int32 HoveredDigit = -1;
    FKey ActivePointerKey = EKeys::Invalid;
    TWeakPtr<SWidget> SheetWidget;
};
