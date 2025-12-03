#include "UI/Programs/MarkAlignmentProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MarkAlignmentProgram"

REGISTER_SCREEN_PROGRAM(FMarkAlignmentProgram, "MarkAlignment")

namespace
{
    // Draw an L-shaped mark with rotation and optional X/Y mirroring
    // Order of operations: First rotate, then mirror the result
    void DrawMark(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
        const FVector2D& Center, float Size, float RotationDegrees, bool bMirroredX, bool bMirroredY, int32 MarkType,
        const FLinearColor& Color, float Thickness)
    {
        const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
        
        const float Angle = FMath::DegreesToRadians(RotationDegrees);
        const float Cos = FMath::Cos(Angle);
        const float Sin = FMath::Sin(Angle);
        
        // Helper to transform a point: rotate first, then mirror the result
        auto TransformPoint = [&](FVector2D Pt) -> FVector2D
        {
            // First rotate
            FVector2D Rotated(Pt.X * Cos - Pt.Y * Sin, Pt.X * Sin + Pt.Y * Cos);
            // Then mirror the rotated result
            if (bMirroredX) Rotated.X = -Rotated.X;
            if (bMirroredY) Rotated.Y = -Rotated.Y;
            return Center + Rotated;
        };
        
        const float Arm = Size * 0.4f;
        const float Stub = Size * 0.2f;
        
        // Different mark types for variety
        TArray<TArray<FVector2D>> Segments;
        
        switch (MarkType % 3)
        {
        case 0: // Basic L shape with stub
            {
                TArray<FVector2D> Seg1;
                Seg1.Add(TransformPoint(FVector2D(0, -Arm)));
                Seg1.Add(TransformPoint(FVector2D(0, Arm)));
                Segments.Add(Seg1);
                TArray<FVector2D> Seg2;
                Seg2.Add(TransformPoint(FVector2D(0, Arm)));
                Seg2.Add(TransformPoint(FVector2D(Arm, Arm)));
                Segments.Add(Seg2);
                TArray<FVector2D> Seg3;
                Seg3.Add(TransformPoint(FVector2D(0, -Arm)));
                Seg3.Add(TransformPoint(FVector2D(Stub, -Arm)));
                Segments.Add(Seg3);
            }
            break;
        case 1: // T with extended arm
            {
                TArray<FVector2D> Seg1;
                Seg1.Add(TransformPoint(FVector2D(0, 0)));
                Seg1.Add(TransformPoint(FVector2D(0, Arm)));
                Segments.Add(Seg1);
                TArray<FVector2D> Seg2;
                Seg2.Add(TransformPoint(FVector2D(-Arm * 0.7f, 0)));
                Seg2.Add(TransformPoint(FVector2D(Arm * 0.3f, 0)));
                Segments.Add(Seg2);
                TArray<FVector2D> Seg3;
                Seg3.Add(TransformPoint(FVector2D(0, Arm)));
                Seg3.Add(TransformPoint(FVector2D(Arm * 0.6f, Arm)));
                Segments.Add(Seg3);
            }
            break;
        case 2: // Arrow-like shape
        default:
            {
                TArray<FVector2D> Seg1;
                Seg1.Add(TransformPoint(FVector2D(0, -Arm)));
                Seg1.Add(TransformPoint(FVector2D(0, Arm)));
                Segments.Add(Seg1);
                TArray<FVector2D> Seg2;
                Seg2.Add(TransformPoint(FVector2D(-Stub, -Arm + Stub)));
                Seg2.Add(TransformPoint(FVector2D(0, -Arm)));
                Segments.Add(Seg2);
                TArray<FVector2D> Seg3;
                Seg3.Add(TransformPoint(FVector2D(0, Arm)));
                Seg3.Add(TransformPoint(FVector2D(Arm * 0.7f, Arm)));
                Segments.Add(Seg3);
            }
            break;
        }
        
        for (const auto& Seg : Segments)
        {
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                Seg, DrawEffects, Color, true, Thickness);
        }
    }
    
    // Draw rotation ring with tick marks
    void DrawRotationRing(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
        const FVector2D& Center, float Radius, const FLinearColor& Color, float Thickness)
    {
        const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
        const int32 Segments = 48;
        
        // Draw ring
        TArray<FVector2D> Ring;
        for (int32 i = 0; i <= Segments; ++i)
        {
            const float Angle = (float)i / Segments * 2.0f * PI;
            Ring.Add(Center + FVector2D(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius));
        }
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
            Ring, DrawEffects, Color * 0.3f, true, Thickness * 0.5f);
        
        // Draw 45-degree tick marks
        for (int32 i = 0; i < 8; ++i)
        {
            const float Angle = (float)i * 45.0f * PI / 180.0f;
            const float TickLen = (i % 2 == 0) ? 8.0f : 5.0f;  // Longer ticks at 90° intervals
            TArray<FVector2D> Tick;
            Tick.Add(Center + FVector2D(FMath::Cos(Angle) * (Radius - TickLen), FMath::Sin(Angle) * (Radius - TickLen)));
            Tick.Add(Center + FVector2D(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius));
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                Tick, DrawEffects, Color * 0.5f, true, Thickness * 0.5f);
        }
    }
}

// ============================================================================
// SMarkAlignmentWidget Implementation
// ============================================================================

void SMarkAlignmentWidget::Construct(const FArguments& InArgs)
{
    PrimaryColor = InArgs._PrimaryColor;
    DimColor = InArgs._DimColor;
    LineThickness = InArgs._LineThickness;
    Program = InArgs._Program;
}

FVector2D SMarkAlignmentWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return FVector2D::ZeroVector;
}

int32 SMarkAlignmentWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (!Program) return LayerId;
    
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FLinearColor Primary = PrimaryColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const FLinearColor Dim = DimColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const FLinearColor InvertedText = FLinearColor::Black;  // For inverted button hover state
    const float Thickness = LineThickness.Get();
    const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
    
    // Layout: Two panes (target left, adjustable right), mirror controls at bottom of right pane
    const float ControlAreaHeight = 70.0f;  // Bottom area for mirror controls and apply button
    const float PaneWidth = Size.X * 0.5f;
    const float PaneHeight = Size.Y - ControlAreaHeight;
    const float MarkSize = FMath::Min(PaneWidth, PaneHeight) * 0.45f;
    const float RingRadius = MarkSize * 0.9f;
    
    // === Left Pane: Target ===
    const FVector2D TargetCenter(PaneWidth * 0.5f, PaneHeight * 0.5f);
    
    // Draw target frame
    const float FrameInset = 20.0f;
    TArray<FVector2D> TargetFrame;
    TargetFrame.Add(FVector2D(FrameInset, FrameInset));
    TargetFrame.Add(FVector2D(PaneWidth - FrameInset, FrameInset));
    TargetFrame.Add(FVector2D(PaneWidth - FrameInset, PaneHeight - FrameInset));
    TargetFrame.Add(FVector2D(FrameInset, PaneHeight - FrameInset));
    TargetFrame.Add(FVector2D(FrameInset, FrameInset));
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
        TargetFrame, DrawEffects, Dim * 0.5f, true, Thickness * 0.5f);
    
    // Draw target mark
    DrawMark(OutDrawElements, LayerId + 1, AllottedGeometry, TargetCenter, MarkSize,
        Program->TargetRotation, Program->bTargetMirroredX, Program->bTargetMirroredY, Program->MarkType, Primary, Thickness * 1.5f);
    
    // === Right Pane: Adjustable ===
    const FVector2D AdjustCenter(PaneWidth + PaneWidth * 0.5f, PaneHeight * 0.5f);
    
    // Draw adjustable frame
    TArray<FVector2D> AdjustFrame;
    AdjustFrame.Add(FVector2D(PaneWidth + FrameInset, FrameInset));
    AdjustFrame.Add(FVector2D(Size.X - FrameInset, FrameInset));
    AdjustFrame.Add(FVector2D(Size.X - FrameInset, PaneHeight - FrameInset));
    AdjustFrame.Add(FVector2D(PaneWidth + FrameInset, PaneHeight - FrameInset));
    AdjustFrame.Add(FVector2D(PaneWidth + FrameInset, FrameInset));
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
        AdjustFrame, DrawEffects, Primary * 0.5f, true, Thickness * 0.5f);
    
    // Draw rotation ring around adjustable mark
    DrawRotationRing(OutDrawElements, LayerId, AllottedGeometry, AdjustCenter, RingRadius, Dim, Thickness);
    
    // Draw adjustable mark (uses snapped CurrentRotation)
    DrawMark(OutDrawElements, LayerId + 1, AllottedGeometry, AdjustCenter, MarkSize,
        Program->CurrentRotation, Program->bCurrentMirroredX, Program->bCurrentMirroredY, Program->MarkType, Primary, Thickness * 1.5f);
    
    // === Knurled rotation grip (radial lines around the edge like a dial/knob) ===
    // The grip rotates smoothly with VisualRotation during drag
    const bool bRotationHovered = (Program->HoveredTarget == EMarkAlignmentDragTarget::RotationHandle);
    const bool bRotationDragging = (Program->DragTarget == EMarkAlignmentDragTarget::RotationHandle);
    const float GripInnerRadius = RingRadius + 4.0f;
    const float GripOuterRadius = RingRadius + 16.0f;
    const int32 NumGripLines = 24;  // Lines around the edge
    
    // Use VisualRotation for smooth ring rotation during drag, otherwise use snapped CurrentRotation
    float RingRotation = bRotationDragging ? Program->VisualRotation : Program->CurrentRotation;
    
    // When mirrored on exactly one axis, the visual rotation direction is reversed
    // (matching how DrawMark applies rotate-then-mirror)
    if (Program->bCurrentMirroredX != Program->bCurrentMirroredY)
    {
        RingRotation = -RingRotation;
    }
    
    const float RingRotationOffset = FMath::DegreesToRadians(RingRotation);
    
    for (int32 i = 0; i < NumGripLines; ++i)
    {
        const float BaseAngle = (float)i / NumGripLines * 2.0f * PI;
        const float GripAngle = BaseAngle + RingRotationOffset;
        TArray<FVector2D> GripLine;
        GripLine.Add(AdjustCenter + FVector2D(FMath::Cos(GripAngle), FMath::Sin(GripAngle)) * GripInnerRadius);
        GripLine.Add(AdjustCenter + FVector2D(FMath::Cos(GripAngle), FMath::Sin(GripAngle)) * GripOuterRadius);
        // Hover/drag = thicker lines, same color (not white highlight)
        const float LineThick = (bRotationHovered || bRotationDragging) ? Thickness * 2.0f : Thickness;
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
            GripLine, DrawEffects, Primary, true, LineThick);
    }
    
    // === Mirror Controls (bottom area) ===
    // Layout: [Mirror X] [Mirror Y] [Apply] centered across the full width
    const float MirrorControlY = PaneHeight + 8.0f;
    const float MirrorBoxSize = 50.0f;  // Larger buttons for easier clicking
    const float ApplyWidth = 100.0f;
    const float ApplyHeight = 50.0f;
    const float ButtonSpacing = 30.0f;  // Space between buttons
    
    // Calculate total width of all buttons and center them
    const float TotalButtonsWidth = MirrorBoxSize + ButtonSpacing + MirrorBoxSize + ButtonSpacing + ApplyWidth;
    const float ButtonsStartX = (Size.X - TotalButtonsWidth) * 0.5f;
    
    // Mirror X control (horizontal flip - mirrors across vertical axis)
    const float MirrorXCenterX = ButtonsStartX + MirrorBoxSize * 0.5f;
    const FVector2D MirrorXCenter(MirrorXCenterX, MirrorControlY + MirrorBoxSize * 0.5f);
    const bool bMirrorXHovered = (Program->HoveredTarget == EMarkAlignmentDragTarget::MirrorX);
    
    // Draw Mirror X box - fill with green on hover (inverted style)
    if (bMirrorXHovered)
    {
        const FPaintGeometry MirrorXFillGeom = AllottedGeometry.ToPaintGeometry(
            FVector2f(MirrorBoxSize, MirrorBoxSize),
            FSlateLayoutTransform(FVector2f(MirrorXCenter.X - MirrorBoxSize * 0.5f, MirrorXCenter.Y - MirrorBoxSize * 0.5f))
        );
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2, MirrorXFillGeom,
            FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, Primary);
    }
    
    TArray<FVector2D> MirrorXBox;
    MirrorXBox.Add(MirrorXCenter + FVector2D(-MirrorBoxSize * 0.5f, -MirrorBoxSize * 0.5f));
    MirrorXBox.Add(MirrorXCenter + FVector2D(MirrorBoxSize * 0.5f, -MirrorBoxSize * 0.5f));
    MirrorXBox.Add(MirrorXCenter + FVector2D(MirrorBoxSize * 0.5f, MirrorBoxSize * 0.5f));
    MirrorXBox.Add(MirrorXCenter + FVector2D(-MirrorBoxSize * 0.5f, MirrorBoxSize * 0.5f));
    MirrorXBox.Add(MirrorXCenter + FVector2D(-MirrorBoxSize * 0.5f, -MirrorBoxSize * 0.5f));
    FLinearColor MirrorXColor = Program->bCurrentMirroredX ? Primary : (bMirrorXHovered ? InvertedText : Dim);
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
        MirrorXBox, DrawEffects, MirrorXColor, true, Thickness);
    
    // Draw vertical line inside (shows the axis we mirror across)
    TArray<FVector2D> XLineV;
    XLineV.Add(MirrorXCenter + FVector2D(0, -MirrorBoxSize * 0.25f));
    XLineV.Add(MirrorXCenter + FVector2D(0, MirrorBoxSize * 0.25f));
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
        XLineV, DrawEffects, MirrorXColor, true, Thickness);
    
    // Draw vertical axis line through the adjustable pane when hovering Mirror X
    if (bMirrorXHovered)
    {
        TArray<FVector2D> AxisLine;
        AxisLine.Add(FVector2D(AdjustCenter.X, FrameInset));
        AxisLine.Add(FVector2D(AdjustCenter.X, PaneHeight - FrameInset));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
            AxisLine, DrawEffects, Primary * 0.7f, true, Thickness * 0.5f);
    }
    
    // Mirror Y control (vertical flip - mirrors across horizontal axis) - next to Mirror X
    const float MirrorYCenterX = MirrorXCenterX + MirrorBoxSize + ButtonSpacing;
    const FVector2D MirrorYCenter(MirrorYCenterX, MirrorControlY + MirrorBoxSize * 0.5f);
    const bool bMirrorYHovered = (Program->HoveredTarget == EMarkAlignmentDragTarget::MirrorY);
    
    // Draw Mirror Y box - fill with green on hover (inverted style)
    if (bMirrorYHovered)
    {
        const FPaintGeometry MirrorYFillGeom = AllottedGeometry.ToPaintGeometry(
            FVector2f(MirrorBoxSize, MirrorBoxSize),
            FSlateLayoutTransform(FVector2f(MirrorYCenter.X - MirrorBoxSize * 0.5f, MirrorYCenter.Y - MirrorBoxSize * 0.5f))
        );
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2, MirrorYFillGeom,
            FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, Primary);
    }
    
    TArray<FVector2D> MirrorYBox;
    MirrorYBox.Add(MirrorYCenter + FVector2D(-MirrorBoxSize * 0.5f, -MirrorBoxSize * 0.5f));
    MirrorYBox.Add(MirrorYCenter + FVector2D(MirrorBoxSize * 0.5f, -MirrorBoxSize * 0.5f));
    MirrorYBox.Add(MirrorYCenter + FVector2D(MirrorBoxSize * 0.5f, MirrorBoxSize * 0.5f));
    MirrorYBox.Add(MirrorYCenter + FVector2D(-MirrorBoxSize * 0.5f, MirrorBoxSize * 0.5f));
    MirrorYBox.Add(MirrorYCenter + FVector2D(-MirrorBoxSize * 0.5f, -MirrorBoxSize * 0.5f));
    FLinearColor MirrorYColor = Program->bCurrentMirroredY ? Primary : (bMirrorYHovered ? InvertedText : Dim);
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
        MirrorYBox, DrawEffects, MirrorYColor, true, Thickness);
    
    // Draw horizontal line inside (shows the axis we mirror across)
    TArray<FVector2D> YLineH;
    YLineH.Add(MirrorYCenter + FVector2D(-MirrorBoxSize * 0.25f, 0));
    YLineH.Add(MirrorYCenter + FVector2D(MirrorBoxSize * 0.25f, 0));
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
        YLineH, DrawEffects, MirrorYColor, true, Thickness);
    
    // Draw horizontal axis line through the adjustable pane when hovering Mirror Y
    if (bMirrorYHovered)
    {
        TArray<FVector2D> AxisLine;
        AxisLine.Add(FVector2D(PaneWidth + FrameInset, AdjustCenter.Y));
        AxisLine.Add(FVector2D(Size.X - FrameInset, AdjustCenter.Y));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
            AxisLine, DrawEffects, Primary * 0.7f, true, Thickness * 0.5f);
    }
    
    // === Apply Button ===
    const FVector2D ApplyCenter(MirrorYCenterX + MirrorBoxSize * 0.5f + ButtonSpacing + ApplyWidth * 0.5f, MirrorControlY + MirrorBoxSize * 0.5f);
    const bool bApplyHovered = (Program->HoveredTarget == EMarkAlignmentDragTarget::ApplyButton);
    
    // Draw apply button box
    const FVector2D ApplyTopLeft(ApplyCenter.X - ApplyWidth * 0.5f, ApplyCenter.Y - ApplyHeight * 0.5f);
    const FVector2D ApplyBottomRight(ApplyCenter.X + ApplyWidth * 0.5f, ApplyCenter.Y + ApplyHeight * 0.5f);
    
    if (bApplyHovered)
    {
        const FVector2D BtnSize = ApplyBottomRight - ApplyTopLeft;
        const FPaintGeometry ButtonGeometry = AllottedGeometry.ToPaintGeometry(
            FVector2f(BtnSize),
            FSlateLayoutTransform(FVector2f(ApplyTopLeft))
        );
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2, ButtonGeometry,
            FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, Primary);
    }
    
    TArray<FVector2D> ApplyBox;
    ApplyBox.Add(ApplyTopLeft);
    ApplyBox.Add(FVector2D(ApplyBottomRight.X, ApplyTopLeft.Y));
    ApplyBox.Add(ApplyBottomRight);
    ApplyBox.Add(FVector2D(ApplyTopLeft.X, ApplyBottomRight.Y));
    ApplyBox.Add(ApplyTopLeft);
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
        ApplyBox, DrawEffects, bApplyHovered ? InvertedText : Primary, true, Thickness);
    
    // Draw checkmark inside apply button
    TArray<FVector2D> Checkmark;
    Checkmark.Add(ApplyCenter + FVector2D(-12, 0));
    Checkmark.Add(ApplyCenter + FVector2D(-4, 8));
    Checkmark.Add(ApplyCenter + FVector2D(12, -8));
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
        Checkmark, DrawEffects, bApplyHovered ? InvertedText : Primary, false, Thickness * 1.5f);
    
    return LayerId + 4;
}

// ============================================================================
// FMarkAlignmentProgram Implementation
// ============================================================================

FMarkAlignmentProgram::FMarkAlignmentProgram()
    : FTrialProgramBase(8)
{
    SetTrialTitle(LOCTEXT("MarkAlignmentTitle", "Mark Alignment"));
    GenerateNewTrial();
}

void FMarkAlignmentProgram::GenerateNewTrial()
{
    MarkType = FMath::RandRange(0, 2);
    
    // Random target rotation (snap to 45 degree increments)
    TargetRotation = FMath::RandRange(0, 7) * 45.0f;
    // Target must always be mirrored on exactly one axis to keep the puzzle asymmetric
    const bool bMirrorX = FMath::RandBool();
    bTargetMirroredX = bMirrorX;
    bTargetMirroredY = !bMirrorX;
    
    // Start with different rotation (at least 90 degrees different, accounting for wrap-around)
    do
    {
        CurrentRotation = FMath::RandRange(0, 7) * 45.0f;
        float AngularDiff = FMath::Abs(CurrentRotation - TargetRotation);
        if (AngularDiff > 180.0f) AngularDiff = 360.0f - AngularDiff;
        if (AngularDiff >= 90.0f) break;
    } while (true);
    
    // Initialize visual rotation to match
    VisualRotation = CurrentRotation;
    
    bCurrentMirroredX = false;
    bCurrentMirroredY = false;
    
    InvalidateWidget();
}

bool FMarkAlignmentProgram::CheckMatch() const
{
    // Build transformed basis vectors (unit X/Y) using the same rotate-then-mirror order as DrawMark
    auto BuildTransformedAxes = [this](float RotationDegrees, bool bMirrorX, bool bMirrorY, FVector2D& OutX, FVector2D& OutY)
    {
        const float Normalized = NormalizeAngle(RotationDegrees);
        const float Radians = FMath::DegreesToRadians(Normalized);
        const float Cos = FMath::Cos(Radians);
        const float Sin = FMath::Sin(Radians);
        
        auto TransformPoint = [bMirrorX, bMirrorY, Cos, Sin](float X, float Y) -> FVector2D
        {
            FVector2D Rotated(X * Cos - Y * Sin, X * Sin + Y * Cos);
            if (bMirrorX) Rotated.X = -Rotated.X;
            if (bMirrorY) Rotated.Y = -Rotated.Y;
            return Rotated;
        };
        
        OutX = TransformPoint(1.0f, 0.0f);
        OutY = TransformPoint(0.0f, 1.0f);
    };
    
    FVector2D CurrentX, CurrentY;
    BuildTransformedAxes(CurrentRotation, bCurrentMirroredX, bCurrentMirroredY, CurrentX, CurrentY);
    
    FVector2D TargetX, TargetY;
    BuildTransformedAxes(TargetRotation, bTargetMirroredX, bTargetMirroredY, TargetX, TargetY);
    
    const float AxisTolerance = 0.01f;
    const float AxisToleranceSq = AxisTolerance * AxisTolerance;
    
    const bool bXAxisMatches = (CurrentX - TargetX).SizeSquared() <= AxisToleranceSq;
    const bool bYAxisMatches = (CurrentY - TargetY).SizeSquared() <= AxisToleranceSq;
    
    return bXAxisMatches && bYAxisMatches;
}

void FMarkAlignmentProgram::ApplyAlignment()
{
    if (CheckMatch())
    {
        AdvanceTrial();
    }
    else
    {
        FailTrial();
    }
}

FVector2D FMarkAlignmentProgram::GetAdjustCenter(const FVector2D& WidgetSize) const
{
    const float ControlAreaHeight = 70.0f;
    const float PaneWidth = WidgetSize.X * 0.5f;
    const float PaneHeight = WidgetSize.Y - ControlAreaHeight;
    return FVector2D(PaneWidth + PaneWidth * 0.5f, PaneHeight * 0.5f);
}

float FMarkAlignmentProgram::GetMarkSize(const FVector2D& WidgetSize) const
{
    const float ControlAreaHeight = 70.0f;
    const float PaneWidth = WidgetSize.X * 0.5f;
    const float PaneHeight = WidgetSize.Y - ControlAreaHeight;
    return FMath::Min(PaneWidth, PaneHeight) * 0.45f;
}

float FMarkAlignmentProgram::GetRingRadius(const FVector2D& WidgetSize) const
{
    return GetMarkSize(WidgetSize) * 0.9f;
}

float FMarkAlignmentProgram::AngleFromPosition(const FVector2D& Pos, const FVector2D& Center) const
{
    const FVector2D Dir = Pos - Center;
    // Convert to angle where 0 = 12 o'clock (top), clockwise positive
    float Angle = FMath::RadiansToDegrees(FMath::Atan2(Dir.X, -Dir.Y));
    return NormalizeAngle(Angle);
}

float FMarkAlignmentProgram::NormalizeAngle(float Angle) const
{
    while (Angle < 0.0f) Angle += 360.0f;
    while (Angle >= 360.0f) Angle -= 360.0f;
    return Angle;
}

EMarkAlignmentDragTarget FMarkAlignmentProgram::GetHitTarget(const FVector2D& LocalPos, const FVector2D& WidgetSize) const
{
    const float ControlAreaHeight = 70.0f;
    const float PaneHeight = WidgetSize.Y - ControlAreaHeight;
    const float MirrorControlY = PaneHeight + 8.0f;
    const float MirrorBoxSize = 50.0f;
    const float ApplyWidth = 100.0f;
    const float ApplyHeight = 50.0f;
    const float ButtonSpacing = 30.0f;
    
    // Check rotation grip (entire circle area around the adjustable mark)
    const FVector2D AdjustCenter = GetAdjustCenter(WidgetSize);
    const float RingRadius = GetRingRadius(WidgetSize);
    const float GripOuterRadius = RingRadius + 16.0f;
    const float DistFromCenter = FVector2D::Distance(LocalPos, AdjustCenter);
    
    // Check if within the entire rotation circle (from center to outer edge of knurling)
    const float GripTolerance = 8.0f;
    if (DistFromCenter <= (GripOuterRadius + GripTolerance))
    {
        return EMarkAlignmentDragTarget::RotationHandle;
    }
    
    // Check if in control area
    if (LocalPos.Y >= MirrorControlY)
    {
        // Calculate centered button positions (must match OnPaint)
        const float TotalButtonsWidth = MirrorBoxSize + ButtonSpacing + MirrorBoxSize + ButtonSpacing + ApplyWidth;
        const float ButtonsStartX = (WidgetSize.X - TotalButtonsWidth) * 0.5f;
        
        // Mirror X button
        const float MirrorXCenterX = ButtonsStartX + MirrorBoxSize * 0.5f;
        const FVector2D MirrorXCenter(MirrorXCenterX, MirrorControlY + MirrorBoxSize * 0.5f);
        if (FMath::Abs(LocalPos.X - MirrorXCenter.X) <= MirrorBoxSize * 0.5f &&
            FMath::Abs(LocalPos.Y - MirrorXCenter.Y) <= MirrorBoxSize * 0.5f)
        {
            return EMarkAlignmentDragTarget::MirrorX;
        }
        
        // Mirror Y button
        const float MirrorYCenterX = MirrorXCenterX + MirrorBoxSize + ButtonSpacing;
        const FVector2D MirrorYCenter(MirrorYCenterX, MirrorControlY + MirrorBoxSize * 0.5f);
        if (FMath::Abs(LocalPos.X - MirrorYCenter.X) <= MirrorBoxSize * 0.5f &&
            FMath::Abs(LocalPos.Y - MirrorYCenter.Y) <= MirrorBoxSize * 0.5f)
        {
            return EMarkAlignmentDragTarget::MirrorY;
        }
        
        // Apply button
        const FVector2D ApplyCenter(MirrorYCenterX + MirrorBoxSize * 0.5f + ButtonSpacing + ApplyWidth * 0.5f, MirrorControlY + MirrorBoxSize * 0.5f);
        if (FMath::Abs(LocalPos.X - ApplyCenter.X) <= ApplyWidth * 0.5f &&
            FMath::Abs(LocalPos.Y - ApplyCenter.Y) <= ApplyHeight * 0.5f)
        {
            return EMarkAlignmentDragTarget::ApplyButton;
        }
    }
    
    return EMarkAlignmentDragTarget::None;
}

void FMarkAlignmentProgram::InvalidateWidget() const
{
    if (const TSharedPtr<SWidget> Widget = AlignWidget.Pin())
    {
        Widget->Invalidate(EInvalidateWidget::Paint);
    }
}

TSharedRef<SWidget> FMarkAlignmentProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();

    TSharedPtr<SMarkAlignmentWidget> MarkWidget;

    TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1.0f).Padding(Style.GetSmallPadding())
        [SAssignNew(MarkWidget, SMarkAlignmentWidget)
            .PrimaryColor(Style.GetPrimaryColor())
            .DimColor(Style.GetDimColor())
            .LineThickness(Style.LineThickness)
            .Program(this)];

    AlignWidget = MarkWidget;
    return Content;
}

void FMarkAlignmentProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || GetTaskComplete()) return;
    
    const TSharedPtr<SWidget> Widget = AlignWidget.Pin();
    if (!Widget.IsValid()) return;
    
    const FGeometry& Geometry = Widget->GetCachedGeometry();
    const FVector2D LocalPos = Geometry.AbsoluteToLocal(Event.ScreenPixelPosition);
    const FVector2D WidgetSize = Geometry.GetLocalSize();
    
    // If dragging rotation handle, update rotation based on delta from grab point
    if (DragTarget == EMarkAlignmentDragTarget::RotationHandle && ActivePointerKey.IsValid())
    {
        const FVector2D Center = GetAdjustCenter(WidgetSize);
        const float CurrentAngle = AngleFromPosition(LocalPos, Center);
        
        // Calculate delta from where we grabbed
        float Delta = CurrentAngle - DragStartAngle;
        // Handle wrap-around
        if (Delta > 180.0f) Delta -= 360.0f;
        if (Delta < -180.0f) Delta += 360.0f;
        
        // When mirrored on exactly one axis, the visual rotation is reversed,
        // so we need to negate the delta to make dragging feel intuitive
        if (bCurrentMirroredX != bCurrentMirroredY)
        {
            Delta = -Delta;
        }
        
        // Visual rotation is smooth (unsnapped)
        VisualRotation = NormalizeAngle(DragStartRotation + Delta);
        
        // Shape rotation is snapped to 45-degree increments
        CurrentRotation = FMath::RoundToFloat(VisualRotation / 45.0f) * 45.0f;
        CurrentRotation = NormalizeAngle(CurrentRotation);
        
        InvalidateWidget();
        return;
    }
    
    // Update hover state
    EMarkAlignmentDragTarget NewHovered = GetHitTarget(LocalPos, WidgetSize);
    
    if (NewHovered != HoveredTarget)
    {
        HoveredTarget = NewHovered;
        InvalidateWidget();
    }
    
    UpdateCursorForState(false, HoveredTarget != EMarkAlignmentDragTarget::None);
}

void FMarkAlignmentProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || GetTaskComplete()) return;
    
    ActivePointerKey = Event.TriggerKey;
    
    const TSharedPtr<SWidget> Widget = AlignWidget.Pin();
    if (!Widget.IsValid()) return;
    
    const FGeometry& Geometry = Widget->GetCachedGeometry();
    const FVector2D LocalPos = Geometry.AbsoluteToLocal(Event.ScreenPixelPosition);
    const FVector2D WidgetSize = Geometry.GetLocalSize();
    
    switch (HoveredTarget)
    {
    case EMarkAlignmentDragTarget::RotationHandle:
        {
            DragTarget = EMarkAlignmentDragTarget::RotationHandle;
            // Store the grab angle and current rotation for delta calculation
            const FVector2D Center = GetAdjustCenter(WidgetSize);
            DragStartAngle = AngleFromPosition(LocalPos, Center);
            DragStartRotation = CurrentRotation;
            VisualRotation = CurrentRotation;
        }
        break;
        
    case EMarkAlignmentDragTarget::MirrorX:
        bCurrentMirroredX = !bCurrentMirroredX;
        InvalidateWidget();
        break;
        
    case EMarkAlignmentDragTarget::MirrorY:
        bCurrentMirroredY = !bCurrentMirroredY;
        InvalidateWidget();
        break;
        
    case EMarkAlignmentDragTarget::ApplyButton:
        ApplyAlignment();
        break;
        
    default:
        break;
    }
}

void FMarkAlignmentProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    // Snap the visual rotation to match the snapped rotation on release
    if (DragTarget == EMarkAlignmentDragTarget::RotationHandle)
    {
        VisualRotation = CurrentRotation;
        InvalidateWidget();
    }
    
    ActivePointerKey = FKey();
    DragTarget = EMarkAlignmentDragTarget::None;
}

#undef LOCTEXT_NAMESPACE
