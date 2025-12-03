#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"
#include "Widgets/SLeafWidget.h"

class FRavensProgressiveMatricesProgram;

/** Custom widget for Template Matrix display */
class STemplateMatrixWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(STemplateMatrixWidget)
        : _LineThickness(2.0f)
    {}
        SLATE_ATTRIBUTE(FSlateColor, PrimaryColor)
        SLATE_ATTRIBUTE(FSlateColor, DimColor)
        SLATE_ATTRIBUTE(float, LineThickness)
        SLATE_ARGUMENT(FRavensProgressiveMatricesProgram*, Program)
    SLATE_END_ARGS()
    
    void Construct(const FArguments& InArgs);
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    
private:
    TAttribute<FSlateColor> PrimaryColor;
    TAttribute<FSlateColor> DimColor;
    TAttribute<float> LineThickness;
    FRavensProgressiveMatricesProgram* Program = nullptr;
};

/** Program N: Template Matrix - Raven's Progressive Matrices */
class ELEVATOR_API FRavensProgressiveMatricesProgram : public FTrialProgramBase
{
    friend class STemplateMatrixWidget;
public:
    FRavensProgressiveMatricesProgram();
protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void GenerateNewTrial() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
private:
    void SelectOption(int32 OptionIndex);
    int32 GetCandidateAtPosition(const FVector2D& LocalPos, const FVector2D& WidgetSize) const;
    void InvalidateWidget() const;
    void DrawShape(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry, 
                   int32 ShapeType, const FVector2D& Center, float Size, const FLinearColor& Color, float Thickness) const;
    
    TArray<int32> MatrixCells;     // 9 cells (3x3), -1 for blank
    TArray<int32> CandidateTiles;  // 6 candidate answers
    int32 CorrectAnswer = 0;
    int32 HoveredOption = -1;
    FKey ActivePointerKey = EKeys::Invalid;
    TWeakPtr<SWidget> MatrixWidget;
};
