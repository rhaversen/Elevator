#pragma once

#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"
#include "Widgets/SLeafWidget.h"

class FFeatureStroopProgram;

/** Custom canvas widget for the Spatial-Stroop + Simon task */
class SThemeConsistencyCheckWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SThemeConsistencyCheckWidget) {}
        SLATE_ARGUMENT(FFeatureStroopProgram*, Program)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    FFeatureStroopProgram* Program = nullptr;
};

/**
 * Program P: Theme Consistency Check - Spatial-Stroop + Simon Hybrid
 * 
 * Tests interference control and spatial compatibility.
 * - Stroop factor: Word vs Arrow direction
 * - Simon factor: Stimulus position vs Response side
 * 
 * Drag the card left or right to respond.
 */
class ELEVATOR_API FFeatureStroopProgram : public FTrialProgramBase
{
    friend class SThemeConsistencyCheckWidget;
public:
    FFeatureStroopProgram();

protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void GenerateNewTrial() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;

private:
    struct FStimulus
    {
        bool bIsSolid;          // true=solid arrow, false=hollow arrow
        bool bArrowIsLeft;      // true=points left, false=points right (distractor)
        float OffsetX;          // Random X offset from center
        float OffsetY;          // Random Y offset from center
    };

    void SubmitResponse(bool bIsLeftResponse);
    void InvalidateWidget() const;
    float GetDragThreshold() const;

    FStimulus CurrentStimulus;
    
    // Drag state
    bool bIsDragging = false;
    bool bCardHovered = false;
    FVector2D DragStartPos = FVector2D::ZeroVector;
    float CardDragOffset = 0.0f;  // Current horizontal offset from drag

    TWeakPtr<SThemeConsistencyCheckWidget> CanvasWidget;
};
