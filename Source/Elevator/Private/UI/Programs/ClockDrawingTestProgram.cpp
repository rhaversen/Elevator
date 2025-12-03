#include "UI/Programs/ClockDrawingTestProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// ============================================================================
// SClockWidget Implementation
// ============================================================================

void SClockWidget::Construct(const FArguments& InArgs)
{
    HourAngle = InArgs._HourAngle;
    MinuteAngle = InArgs._MinuteAngle;
    ClockColor = InArgs._ClockColor;
    LineThickness = InArgs._LineThickness;
    HourHandleHighlighted = InArgs._HourHandleHighlighted;
    MinuteHandleHighlighted = InArgs._MinuteHandleHighlighted;
}

FVector2D SClockWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return FVector2D(300.0f, 300.0f);
}

int32 SClockWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FVector2D Center = Size * 0.5f;
    const float Radius = FMath::Min(Size.X, Size.Y) * 0.45f;
    const FLinearColor Color = ClockColor.Get() * InWidgetStyle.GetColorAndOpacityTint();
    const FLinearColor DimColor = Color * 0.5f;
    const FLinearColor HighlightColor = FLinearColor::White;
    const float Thickness = LineThickness.Get();
    const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
    const float HandleSize = 12.0f;
    
    // Draw outer circle
    {
        TArray<FVector2D> CirclePoints;
        for (int32 i = 0; i <= 64; ++i)
        {
            float Angle = (static_cast<float>(i) / 64.0f) * 2.0f * PI;
            CirclePoints.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
        }
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            CirclePoints,
            DrawEffects,
            Color,
            true,
            Thickness
        );
    }
    
    // Draw hour tick marks
    for (int32 h = 1; h <= 12; ++h)
    {
        float Angle = FMath::DegreesToRadians(h * 30.0f - 90.0f);
        float InnerRadius = (h % 3 == 0) ? Radius * 0.75f : Radius * 0.85f;
        float OuterRadius = Radius * 0.95f;
        
        TArray<FVector2D> TickPoints;
        TickPoints.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * InnerRadius);
        TickPoints.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * OuterRadius);
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            TickPoints,
            DrawEffects,
            (h % 3 == 0) ? Color : DimColor,
            true,
            (h % 3 == 0) ? Thickness * 1.5f : Thickness
        );
    }
    
    // Draw hour hand (shorter, thicker) with handle outside clock
    {
        float HourRad = FMath::DegreesToRadians(HourAngle.Get() - 90.0f);
        float HourLength = Radius * 0.65f;
        float HourHandleLength = Radius * 1.15f;
        FVector2D HourEnd = Center + FVector2D(FMath::Cos(HourRad), FMath::Sin(HourRad)) * HourLength;
        FVector2D HourHandlePos = Center + FVector2D(FMath::Cos(HourRad), FMath::Sin(HourRad)) * HourHandleLength;
        
        TArray<FVector2D> HourHandPoints;
        HourHandPoints.Add(Center);
        HourHandPoints.Add(HourEnd);
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(),
            HourHandPoints,
            DrawEffects,
            Color,
            true,
            Thickness * 2.0f
        );
        
        // Hour hand handle (circle outside clock)
        bool bHourHighlighted = HourHandleHighlighted.Get();
        TArray<FVector2D> HandleCircle;
        for (int32 i = 0; i <= 16; ++i)
        {
            float Angle = (static_cast<float>(i) / 16.0f) * 2.0f * PI;
            HandleCircle.Add(HourHandlePos + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * HandleSize);
        }
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(),
            HandleCircle,
            DrawEffects,
            bHourHighlighted ? HighlightColor : Color,
            true,
            bHourHighlighted ? Thickness * 2.0f : Thickness
        );
    }
    
    // Draw minute hand (longer, thinner) with handle outside clock
    {
        float MinRad = FMath::DegreesToRadians(MinuteAngle.Get() - 90.0f);
        float MinLength = Radius * 0.85f;
        float MinHandleLength = Radius * 1.25f;
        FVector2D MinEnd = Center + FVector2D(FMath::Cos(MinRad), FMath::Sin(MinRad)) * MinLength;
        FVector2D MinHandlePos = Center + FVector2D(FMath::Cos(MinRad), FMath::Sin(MinRad)) * MinHandleLength;
        
        TArray<FVector2D> MinHandPoints;
        MinHandPoints.Add(Center);
        MinHandPoints.Add(MinEnd);
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(),
            MinHandPoints,
            DrawEffects,
            Color,
            true,
            Thickness * 1.5f
        );
        
        // Minute hand handle (circle outside clock)
        bool bMinHighlighted = MinuteHandleHighlighted.Get();
        TArray<FVector2D> HandleCircle;
        for (int32 i = 0; i <= 16; ++i)
        {
            float Angle = (static_cast<float>(i) / 16.0f) * 2.0f * PI;
            HandleCircle.Add(MinHandlePos + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * HandleSize);
        }
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(),
            HandleCircle,
            DrawEffects,
            bMinHighlighted ? HighlightColor : Color,
            true,
            bMinHighlighted ? Thickness * 2.0f : Thickness
        );
    }
    
    // Draw center dot
    {
        TArray<FVector2D> CenterDot;
        float DotRadius = 5.0f;
        for (int32 i = 0; i <= 12; ++i)
        {
            float Angle = (static_cast<float>(i) / 12.0f) * 2.0f * PI;
            CenterDot.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * DotRadius);
        }
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId + 3,
            AllottedGeometry.ToPaintGeometry(),
            CenterDot,
            DrawEffects,
            Color,
            true,
            Thickness * 2.0f
        );
    }
    
    return LayerId + 4;
}

// ============================================================================
// FClockDrawingTestProgram Implementation
// ============================================================================

REGISTER_SCREEN_PROGRAM(FClockDrawingTestProgram, "clock_drawing_test")

FClockDrawingTestProgram::FClockDrawingTestProgram()
    : FTrialProgramBase(1)
{
    SetTrialTitle(FText::FromString(TEXT("Clock Drawing Test")));
    SetTaskComplete(false);
    InitializeClock();
}

void FClockDrawingTestProgram::GenerateNewTrial()
{
    SetTaskComplete(false);
    InitializeClock();
}

void FClockDrawingTestProgram::InitializeClock()
{
    Clock = FClockState();
    GenerateTargetTime();
    
    // Set random clock positions that don't match the target
    float targetHourAngle = GetAngleForHour(Clock.TargetHour, Clock.TargetMinute);
    float targetMinuteAngle = GetAngleForMinute(Clock.TargetMinute);
    
    // Ensure hour hand is at least 60 degrees away from target
    do {
        Clock.HourHandAngle = FMath::RandRange(0, 11) * 30.0f;
    } while (FMath::Abs(Clock.HourHandAngle - targetHourAngle) < 60.0f || 
             FMath::Abs(Clock.HourHandAngle - targetHourAngle) > 300.0f);
    
    // Ensure minute hand is at least 90 degrees away from target  
    do {
        Clock.MinuteHandAngle = FMath::RandRange(0, 11) * 30.0f;
    } while (FMath::Abs(Clock.MinuteHandAngle - targetMinuteAngle) < 90.0f ||
             FMath::Abs(Clock.MinuteHandAngle - targetMinuteAngle) > 270.0f);
    
    bTargetVisible = true;
    bTargetHidden = false;
    DragTarget = EDragTarget::None;
    HoveredHandle = EDragTarget::None;
    SetTaskComplete(false);
}

void FClockDrawingTestProgram::GenerateTargetTime()
{
    Clock.TargetHour = FMath::RandRange(1, 12);
    Clock.TargetMinute = FMath::RandRange(0, 3) * 15; // 0, 15, 30, 45
}

FVector2D FClockDrawingTestProgram::GetClockCenter() const
{
    // The clock widget is 300x300, centered in the middle slot of a vertical box
    // We need to estimate where it actually renders
    // Header (title + target time) takes roughly 15% of screen
    // Footer (SYNC button) takes roughly 10% of screen
    // Clock is centered in the remaining middle area
    FVector2D Size = GetProgramSize();
    float HeaderHeight = Size.Y * 0.15f;
    float FooterHeight = Size.Y * 0.12f;
    float MiddleHeight = Size.Y - HeaderHeight - FooterHeight;
    float ClockCenterY = HeaderHeight + (MiddleHeight * 0.5f);
    return FVector2D(Size.X * 0.5f, ClockCenterY);
}

float FClockDrawingTestProgram::GetClockRadius() const
{
    // Match the SClockWidget radius calculation
    return 300.0f * 0.45f; // 300 is widget size, 0.45 is radius factor
}

FVector2D FClockDrawingTestProgram::GetHourHandlePosition() const
{
    FVector2D Center = GetClockCenter();
    float Radius = GetClockRadius();
    float HourHandleLength = Radius * 1.15f; // Outside the clock circle
    float HourRad = FMath::DegreesToRadians(Clock.HourHandAngle - 90.0f);
    return Center + FVector2D(FMath::Cos(HourRad), FMath::Sin(HourRad)) * HourHandleLength;
}

FVector2D FClockDrawingTestProgram::GetMinuteHandlePosition() const
{
    FVector2D Center = GetClockCenter();
    float Radius = GetClockRadius();
    float MinHandleLength = Radius * 1.25f; // Outside the clock circle
    float MinRad = FMath::DegreesToRadians(Clock.MinuteHandAngle - 90.0f);
    return Center + FVector2D(FMath::Cos(MinRad), FMath::Sin(MinRad)) * MinHandleLength;
}

float FClockDrawingTestProgram::AngleFromPosition(const FVector2D& Pos) const
{
    FVector2D Center = GetClockCenter();
    FVector2D Dir = Pos - Center;
    // Convert to angle where 0 = 12 o'clock, clockwise positive
    float Angle = FMath::RadiansToDegrees(FMath::Atan2(Dir.X, -Dir.Y));
    return NormalizeAngle(Angle);
}

TSharedRef<SWidget> FClockDrawingTestProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(Style.GetSmallPadding()).HAlign(HAlign_Center)
        [SNew(STextBlock).Text_Lambda([this]() { 
            return FText::FromString(FString::Printf(TEXT("%d:%02d"), Clock.TargetHour, Clock.TargetMinute));
        }).Font(FCoreStyle::GetDefaultFontStyle("Bold", GetProgramStyle().TextSize * 3))
          .ColorAndOpacity(Style.GetPrimaryColor())]
        + SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SClockWidget)
            .HourAngle_Lambda([this]() { return Clock.HourHandAngle; })
            .MinuteAngle_Lambda([this]() { return Clock.MinuteHandAngle; })
            .ClockColor(Style.GetPrimaryColor())
            .LineThickness(Style.LineThickness)
            .HourHandleHighlighted_Lambda([this]() { return HoveredHandle == EDragTarget::HourHand || DragTarget == EDragTarget::HourHand; })
            .MinuteHandleHighlighted_Lambda([this]() { return HoveredHandle == EDragTarget::MinuteHand || DragTarget == EDragTarget::MinuteHand; })]
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(Style.GetSmallPadding())
        [SNew(SBorder)
            .BorderBackgroundColor_Lambda([this]() {
                return (bHoveringSyncButton || CheckTimeMatch()) ? GetProgramStyle().GetPrimaryColor() : GetProgramStyle().GetDimColor();
            })
            .Padding(GetProgramStyle().GetSmallPadding() * 2)
            [SNew(STextBlock).Text(FText::FromString(TEXT("    SYNC    ")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", Style.TextSize * 2))
                .ColorAndOpacity(Style.GetPrimaryColor())]];
}

void FClockDrawingTestProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor) return;
    
    const FVector2D& Pos = Event.ProgramPixelPosition;
    const FVector2D Size = GetProgramSize();
    
    // If dragging, update the appropriate hand angle
    if (DragTarget != EDragTarget::None && ActivePointerKey != EKeys::Invalid)
    {
        float NewAngle = AngleFromPosition(Pos);
        
        if (DragTarget == EDragTarget::HourHand)
        {
            Clock.HourHandAngle = NewAngle;
        }
        else if (DragTarget == EDragTarget::MinuteHand)
        {
            Clock.MinuteHandAngle = NewAngle;
        }
        return;
    }
    
    // Check if hovering over handles
    HoveredHandle = EDragTarget::None;
    bHoveringSyncButton = false;
    
    FVector2D HourHandlePos = GetHourHandlePosition();
    FVector2D MinuteHandlePos = GetMinuteHandlePosition();
    
    float DistToHour = FVector2D::Distance(Pos, HourHandlePos);
    float DistToMinute = FVector2D::Distance(Pos, MinuteHandlePos);
    
    // Check minute handle first (it's on top/longer)
    if (DistToMinute <= HandleRadius * 1.5f)
    {
        HoveredHandle = EDragTarget::MinuteHand;
    }
    else if (DistToHour <= HandleRadius * 1.5f)
    {
        HoveredHandle = EDragTarget::HourHand;
    }
    
    // Check SYNC button (bottom area)
    float ButtonAreaTop = Size.Y * 0.85f;
    float ButtonWidth = Size.X * 0.4f;
    float ButtonLeft = (Size.X - ButtonWidth) * 0.5f;
    
    if (Pos.Y > ButtonAreaTop && Pos.X >= ButtonLeft && Pos.X <= ButtonLeft + ButtonWidth)
    {
        bHoveringSyncButton = true;
    }
    
    UpdateCursorForState(false, HoveredHandle != EDragTarget::None || bHoveringSyncButton);
}

void FClockDrawingTestProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor) return;
    ActivePointerKey = Event.TriggerKey;
    
    // Hide target on first interaction
    if (bTargetVisible && !bTargetHidden)
    {
        bTargetHidden = true;
    }
    
    // Check if clicking SYNC button
    if (bHoveringSyncButton)
    {
        if (CheckTimeMatch())
        {
            if (!IsTaskComplete())
            {
                AdvanceTrial();
            }
        }
        else
        {
            // Wrong - reset with new target and clock positions
            InitializeClock();
        }
        return;
    }
    
    // Start dragging if on a handle
    if (HoveredHandle != EDragTarget::None)
    {
        DragTarget = HoveredHandle;
    }
}

void FClockDrawingTestProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    ActivePointerKey = EKeys::Invalid;
    DragTarget = EDragTarget::None;
}

bool FClockDrawingTestProgram::CheckTimeMatch() const
{
    float targetHourAngle = GetAngleForHour(Clock.TargetHour, Clock.TargetMinute);
    float targetMinuteAngle = GetAngleForMinute(Clock.TargetMinute);
    float hourDiff = FMath::Abs(NormalizeAngle(Clock.HourHandAngle) - NormalizeAngle(targetHourAngle));
    if (hourDiff > 180.0f) hourDiff = 360.0f - hourDiff;
    float minDiff = FMath::Abs(NormalizeAngle(Clock.MinuteHandAngle) - NormalizeAngle(targetMinuteAngle));
    if (minDiff > 180.0f) minDiff = 360.0f - minDiff;
    return hourDiff <= ToleranceDegrees && minDiff <= ToleranceDegrees;
}

float FClockDrawingTestProgram::GetAngleForHour(int32 Hour, int32 Minute) const
{
    return (Hour % 12) * 30.0f + Minute * 0.5f;
}

float FClockDrawingTestProgram::GetAngleForMinute(int32 Minute) const
{
    return Minute * 6.0f;
}

float FClockDrawingTestProgram::NormalizeAngle(float Angle) const
{
    while (Angle < 0.0f) Angle += 360.0f;
    while (Angle >= 360.0f) Angle -= 360.0f;
    return Angle;
}
