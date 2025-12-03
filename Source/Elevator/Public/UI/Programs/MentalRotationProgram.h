#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"
#include "Widgets/SLeafWidget.h"

class FMentalRotationProgram;

/** Drag targets for the mark alignment widget */
enum class EMarkAlignmentDragTarget : uint8
{
    None,
    RotationHandle,
    MirrorX,
    MirrorY,
    ApplyButton
};

/** Custom widget that draws the mental rotation marks */
class SMarkAlignmentWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SMarkAlignmentWidget)
        : _LineThickness(2.0f)
    {}
        SLATE_ATTRIBUTE(FSlateColor, PrimaryColor)
        SLATE_ATTRIBUTE(FSlateColor, DimColor)
        SLATE_ATTRIBUTE(float, LineThickness)
        SLATE_ARGUMENT(FMentalRotationProgram*, Program)
    SLATE_END_ARGS()
    
    void Construct(const FArguments& InArgs);
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    
private:
    TAttribute<FSlateColor> PrimaryColor;
    TAttribute<FSlateColor> DimColor;
    TAttribute<float> LineThickness;
    FMentalRotationProgram* Program = nullptr;
};

/** Program K: Mark Alignment - Mental Rotation Test */
class ELEVATOR_API FMentalRotationProgram : public FTrialProgramBase
{
    friend class SMarkAlignmentWidget;
public:
    FMentalRotationProgram();
protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
private:
    virtual void GenerateNewTrial() override;
    void ApplyAlignment();
    bool CheckMatch() const;
    void InvalidateWidget() const;
    
    // Get the center of the adjustable pane in widget-local coordinates
    FVector2D GetAdjustCenter(const FVector2D& WidgetSize) const;
    float GetMarkSize(const FVector2D& WidgetSize) const;
    float GetRingRadius(const FVector2D& WidgetSize) const;
    float AngleFromPosition(const FVector2D& Pos, const FVector2D& Center) const;
    float NormalizeAngle(float Angle) const;
    
    // Hit testing
    EMarkAlignmentDragTarget GetHitTarget(const FVector2D& LocalPos, const FVector2D& WidgetSize) const;
    
    // Mark shape type (different L-like shapes for variety)
    int32 MarkType = 0;
    
    float TargetRotation = 0.0f;    // Target angle (degrees)
    bool bTargetMirroredX = false;
    bool bTargetMirroredY = false;
    float CurrentRotation = 0.0f;   // User-adjusted angle
    bool bCurrentMirroredX = false;
    bool bCurrentMirroredY = false;
    
    // Interaction state
    EMarkAlignmentDragTarget HoveredTarget = EMarkAlignmentDragTarget::None;
    EMarkAlignmentDragTarget DragTarget = EMarkAlignmentDragTarget::None;
    FKey ActivePointerKey;
    
    // Rotation drag state
    float DragStartAngle = 0.0f;      // Angle where the drag started (cursor position)
    float DragStartRotation = 0.0f;   // Rotation value when drag started
    float VisualRotation = 0.0f;      // Smooth rotation for the ring (unsnapped)
    
    float ToleranceDegrees = 15.0f;
    
    TWeakPtr<SWidget> AlignWidget;
};
