#pragma once
#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"
#include "Widgets/SLeafWidget.h"

class FReyOsterriethCopyProgram;

/** Custom canvas widget for Directory Index Rebuild */
class SDirectoryIndexRebuildWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SDirectoryIndexRebuildWidget) {}
        SLATE_ARGUMENT(FReyOsterriethCopyProgram*, Program)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    FReyOsterriethCopyProgram* Program = nullptr;
};

/** Program Q: Directory Index Rebuild - Rey-Osterrieth Complex Figure Test */
class ELEVATOR_API FReyOsterriethCopyProgram : public FTrialProgramBase
{
    friend class SDirectoryIndexRebuildWidget;
public:
    FReyOsterriethCopyProgram();
protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void GenerateNewTrial() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
private:
    void CheckAndApply();
    void InvalidateWidget() const;
    float GetPreviewFadeAlpha(double Now) const;
    bool IsPreviewFadeActive(double Now) const;
    float GetErrorFlashAlpha(double Now) const;
    
    // Grid layout constants
    static constexpr int32 GridCols = 3;
    static constexpr int32 GridRows = 3;
    static constexpr float CellSize = 140.0f;
    static constexpr float ApplyButtonWidth = 150.0f;
    static constexpr float ApplyButtonHeight = 50.0f;
    static constexpr double PreviewFadeDurationSeconds = 0.35;
    static constexpr double ErrorFlashDurationSeconds = 0.4;
    
    // Layout helpers
    struct FCanvasLayout
    {
        FVector2D GridOrigin = FVector2D::ZeroVector;
        FVector2D PaletteOrigin = FVector2D::ZeroVector;
        FVector2D ApplyButtonCenter = FVector2D::ZeroVector;
        float GridWidth = 0.0f;
        float GridHeight = 0.0f;
        float PaletteSpacing = 0.0f;
    };

    FCanvasLayout BuildCanvasLayout(const FVector2D& CanvasSize) const;
    int32 FindGridCellAtPosition(const FVector2D& LocalPosition, const FCanvasLayout& Layout) const;
    int32 FindPaletteItemAtPosition(const FVector2D& LocalPosition, const FCanvasLayout& Layout) const;
    bool IsOverApplyButton(const FVector2D& LocalPosition, const FCanvasLayout& Layout) const;
    bool IsApplyButtonClickable() const;
    
    // Pattern element
    struct FPatternElement
    {
        int32 GridX;
        int32 GridY;
        int32 Type; // 0=rect, 1=line, 2=triangle, 3=circle
    };
    
    // Target pattern
    TArray<FPatternElement> TargetPattern;
    
    // User placements (grid cell -> primitive type, -1 if empty)
    TArray<int32> UserGrid;
    
    bool bShowingTarget = true;
    double PreviewFadeStartTime = 0.0; // Seconds timestamp when fade begins
    double ErrorFlashStartTime = 0.0;  // Timestamp for wrong-answer flash
    int32 SelectedPrimitive = -1;  // Currently grabbed shape from palette
    int32 HoveredPrimitive = -1;
    int32 HoveredGridCell = -1;
    bool bHoveringApply = false;
    
    TWeakPtr<SDirectoryIndexRebuildWidget> CanvasWidget;
};
