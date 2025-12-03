#pragma once

#include "CoreMinimal.h"
#include "UI/Programs/TrialProgramBase.h"
#include "Widgets/SLeafWidget.h"

/**
 * Custom widget that draws an analog clock with draggable hand handles
 */
class ELEVATOR_API SClockWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SClockWidget)
        : _HourAngle(0.0f)
        , _MinuteAngle(0.0f)
        , _ClockColor(FLinearColor::Green)
        , _LineThickness(2.0f)
        , _HourHandleHighlighted(false)
        , _MinuteHandleHighlighted(false)
    {}
        SLATE_ATTRIBUTE(float, HourAngle)
        SLATE_ATTRIBUTE(float, MinuteAngle)
        SLATE_ATTRIBUTE(FLinearColor, ClockColor)
        SLATE_ATTRIBUTE(float, LineThickness)
        SLATE_ATTRIBUTE(bool, HourHandleHighlighted)
        SLATE_ATTRIBUTE(bool, MinuteHandleHighlighted)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
    TAttribute<float> HourAngle;
    TAttribute<float> MinuteAngle;
    TAttribute<FLinearColor> ClockColor;
    TAttribute<float> LineThickness;
    TAttribute<bool> HourHandleHighlighted;
    TAttribute<bool> MinuteHandleHighlighted;
};

/**
 * Program B: Time Drift Synchronizer
 * Psychological Test: Clock Drawing Test (Visuospatial/Executive Function)
 * 
 * Drag the clock hand handles to match the target time, then press SYNC.
 */
class ELEVATOR_API FClockDrawingTestProgram : public FTrialProgramBase
{
public:
    FClockDrawingTestProgram();
    virtual ~FClockDrawingTestProgram() override = default;

protected:
    virtual TSharedRef<SWidget> BuildTrialBody() override;
    virtual void GenerateNewTrial() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;

private:
    struct FClockState
    {
        int32 TargetHour = 0;   // 1-12
        int32 TargetMinute = 0; // 0-59
        float HourHandAngle = 0.0f;   // Current angle (degrees from 12)
        float MinuteHandAngle = 0.0f; // Current angle (degrees from 12)
    };
    
    enum class EDragTarget { None, HourHand, MinuteHand };
    
    void InitializeClock();
    void GenerateTargetTime();
    bool CheckTimeMatch() const;
    float GetAngleForHour(int32 Hour, int32 Minute) const;
    float GetAngleForMinute(int32 Minute) const;
    float NormalizeAngle(float Angle) const;
    FVector2D GetClockCenter() const;
    float GetClockRadius() const;
    FVector2D GetHourHandlePosition() const;
    FVector2D GetMinuteHandlePosition() const;
    float AngleFromPosition(const FVector2D& Pos) const;
    
    FClockState Clock;
    EDragTarget DragTarget = EDragTarget::None;
    EDragTarget HoveredHandle = EDragTarget::None;
    bool bHoveringSyncButton = false;
    bool bTargetVisible = true;
    bool bTargetHidden = false;
    FKey ActivePointerKey = EKeys::Invalid;
    float ToleranceDegrees = 15.0f;
    float HandleRadius = 12.0f; // Size of draggable handles
};
