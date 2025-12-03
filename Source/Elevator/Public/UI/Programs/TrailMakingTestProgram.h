#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"
#include "Widgets/SLeafWidget.h"

class FTrailMakingTestProgram;

enum class ERoutePhase : uint8
{
    PhaseA,
    PhaseB
};

enum class EAdvanceAction : uint8
{
    None,
    PhaseB,
    NextTrial
};

/** Custom widget that draws the trail making test with circles and lines */
class STrailWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(STrailWidget)
        : _LineThickness(2.0f)
    {}
        SLATE_ATTRIBUTE(FSlateColor, PrimaryColor)
        SLATE_ATTRIBUTE(FSlateColor, DimColor)
        SLATE_ATTRIBUTE(float, LineThickness)
        SLATE_ARGUMENT(FTrailMakingTestProgram*, Program)
    SLATE_END_ARGS()
    
    void Construct(const FArguments& InArgs);
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    
private:
    TAttribute<FSlateColor> PrimaryColor;
    TAttribute<FSlateColor> DimColor;
    TAttribute<float> LineThickness;
    FTrailMakingTestProgram* Program = nullptr;
};

/** Program H: Workflow Router - Trail Making Test (Processing Speed/Flexibility) */
class ELEVATOR_API FTrailMakingTestProgram : public FTrialProgramBase
{
    friend class STrailWidget;
public:
    FTrailMakingTestProgram();
protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
private:
    virtual void GenerateNewTrial() override;
    void GenerateNodes();
    void ResetTrial();
    void CheckNodeIntersection(const FVector2D& NormalizedPos);
    FVector2D GetNormalizedPosition(const FVector2D& PixelPosition, FVector2D* OutLocalPosition = nullptr) const;
    FText GetStatusText() const;
    void InvalidateTrail() const;
    void ScheduleAdvance(EAdvanceAction Action);
    void ProcessPendingAdvance();
    void StartFailureFade();
    void ClearFailureFade();
    float GetFailureFadeAlpha(double Now) const;
    bool HasFailureFade() const { return FailureSegments.Num() > 0; }
    
    struct FRouteNode
    {
        FString Label;
        FVector2D NormalizedPosition;  // 0-1 range within canvas
        bool bVisited = false;
    };
    
    struct FTrailSegment
    {
        FVector2D StartNormalized;
        FVector2D EndNormalized;
    };

    TArray<FRouteNode> Nodes;
    TWeakPtr<SWidget> TrailWidget;
    int32 CurrentTarget = 0;
    ERoutePhase CurrentPhase = ERoutePhase::PhaseA;
    bool bAlternating = false;
    bool bDragging = false;     // Currently dragging through nodes
    bool bPendingAdvance = false;
    EAdvanceAction PendingAdvanceAction = EAdvanceAction::None;
    double PendingAdvanceTime = 0.0;
    TArray<FTrailSegment> FailureSegments;
    double FailureFadeStartTime = 0.0;
    FVector2D CursorNormalizedPosition = FVector2D(0.5f, 0.5f);
    FVector2D CursorLocalPosition = FVector2D::ZeroVector;
    TArray<FVector2D> SerialNodePositions;  // Stored positions from serial phase for reuse in multiplex
};
