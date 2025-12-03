#include "UI/Programs/VendorSandboxProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Algo/RandomShuffle.h"

#define LOCTEXT_NAMESPACE "VendorSandboxProgram"

REGISTER_SCREEN_PROGRAM(FVendorSandboxProgram, "VendorSandbox")

namespace VendorSandboxLayout
{
    constexpr float CardWidth = 240.0f;
    constexpr float CardHeight = 260.0f;
    constexpr float CardSpacing = 48.0f;
    constexpr float CardPadding = 24.0f;
    constexpr float HistogramHeight = 140.0f;
    constexpr float HistogramVerticalGap = 90.0f;
    constexpr float HistogramBaselineOffset = 70.0f;
    constexpr float HistogramBarGap = 60.0f;
    constexpr float HistogramBarWidth = 42.0f;
    constexpr float MeterWidth = 380.0f;
    constexpr float MeterHeight = 20.0f;
    constexpr float MeterFrameThickness = 2.0f;
    constexpr float CardDeltaToCardGap = 18.0f;
    constexpr float CardValueBarHeight = 20.0f;
    constexpr float CardValueBarLabelGap = 30.0f;
    constexpr float ScoreToDeltaGap = 36.0f;
    constexpr int32 MaxFatigueStacks = 12;
}

// ============================================================================
// SVendorSandboxWidget Implementation
// ============================================================================

void SVendorSandboxWidget::Construct(const FArguments& InArgs)
{
    PrimaryColor = InArgs._PrimaryColor;
    DimColor = InArgs._DimColor;
    LineThickness = InArgs._LineThickness;
    Program = InArgs._Program;
}

FVector2D SVendorSandboxWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return FVector2D::ZeroVector;
}

int32 SVendorSandboxWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (!Program) return LayerId;

    const FScreenProgramStyle& Style = Program->GetProgramStyle();
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FLinearColor Primary = PrimaryColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const FLinearColor Dim = DimColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const float Thickness = LineThickness.Get();
    const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;

    const float CardsTotalWidth = VendorSandboxLayout::CardWidth * 4.0f + VendorSandboxLayout::CardSpacing * 3.0f;
    const float CardsLeft = (Size.X - CardsTotalWidth) * 0.5f;
    const float CardsTop = Size.Y * 0.35f - VendorSandboxLayout::CardHeight * 0.5f;

    const float HistogramTop = CardsTop + VendorSandboxLayout::CardHeight + VendorSandboxLayout::HistogramVerticalGap;
    const float HistogramBaselineY = HistogramTop + VendorSandboxLayout::HistogramBaselineOffset;
    const float HistogramWidth = VendorSandboxLayout::HistogramBarWidth * 4.0f + VendorSandboxLayout::HistogramBarGap * 3.0f;
    const float HistogramLeft = (Size.X - HistogramWidth) * 0.5f;

    const int32 TitleFontSize = FMath::RoundToInt(Style.TextSize * 1.25f);
    const int32 ValueFontSize = FMath::RoundToInt(Style.TextSize * 2.2f);
    const int32 ChangeFontSize = FMath::RoundToInt(Style.TextSize * 0.9f);
    const int32 ScoreDisplayFontSize = FMath::RoundToInt(Style.TextSize * 2.4f);
    const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", TitleFontSize);
    const FSlateFontInfo ValueFont = FCoreStyle::GetDefaultFontStyle("Bold", ValueFontSize);
    const FSlateFontInfo ChangeFont = FCoreStyle::GetDefaultFontStyle("Regular", ChangeFontSize);
    const FSlateFontInfo ScoreDisplayFont = FCoreStyle::GetDefaultFontStyle("Bold", ScoreDisplayFontSize);
    const float SectionGap = Style.GetSmallPadding();

    // ---------------------------------------------------------------------
    // Draw channel cards
    // ---------------------------------------------------------------------
    for (int32 DeckIndex = 0; DeckIndex < 4; ++DeckIndex)
    {
        const FVector2D TopLeft = Program->GetDeckCardTopLeft(DeckIndex, Size);
        const FVector2D CardSize(VendorSandboxLayout::CardWidth, VendorSandboxLayout::CardHeight);
        const bool bHovered = (Program->HoveredDeck == DeckIndex);

        const float PositiveTotal = Program->DeckPositiveTotals.IsValidIndex(DeckIndex) ? Program->DeckPositiveTotals[DeckIndex] : 0.0f;
        const float NegativeTotal = Program->DeckNegativeTotals.IsValidIndex(DeckIndex) ? Program->DeckNegativeTotals[DeckIndex] : 0.0f;
        const int32 Consecutive = Program->DeckConsecutiveCounts.IsValidIndex(DeckIndex) ? Program->DeckConsecutiveCounts[DeckIndex] : 0;
        const float LastChange = Program->DeckLastChange.IsValidIndex(DeckIndex) ? Program->DeckLastChange[DeckIndex] : 0.0f;
        const float NetValue = PositiveTotal - NegativeTotal;

        float FatigueRatio = 0.0f;
        if (Consecutive >= 1)
        {
            const int32 EffectiveStacks = FMath::Clamp(Consecutive, 0, VendorSandboxLayout::MaxFatigueStacks);
            FatigueRatio = static_cast<float>(EffectiveStacks) / static_cast<float>(VendorSandboxLayout::MaxFatigueStacks);
        }
        FatigueRatio = FMath::Clamp(FatigueRatio, 0.0f, 1.0f);

        const FLinearColor BaseFillColor = Dim * 0.15f;
        const FLinearColor FatigueFillColor = BaseFillColor * FLinearColor(0.5f, 0.75f, 0.5f, 1.0f);

        const FVector2D FillSize = CardSize;

        // Always draw fatigue background first
        const FPaintGeometry FillGeom = AllottedGeometry.ToPaintGeometry(FVector2f(FillSize), FSlateLayoutTransform(FVector2f(TopLeft)));
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
            FillGeom, FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, FatigueFillColor);

        // Always draw healthy band on top (full height when no fatigue, shrinks as fatigue increases)
        const float HealthyBandHeight = FMath::Clamp(CardSize.Y * (1.0f - FatigueRatio), 0.0f, CardSize.Y);
        if (HealthyBandHeight > KINDA_SMALL_NUMBER)
        {
            const FVector2D HealthyBandPos(TopLeft.X, TopLeft.Y + CardSize.Y - HealthyBandHeight);
            const FVector2D HealthyBandSize(CardSize.X, HealthyBandHeight);
            const FPaintGeometry HealthyBandGeom = AllottedGeometry.ToPaintGeometry(FVector2f(HealthyBandSize), FSlateLayoutTransform(FVector2f(HealthyBandPos)));
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
                HealthyBandGeom, FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, BaseFillColor);
        }

        // Draw divider line when there's fatigue
        if (FatigueRatio > 0.0f && HealthyBandHeight > KINDA_SMALL_NUMBER && HealthyBandHeight < CardSize.Y - KINDA_SMALL_NUMBER)
        {
            const float DividerY = TopLeft.Y + CardSize.Y - HealthyBandHeight;
            TArray<FVector2D> FatigueDivider;
            FatigueDivider.Add(FVector2D(TopLeft.X, DividerY));
            FatigueDivider.Add(FVector2D(TopLeft.X + CardSize.X, DividerY));
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), FatigueDivider,
                DrawEffects, Dim * 0.6f, false, Thickness);
        }

        // Outline
        TArray<FVector2D> Outline;
        Outline.Add(TopLeft);
        Outline.Add(FVector2D(TopLeft.X + CardSize.X, TopLeft.Y));
        Outline.Add(FVector2D(TopLeft.X + CardSize.X, TopLeft.Y + CardSize.Y));
        Outline.Add(FVector2D(TopLeft.X, TopLeft.Y + CardSize.Y));
        Outline.Add(TopLeft);
        const FLinearColor OutlineColor = bHovered ? Primary : Dim * 0.75f;
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), Outline, DrawEffects,
            OutlineColor, true, Thickness);

        const float TextLeft = TopLeft.X + VendorSandboxLayout::CardPadding;

        const FString Title = FString::Printf(TEXT("Channel %s"), *Program->GetDeckLabel(DeckIndex));
        const FVector2D TitlePos(TextLeft, TopLeft.Y + VendorSandboxLayout::CardPadding);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 4,
            AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(TitlePos))), Title, TitleFont, DrawEffects, Primary);

        // Net value displayed in center of card
        const int32 NetOutput = FMath::RoundToInt(NetValue);
        const FString NetText = FString::Printf(TEXT("%s%d"), NetOutput >= 0 ? TEXT("+") : TEXT(""), NetOutput);
        const float ApproxCharWidth = ValueFontSize * 0.6f;
        const float ValueWidth = NetText.Len() * ApproxCharWidth;
        const float ValueHeight = static_cast<float>(ValueFontSize);
        const FVector2D ValuePos(TopLeft.X + (CardSize.X - ValueWidth) * 0.5f,
            TopLeft.Y + (CardSize.Y - ValueHeight) * 0.5f);
        const FLinearColor ValueColor = NetOutput >= 0 ? Primary : Program->GetProgramStyle().GetErrorColor().GetSpecifiedColor();
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 4,
            AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(ValuePos))), NetText, ValueFont, DrawEffects, ValueColor);

        // Visual bar ABOVE the card for cumulative net value
        const float BarPadding = VendorSandboxLayout::CardPadding;
        const float BarMaxWidth = CardSize.X - BarPadding * 2.0f;
        const float BarHeight = VendorSandboxLayout::CardValueBarHeight;
        const float BarCenterY = TopLeft.Y - VendorSandboxLayout::CardDeltaToCardGap - BarHeight * 0.5f;
        const float BarLeft = TopLeft.X + BarPadding;
        
        // Background track
        const FVector2D TrackPos(BarLeft, BarCenterY - BarHeight * 0.5f);
        const FVector2D TrackSize(BarMaxWidth, BarHeight);
        const FPaintGeometry TrackGeom = AllottedGeometry.ToPaintGeometry(FVector2f(TrackSize), FSlateLayoutTransform(FVector2f(TrackPos)));
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 5, TrackGeom,
            FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, Dim * 0.2f);

        // Center line (zero point)
        const float CenterX = BarLeft + BarMaxWidth * 0.5f;
        TArray<FVector2D> CenterLine;
        CenterLine.Add(FVector2D(CenterX, BarCenterY - BarHeight * 0.5f - 3.0f));
        CenterLine.Add(FVector2D(CenterX, BarCenterY + BarHeight * 0.5f + 3.0f));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 6, AllottedGeometry.ToPaintGeometry(), CenterLine,
            DrawEffects, Dim * 0.6f, false, 2.0f);

        // Value bar (grows from center)
        const float MaxDisplayValue = 100.0f;
        const float ClampedNet = FMath::Clamp(NetValue, -MaxDisplayValue, MaxDisplayValue);
        const float BarRatio = ClampedNet / MaxDisplayValue;
        const float HalfBarMax = BarMaxWidth * 0.5f;
        
        if (FMath::Abs(BarRatio) > 0.01f)
        {
            const float BarWidth = FMath::Abs(BarRatio) * HalfBarMax;
            const float BarStartX = BarRatio >= 0 ? CenterX : CenterX - BarWidth;
            const FVector2D BarPos(BarStartX, BarCenterY - BarHeight * 0.5f + 2.0f);
            const FVector2D BarSize(BarWidth, BarHeight - 4.0f);
            const FLinearColor BarColor = BarRatio >= 0 ? Primary * 0.7f : Program->GetProgramStyle().GetErrorColor().GetSpecifiedColor() * 0.7f;
            const FPaintGeometry BarGeom = AllottedGeometry.ToPaintGeometry(FVector2f(BarSize), FSlateLayoutTransform(FVector2f(BarPos)));
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 7, BarGeom,
                FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, BarColor);
        }

        // Delta indicator (highlight segment for last change)
        const int32 RoundedChange = FMath::RoundToInt(LastChange);
        if (RoundedChange != 0)
        {
            const float PreviousNet = NetValue - LastChange;
            const float ClampedPrev = FMath::Clamp(PreviousNet, -MaxDisplayValue, MaxDisplayValue);
            const float PrevRatio = ClampedPrev / MaxDisplayValue;
            
            const float DeltaWidth = FMath::Abs(BarRatio - PrevRatio) * HalfBarMax;
            if (DeltaWidth > 1.0f)
            {
                const float DeltaStartX = RoundedChange >= 0 
                    ? CenterX + PrevRatio * HalfBarMax 
                    : CenterX + BarRatio * HalfBarMax;
                const FVector2D DeltaPos(DeltaStartX, BarCenterY - BarHeight * 0.5f + 2.0f);
                const FVector2D DeltaSize(DeltaWidth, BarHeight - 4.0f);
                const FLinearColor DeltaColor = RoundedChange >= 0 ? Primary : Program->GetProgramStyle().GetErrorColor().GetSpecifiedColor();
                const FPaintGeometry DeltaGeom = AllottedGeometry.ToPaintGeometry(FVector2f(DeltaSize), FSlateLayoutTransform(FVector2f(DeltaPos)));
                FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 8, DeltaGeom,
                    FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, DeltaColor);
            }
            
            // Small delta label above bar
            const FString DeltaText = FString::Printf(TEXT("%s%d"), RoundedChange >= 0 ? TEXT("+") : TEXT(""), RoundedChange);
            const float DeltaTextWidth = DeltaText.Len() * (ChangeFontSize * 0.55f);
            const float DeltaTextBaseline = BarCenterY - BarHeight * 0.5f - VendorSandboxLayout::CardValueBarLabelGap;
            const FVector2D DeltaTextPos(TopLeft.X + (CardSize.X - DeltaTextWidth) * 0.5f, DeltaTextBaseline - static_cast<float>(ChangeFontSize));
            const FLinearColor DeltaTextColor = RoundedChange >= 0 ? Primary : Program->GetProgramStyle().GetErrorColor().GetSpecifiedColor();
            FSlateDrawElement::MakeText(OutDrawElements, LayerId + 9,
                AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(DeltaTextPos))), DeltaText, ChangeFont, DrawEffects, DeltaTextColor);
        }
    }

    // ---------------------------------------------------------------------
    // Output meter
    // ---------------------------------------------------------------------

    const float MeterLeft = (Size.X - VendorSandboxLayout::MeterWidth) * 0.5f;
    const float MeterTop = CardsTop
        - (VendorSandboxLayout::CardDeltaToCardGap
            + VendorSandboxLayout::CardValueBarHeight
            + VendorSandboxLayout::CardValueBarLabelGap
            + static_cast<float>(ChangeFontSize)
            + VendorSandboxLayout::ScoreToDeltaGap
            + VendorSandboxLayout::MeterHeight);

    const int32 Target = Program->QuotaGoal;
    const int32 StartValue = Program->StartingScore;
    const int32 PreviousValue = Program->Score - Program->LastNet;
    const int32 CurrentValue = Program->Score;
    const int32 MaxValue = FMath::Max(Target, FMath::Max(CurrentValue, StartValue));
    const float MeterRange = FMath::Max(static_cast<float>(MaxValue - StartValue), 1.0f);
    const float UnitPerPixel = VendorSandboxLayout::MeterWidth / MeterRange;

    const float PreviousWidth = (static_cast<float>(PreviousValue) - static_cast<float>(StartValue)) * UnitPerPixel;
    const float CurrentWidth = (static_cast<float>(CurrentValue) - static_cast<float>(StartValue)) * UnitPerPixel;
    const float ClampedCurrentWidth = FMath::Clamp(CurrentWidth, 0.0f, VendorSandboxLayout::MeterWidth);
    const float ClampedPreviousWidth = FMath::Clamp(PreviousWidth, 0.0f, VendorSandboxLayout::MeterWidth);

    const FString ScoreValueText = FString::FromInt(Program->Score);
    const float ScoreApproxWidth = ScoreValueText.Len() * (ScoreDisplayFontSize * 0.6f);
    const float ScoreHeight = static_cast<float>(ScoreDisplayFontSize);
    const FVector2D ScorePos(MeterLeft + (VendorSandboxLayout::MeterWidth - ScoreApproxWidth) * 0.5f,
        MeterTop - ScoreHeight - SectionGap * 2.5f);

    FSlateDrawElement::MakeText(OutDrawElements, LayerId + 5,
        AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(ScorePos))), ScoreValueText,
        ScoreDisplayFont, DrawEffects, Primary);

    const FVector2D MeterFramePos(MeterLeft, MeterTop);
    const FVector2D MeterFrameSize(VendorSandboxLayout::MeterWidth, VendorSandboxLayout::MeterHeight);
    const FPaintGeometry MeterBgGeom = AllottedGeometry.ToPaintGeometry(FVector2f(MeterFrameSize), FSlateLayoutTransform(FVector2f(MeterFramePos)));
    FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 7, MeterBgGeom,
        FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, Dim * 0.25f);

    if (ClampedCurrentWidth > 1.0f)
    {
        const FVector2D FillSize(ClampedCurrentWidth, VendorSandboxLayout::MeterHeight);
        const FPaintGeometry FillGeom = AllottedGeometry.ToPaintGeometry(FVector2f(FillSize), FSlateLayoutTransform(FVector2f(MeterFramePos)));
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 8, FillGeom,
            FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, Primary * 0.45f);
    }

    if (Program->LastNet != 0)
    {
        const float PreviousPos = MeterLeft + ClampedPreviousWidth;
        const float CurrentPos = MeterLeft + ClampedCurrentWidth;
        const float SegmentWidth = FMath::Abs(CurrentPos - PreviousPos);
        if (SegmentWidth > 1.0f)
        {
            const float SegmentLeft = FMath::Min(CurrentPos, PreviousPos);
            const FVector2D ImpactPos(SegmentLeft, MeterTop);
            const FVector2D ImpactSize(SegmentWidth, VendorSandboxLayout::MeterHeight);
            const FLinearColor ImpactColor = Program->LastNet >= 0 ? Primary : Program->GetProgramStyle().GetErrorColor().GetSpecifiedColor();
            const FPaintGeometry ImpactGeom = AllottedGeometry.ToPaintGeometry(FVector2f(ImpactSize), FSlateLayoutTransform(FVector2f(ImpactPos)));
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 9, ImpactGeom,
                FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, ImpactColor * 0.8f);
        }
    }

    const float TargetOffset = (static_cast<float>(Target) - static_cast<float>(StartValue)) * UnitPerPixel;
    if (TargetOffset >= 0.0f && TargetOffset <= VendorSandboxLayout::MeterWidth)
    {
        TArray<FVector2D> Marker;
        Marker.Add(FVector2D(MeterLeft + TargetOffset, MeterTop - 6.0f));
        Marker.Add(FVector2D(MeterLeft + TargetOffset, MeterTop + VendorSandboxLayout::MeterHeight + 6.0f));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 10, AllottedGeometry.ToPaintGeometry(), Marker,
            DrawEffects, Primary, false, VendorSandboxLayout::MeterFrameThickness);
    }

    TArray<FVector2D> FrameOutline;
    FrameOutline.Add(FVector2D(MeterLeft, MeterTop));
    FrameOutline.Add(FVector2D(MeterLeft + VendorSandboxLayout::MeterWidth, MeterTop));
    FrameOutline.Add(FVector2D(MeterLeft + VendorSandboxLayout::MeterWidth, MeterTop + VendorSandboxLayout::MeterHeight));
    FrameOutline.Add(FVector2D(MeterLeft, MeterTop + VendorSandboxLayout::MeterHeight));
    FrameOutline.Add(FVector2D(MeterLeft, MeterTop));
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 11, AllottedGeometry.ToPaintGeometry(), FrameOutline,
        DrawEffects, Dim * 0.8f, true, VendorSandboxLayout::MeterFrameThickness);

    if (Program->StartingDraws > 0)
    {
        const int32 TotalPips = Program->StartingDraws;
        const int32 ActivePips = FMath::Clamp(Program->DrawsRemaining, 0, TotalPips);
        const float PipWidth = 36.0f;
        const float PipHeight = 4.0f;
        const float ColumnHeight = VendorSandboxLayout::CardHeight;
        const float PipSpacing = TotalPips > 1 ? (ColumnHeight - TotalPips * PipHeight) / static_cast<float>(TotalPips - 1) : 0.0f;
        const float PipColumnLeft = CardsLeft - PipWidth - Style.GetPadding();
        const float PipColumnTop = CardsTop;

        for (int32 PipIndex = 0; PipIndex < TotalPips; ++PipIndex)
        {
            const float PipY = PipColumnTop + PipIndex * (PipHeight + PipSpacing);
            const FVector2D PipPos(PipColumnLeft, PipY);
            const FVector2D PipSize(PipWidth, PipHeight);
            const FPaintGeometry PipGeom = AllottedGeometry.ToPaintGeometry(FVector2f(PipSize), FSlateLayoutTransform(FVector2f(PipPos)));
            const int32 SpentCount = TotalPips - ActivePips;
            const bool bActive = PipIndex >= SpentCount;
            const FLinearColor PipColor = bActive ? Primary * 0.55f : Dim * 0.35f;
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 12, PipGeom,
                FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, PipColor);
        }
    }

    // ---------------------------------------------------------------------
    // Histogram (stacked throughput by channel)
    // ---------------------------------------------------------------------
    float MaxMagnitude = 100.0f;
    for (int32 DeckIndex = 0; DeckIndex < 4; ++DeckIndex)
    {
        const float Pos = Program->DeckPositiveTotals.IsValidIndex(DeckIndex) ? Program->DeckPositiveTotals[DeckIndex] : 0.0f;
        const float Neg = Program->DeckNegativeTotals.IsValidIndex(DeckIndex) ? Program->DeckNegativeTotals[DeckIndex] : 0.0f;
        MaxMagnitude = FMath::Max(MaxMagnitude, Pos + Neg);
    }

    const float BarScale = (VendorSandboxLayout::HistogramHeight - 30.0f) / MaxMagnitude;

    // Baseline axis
    TArray<FVector2D> Axis;
    Axis.Add(FVector2D(HistogramLeft, HistogramBaselineY));
    Axis.Add(FVector2D(HistogramLeft + HistogramWidth, HistogramBaselineY));
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 13, AllottedGeometry.ToPaintGeometry(), Axis, DrawEffects, Dim * 0.7f, false, Thickness);

    for (int32 DeckIndex = 0; DeckIndex < 4; ++DeckIndex)
    {
        const float PosTotal = Program->DeckPositiveTotals.IsValidIndex(DeckIndex) ? Program->DeckPositiveTotals[DeckIndex] : 0.0f;
        const float NegTotal = Program->DeckNegativeTotals.IsValidIndex(DeckIndex) ? Program->DeckNegativeTotals[DeckIndex] : 0.0f;

        const float BarX = HistogramLeft + DeckIndex * (VendorSandboxLayout::HistogramBarWidth + VendorSandboxLayout::HistogramBarGap);
        const FVector2D BarSize(VendorSandboxLayout::HistogramBarWidth, 0.0f);

        const float PosHeight = PosTotal * BarScale;
        if (PosHeight > 1.0f)
        {
            const FVector2D PosTopLeft(BarX, HistogramBaselineY - PosHeight);
            const FPaintGeometry PosGeom = AllottedGeometry.ToPaintGeometry(
                FVector2f(VendorSandboxLayout::HistogramBarWidth, PosHeight),
                FSlateLayoutTransform(FVector2f(PosTopLeft)));
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 14, PosGeom,
                FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, Primary * 0.55f);
        }

        const float NegHeight = NegTotal * BarScale;
        if (NegHeight > 1.0f)
        {
            const FVector2D NegTopLeft(BarX, HistogramBaselineY + 1.0f);
            const FPaintGeometry NegGeom = AllottedGeometry.ToPaintGeometry(
                FVector2f(VendorSandboxLayout::HistogramBarWidth, NegHeight),
                FSlateLayoutTransform(FVector2f(NegTopLeft)));
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 14, NegGeom,
                FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, Dim * 0.6f);
        }

        // Deck label under bar
        const FString Label = Program->GetDeckLabel(DeckIndex);
        const float LabelWidth = VendorSandboxLayout::HistogramBarWidth;
        const FVector2D LabelPos(BarX + (LabelWidth * 0.5f) - 10.0f, HistogramBaselineY + 6.0f);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 15,
            AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2f(LabelPos))), Label,
            FCoreStyle::GetDefaultFontStyle("Bold", Style.TextSize), DrawEffects, Primary);
    }

    return LayerId + 16;
}

// ============================================================================
// FVendorSandboxProgram Implementation
// ============================================================================

FVendorSandboxProgram::FVendorSandboxProgram()
    : FTrialProgramBase(3)
{
    SetTrialTitle(LOCTEXT("VendorSandboxTitle", "Vendor Sandbox"));
    DeckOutcomeSequences.SetNum(4);
    DeckOutcomeIndices.Init(0, 4);
    DeckDrawCounts.Init(0, 4);
    DeckConsecutiveCounts.Init(0, 4);
    DeckPositiveTotals.Init(0, 4);
    DeckNegativeTotals.Init(0, 4);

    GenerateNewTrial();
}

namespace
{
    void BuildDeckPattern(int32 DeckIndex, TArray<FVendorSandboxProgram::FDeckOutcome>& OutDeck)
    {
        OutDeck.Reset();
        switch (DeckIndex)
        {
            case 0: // Channel A - High variance gamble. Net ~0/12 (big wins, big losses)
            {
                const FVendorSandboxProgram::FDeckOutcome Pattern[] = {
                    {20, 0}, {20, 0}, {20, 35}, {20, 0}, {20, 40}, {20, 0},
                    {20, 0}, {20, 30}, {20, 0}, {20, 0}, {20, 35}, {20, 0}
                };
                OutDeck.Append(Pattern, UE_ARRAY_COUNT(Pattern));
                break;
            }
            case 1: // Channel B - Boom or bust. Net ~-8/12 (many small wins, rare devastating loss)
            {
                const FVendorSandboxProgram::FDeckOutcome Pattern[] = {
                    {12, 0}, {12, 0}, {12, 0}, {12, 0}, {12, 0}, {12, 80},
                    {12, 0}, {12, 0}, {12, 0}, {12, 0}, {12, 0}, {12, 0}
                };
                OutDeck.Append(Pattern, UE_ARRAY_COUNT(Pattern));
                break;
            }
            case 2: // Channel C - Steady safe. Net ~66/12 = 5.5/draw.
            {
                const FVendorSandboxProgram::FDeckOutcome Pattern[] = {
                    {6, 0}, {6, 0}, {6, 0}, {6, 1}, {6, 0}, {6, 0},
                    {6, 0}, {6, 0}, {6, 1}, {6, 0}, {6, 0}, {6, 1}
                };
                OutDeck.Append(Pattern, UE_ARRAY_COUNT(Pattern));
                break;
            }
            case 3: // Channel D - Most reliable. Net ~67/12 = 5.6/draw.
            {
                const FVendorSandboxProgram::FDeckOutcome Pattern[] = {
                    {6, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 1},
                    {6, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 0}, {6, 4}
                };
                OutDeck.Append(Pattern, UE_ARRAY_COUNT(Pattern));
                break;
            }
        }

        Algo::RandomShuffle(OutDeck);

        // Duplicate once so each deck has a longer runway per trial. Copy through a local
        // variable so we do not violate TArray's self-referential Add guard.
        const int32 OriginalCount = OutDeck.Num();
        OutDeck.Reserve(OriginalCount * 2);
        for (int32 Index = 0; Index < OriginalCount; ++Index)
        {
            const FVendorSandboxProgram::FDeckOutcome Copy = OutDeck[Index];
            OutDeck.Add(Copy);
        }
        Algo::RandomShuffle(OutDeck);
    }
}

void FVendorSandboxProgram::GenerateNewTrial()
{
    Score = 0;
    QuotaGoal = 100;
    DrawsRemaining = 16;
    LastReward = 0;
    LastPenalty = 0;
    LastNet = 0;
    StartingScore = Score;
    StartingDraws = DrawsRemaining;
    StatusLine = TEXT("");

    DeckOutcomeSequences.SetNum(4);
    DeckOutcomeIndices.Init(0, 4);
    DeckDrawCounts.Init(0, 4);
    DeckConsecutiveCounts.Init(0, 4);
    DeckPositiveTotals.Init(0, 4);
    DeckNegativeTotals.Init(0, 4);
    DeckLastChange.Init(0.0f, 4);

    for (int32 DeckIndex = 0; DeckIndex < 4; ++DeckIndex)
    {
        BuildDeckPattern(DeckIndex, DeckOutcomeSequences[DeckIndex]);
        DeckOutcomeIndices[DeckIndex] = 0;
        DeckDrawCounts[DeckIndex] = 0;
        DeckConsecutiveCounts[DeckIndex] = 0;
        DeckPositiveTotals[DeckIndex] = 0.0f;
        DeckNegativeTotals[DeckIndex] = 0.0f;
        DeckLastChange[DeckIndex] = 0.0f;
    }

    HoveredDeck = -1;

    InvalidateWidget();
}

FVendorSandboxProgram::FDeckOutcome FVendorSandboxProgram::ConsumeOutcome(int32 DeckIndex)
{
    if (!DeckOutcomeSequences.IsValidIndex(DeckIndex))
    {
        return FDeckOutcome{};
    }

    TArray<FDeckOutcome>& Sequence = DeckOutcomeSequences[DeckIndex];
    int32& SequenceIndex = DeckOutcomeIndices[DeckIndex];

    if (Sequence.Num() == 0)
    {
        return FDeckOutcome{};
    }

    if (SequenceIndex >= Sequence.Num())
    {
        Algo::RandomShuffle(Sequence);
        SequenceIndex = 0;
    }

    const FDeckOutcome Outcome = Sequence[SequenceIndex];
    ++SequenceIndex;
    return Outcome;
}

void FVendorSandboxProgram::SelectDeck(int32 DeckIndex)
{
    if (AreAllTrialsComplete()) return;
    if (DeckIndex < 0 || DeckIndex > 3) return;
    if (DrawsRemaining <= 0) return;

    FDeckOutcome Outcome = ConsumeOutcome(DeckIndex);

    // Fatigue penalty for hammering the same deck repeatedly
    // Gains +2 per use, recovers -1 per non-use (slower recovery)
    for (int32 Index = 0; Index < DeckConsecutiveCounts.Num(); ++Index)
    {
        if (Index == DeckIndex)
        {
            DeckConsecutiveCounts[Index] += 2;
        }
        else
        {
            DeckConsecutiveCounts[Index] = FMath::Max(0, DeckConsecutiveCounts[Index] - 1);
        }
    }

    const int32 FatiguePenalty = FMath::Max(0, DeckConsecutiveCounts[DeckIndex] - 2) * 5;

    LastReward = Outcome.Reward;
    LastPenalty = Outcome.Penalty + FatiguePenalty;
    LastNet = LastReward - LastPenalty;

    Score += LastNet;
    --DrawsRemaining;

    if (DeckDrawCounts.IsValidIndex(DeckIndex))
    {
        DeckDrawCounts[DeckIndex]++;
    }
    if (DeckPositiveTotals.IsValidIndex(DeckIndex))
    {
        DeckPositiveTotals[DeckIndex] += LastReward;
    }
    if (DeckNegativeTotals.IsValidIndex(DeckIndex))
    {
        DeckNegativeTotals[DeckIndex] += LastPenalty;
    }
    if (DeckLastChange.IsValidIndex(DeckIndex))
    {
        DeckLastChange[DeckIndex] = static_cast<float>(LastNet);
    }

    StatusLine = FString();

    if (Score >= QuotaGoal)
    {
        StatusLine = TEXT("");
        if (AdvanceTrial())
        {
            SetTaskComplete(true);
        }
        InvalidateWidget();
        return;
    }

    if (DrawsRemaining <= 0)
    {
        StatusLine = TEXT("");
        FailTrial();
        return;
    }

    InvalidateWidget();
}

FString FVendorSandboxProgram::GetDeckLabel(int32 DeckIndex) const
{
    static const TCHAR* Labels[] = { TEXT("A"), TEXT("B"), TEXT("C"), TEXT("D") };
    return (DeckIndex >= 0 && DeckIndex < 4) ? FString(Labels[DeckIndex]) : FString(TEXT("?"));
}

FVector2D FVendorSandboxProgram::GetDeckCardTopLeft(int32 DeckIndex, const FVector2D& WidgetSize) const
{
    const float TotalWidth = VendorSandboxLayout::CardWidth * 4.0f + VendorSandboxLayout::CardSpacing * 3.0f;
    const float Left = (WidgetSize.X - TotalWidth) * 0.5f + DeckIndex * (VendorSandboxLayout::CardWidth + VendorSandboxLayout::CardSpacing);
    const float Top = WidgetSize.Y * 0.35f - VendorSandboxLayout::CardHeight * 0.5f;
    return FVector2D(Left, Top);
}

int32 FVendorSandboxProgram::GetDeckAtPosition(const FVector2D& LocalPos, const FVector2D& WidgetSize) const
{
    for (int32 DeckIndex = 0; DeckIndex < 4; ++DeckIndex)
    {
        const FVector2D TopLeft = GetDeckCardTopLeft(DeckIndex, WidgetSize);
        const FVector2D Size(VendorSandboxLayout::CardWidth, VendorSandboxLayout::CardHeight);
        if (IsPointInRect(LocalPos, TopLeft, Size))
        {
            return DeckIndex;
        }
    }
    return -1;
}

void FVendorSandboxProgram::InvalidateWidget() const
{
    if (const TSharedPtr<SWidget> Root = ProgramRootWidget.Pin())
    {
        Root->Invalidate(EInvalidateWidget::Paint);
    }
    if (const TSharedPtr<SWidget> Widget = SandboxWidget.Pin())
    {
        Widget->Invalidate(EInvalidateWidget::Paint);
    }
}

TSharedRef<SWidget> FVendorSandboxProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const float SectionPadding = Style.GetPadding() * 1.5f;

    TSharedPtr<SVendorSandboxWidget> Canvas;

    TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin(SectionPadding))
        [SAssignNew(Canvas, SVendorSandboxWidget)
            .PrimaryColor(Style.GetPrimaryColor())
            .DimColor(Style.GetDimColor())
            .LineThickness(Style.LineThickness)
            .Program(this)];

    ProgramRootWidget = Content;
    SandboxWidget = Canvas;
    return Content;
}

void FVendorSandboxProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || AreAllTrialsComplete())
    {
        HoveredDeck = -1;
        return;
    }

    const TSharedPtr<SWidget> Widget = SandboxWidget.Pin();
    if (!Widget.IsValid())
    {
        HoveredDeck = -1;
        return;
    }

    const FGeometry Geometry = Widget->GetCachedGeometry();
    const FVector2D LocalPos = Geometry.AbsoluteToLocal(Event.ScreenPixelPosition);
    const FVector2D Size = Geometry.GetLocalSize();

    HoveredDeck = GetDeckAtPosition(LocalPos, Size);
    UpdateCursorForState(false, HoveredDeck >= 0);

    InvalidateWidget();
}

void FVendorSandboxProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || AreAllTrialsComplete()) return;

    if (HoveredDeck >= 0)
    {
        SelectDeck(HoveredDeck);
    }
}

void FVendorSandboxProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
}

#undef LOCTEXT_NAMESPACE
