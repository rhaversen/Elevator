#include "UI/Programs/FeatureStroopProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

REGISTER_SCREEN_PROGRAM(FFeatureStroopProgram, "feature_stroop")

// ============================================================
// SThemeConsistencyCheckWidget
// ============================================================

void SThemeConsistencyCheckWidget::Construct(const FArguments& InArgs)
{
    Program = InArgs._Program;
}

FVector2D SThemeConsistencyCheckWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return Program ? Program->GetProgramSize() : FVector2D(400.0f, 320.0f);
}

int32 SThemeConsistencyCheckWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
    const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (!Program) return LayerId;

    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FLinearColor Primary = Program->GetProgramStyle().GetPrimaryColor();
    const FLinearColor Dim = Program->GetProgramStyle().GetDimColor().GetSpecifiedColor();
    const float Thickness = Program->GetProgramStyle().LineThickness;
    const FVector2D Center(Size.X * 0.5f, Size.Y * 0.5f);
    const float DragThreshold = Program->GetDragThreshold();

    // Completed state banner
    if (Program->AreAllTrialsComplete())
    {
        const FSlateFontInfo CompleteFont = FCoreStyle::GetDefaultFontStyle("Bold", 18);
        static const FString CompleteText = TEXT("Handshake verified.");
        const FVector2f BannerSize(290.0f, 32.0f);
        const FVector2f BannerPos(Size.X * 0.5f - BannerSize.X * 0.5f, Size.Y * 0.45f - BannerSize.Y * 0.5f);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(BannerSize, FSlateLayoutTransform(BannerPos)),
            CompleteText, CompleteFont, ESlateDrawEffect::None, Primary);
        return LayerId + 1;
    }

    const float ZoneWidth = FMath::Clamp(Size.X * 0.14f, 86.0f, 130.0f);
    const float ZoneHeight = FMath::Clamp(Size.Y * 0.58f, 240.0f, 340.0f);
    const FVector2D ZoneSize(ZoneWidth, ZoneHeight);
    const FVector2D HalfZone = ZoneSize * 0.5f;

    const FVector2D LeftZoneCenter(Center.X - DragThreshold - HalfZone.X, Center.Y);
    const FVector2D RightZoneCenter(Center.X + DragThreshold + HalfZone.X, Center.Y);

    const FVector2D LeftZoneTopLeft = LeftZoneCenter - HalfZone;
    const FVector2D RightZoneTopLeft = RightZoneCenter - HalfZone;

    const bool bHighlightLeft = Program->CardDragOffset < -DragThreshold * 0.55f;
    const bool bHighlightRight = Program->CardDragOffset > DragThreshold * 0.55f;

    // Solid drop zone (left)
    {
        const FLinearColor Fill = bHighlightLeft ? Primary : Dim * 0.32f;
        const FLinearColor Border = bHighlightLeft ? Primary : Dim * 0.65f;
        const FVector2f BoxSize(ZoneSize);
        const FVector2f BoxPos(LeftZoneTopLeft);
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(BoxPos)),
            FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, Fill);

        TArray<FVector2D> BorderPoints;
        BorderPoints.Add(LeftZoneTopLeft);
        BorderPoints.Add(FVector2D(LeftZoneTopLeft.X + ZoneSize.X, LeftZoneTopLeft.Y));
        BorderPoints.Add(LeftZoneTopLeft + ZoneSize);
        BorderPoints.Add(FVector2D(LeftZoneTopLeft.X, LeftZoneTopLeft.Y + ZoneSize.Y));
        BorderPoints.Add(LeftZoneTopLeft);

        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
            BorderPoints, ESlateDrawEffect::None, Border, true, bHighlightLeft ? Thickness * 1.6f : Thickness * 1.1f);
    }

    // Hollow drop zone (right)
    {
        const FLinearColor Outline = bHighlightRight ? Primary : Dim * 0.55f;
        TArray<FVector2D> BoxPoints;
        BoxPoints.Add(RightZoneTopLeft);
        BoxPoints.Add(FVector2D(RightZoneTopLeft.X + ZoneSize.X, RightZoneTopLeft.Y));
        BoxPoints.Add(RightZoneTopLeft + ZoneSize);
        BoxPoints.Add(FVector2D(RightZoneTopLeft.X, RightZoneTopLeft.Y + ZoneSize.Y));
        BoxPoints.Add(RightZoneTopLeft);

        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
            BoxPoints, ESlateDrawEffect::None, Outline, true, bHighlightRight ? Thickness * 1.8f : Thickness * 1.2f);
    }

    // Stimulus symbol
    const float SymbolHoverScale = Program->bIsDragging ? 1.20f : (Program->bCardHovered ? 1.08f : 1.0f);
    const int32 BaseFontSize = 118;
    const int32 FontSize = FMath::Clamp(FMath::RoundToInt(BaseFontSize * SymbolHoverScale), 88, 168);

    FString ArrowStr;
    if (Program->CurrentStimulus.bIsSolid)
    {
        ArrowStr = Program->CurrentStimulus.bArrowIsLeft ? TEXT("\u25C0") : TEXT("\u25B6");
    }
    else
    {
        ArrowStr = Program->CurrentStimulus.bArrowIsLeft ? TEXT("\u25C1") : TEXT("\u25B7");
    }

    const FLinearColor SymbolColor = (Program->bIsDragging || Program->bCardHovered) ? Primary : Dim * 0.68f;

    const float SymbolX = Center.X + Program->CurrentStimulus.OffsetX + Program->CardDragOffset;
    const float SymbolY = Center.Y + Program->CurrentStimulus.OffsetY;

    const FSlateFontInfo ArrowFont = FCoreStyle::GetDefaultFontStyle("Regular", FontSize);
    FVector2D ArrowSize(0.0f, 0.0f);
    if (FSlateApplication::IsInitialized())
    {
        if (FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer())
        {
            if (TSharedPtr<FSlateFontMeasure> FontMeasure = Renderer->GetFontMeasureService())
            {
                ArrowSize = FontMeasure->Measure(ArrowStr, ArrowFont);
            }
        }
    }
    if (ArrowSize.IsNearlyZero())
    {
        const float Approx = static_cast<float>(FontSize);
        ArrowSize = FVector2D(Approx * 0.72f, Approx);
    }

    const FVector2f ArrowPos(SymbolX - ArrowSize.X * 0.5f, SymbolY - ArrowSize.Y * 0.5f);
    FSlateDrawElement::MakeText(OutDrawElements, LayerId + 4,
        AllottedGeometry.ToPaintGeometry(FVector2f(ArrowSize), FSlateLayoutTransform(ArrowPos)),
        ArrowStr, ArrowFont, ESlateDrawEffect::None, SymbolColor);

    return LayerId + 5;
}

// ============================================================
// FFeatureStroopProgram
// ============================================================

FFeatureStroopProgram::FFeatureStroopProgram()
    : FTrialProgramBase(12)
{
    SetTrialTitle(FText::FromString(TEXT("Stroop Task")));
    GenerateNewTrial();
}

TSharedRef<SWidget> FFeatureStroopProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();

    return SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1.0f).Padding(Style.GetLargePadding()).HAlign(HAlign_Fill).VAlign(VAlign_Fill)
        [
            SAssignNew(CanvasWidget, SThemeConsistencyCheckWidget)
            .Program(this)
        ];
}

void FFeatureStroopProgram::GenerateNewTrial()
{
    CurrentStimulus.bIsSolid = FMath::RandBool();
    CurrentStimulus.bArrowIsLeft = FMath::RandBool();
    CurrentStimulus.OffsetX = FMath::FRandRange(-90.0f, 90.0f);
    CurrentStimulus.OffsetY = FMath::FRandRange(-40.0f, 40.0f);

    bIsDragging = false;
    bCardHovered = false;
    CardDragOffset = 0.0f;

    InvalidateWidget();
}

void FFeatureStroopProgram::SubmitResponse(bool bIsLeftResponse)
{
    if (AreAllTrialsComplete()) return;

    const bool bCorrect = (bIsLeftResponse == CurrentStimulus.bIsSolid);
    if (bCorrect)
    {
        AdvanceTrial();
    }
    else
    {
        FailTrial();
    }

    UpdateCursorForState(false, false);
    InvalidateWidget();
}

void FFeatureStroopProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || AreAllTrialsComplete()) return;

    if (Event.TriggerKey == EKeys::Left || Event.TriggerKey == EKeys::A)
    {
        SubmitResponse(true);
        return;
    }
    if (Event.TriggerKey == EKeys::Right || Event.TriggerKey == EKeys::D)
    {
        SubmitResponse(false);
        return;
    }

    const TSharedPtr<SThemeConsistencyCheckWidget> Widget = CanvasWidget.Pin();
    if (!Widget.IsValid()) return;

    const FGeometry& Geometry = Widget->GetCachedGeometry();
    const FVector2D LocalPos = Geometry.AbsoluteToLocal(Event.ScreenPixelPosition);
    const FVector2D WidgetSize = Geometry.GetLocalSize();
    const FVector2D Center(WidgetSize.X * 0.5f, WidgetSize.Y * 0.5f);

    const float HitWidth = 180.0f;
    const float HitHeight = 160.0f;

    const float SymbolX = Center.X + CurrentStimulus.OffsetX + CardDragOffset;
    const float SymbolY = Center.Y + CurrentStimulus.OffsetY;

    const bool bHit = FMath::Abs(LocalPos.X - SymbolX) < HitWidth * 0.5f &&
                      FMath::Abs(LocalPos.Y - SymbolY) < HitHeight * 0.5f;

    if (bHit)
    {
        bIsDragging = true;
        bCardHovered = true;
        DragStartPos = Event.ProgramPixelPosition;
        UpdateCursorForState(true, false);
        InvalidateWidget();
    }
}

void FFeatureStroopProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (AreAllTrialsComplete()) return;

    if (bIsDragging)
    {
        CardDragOffset = Event.ProgramPixelPosition.X - DragStartPos.X;
        bCardHovered = true;
        InvalidateWidget();
    }
    else
    {
        const TSharedPtr<SThemeConsistencyCheckWidget> Widget = CanvasWidget.Pin();
        if (!Widget.IsValid()) return;

        const FGeometry& Geometry = Widget->GetCachedGeometry();
        const FVector2D LocalPos = Geometry.AbsoluteToLocal(Event.ScreenPixelPosition);
        const FVector2D WidgetSize = Geometry.GetLocalSize();
        const FVector2D Center(WidgetSize.X * 0.5f, WidgetSize.Y * 0.5f);

        const float HitWidth = 180.0f;
        const float HitHeight = 160.0f;

        const float SymbolX = Center.X + CurrentStimulus.OffsetX + CardDragOffset;
        const float SymbolY = Center.Y + CurrentStimulus.OffsetY;

        const bool bOverCard = FMath::Abs(LocalPos.X - SymbolX) < HitWidth * 0.5f &&
                               FMath::Abs(LocalPos.Y - SymbolY) < HitHeight * 0.5f;

        if (bCardHovered != bOverCard)
        {
            bCardHovered = bOverCard;
            InvalidateWidget();
        }

        UpdateCursorForState(false, bOverCard);
    }
}

void FFeatureStroopProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (!bIsDragging || AreAllTrialsComplete())
    {
        bIsDragging = false;
        UpdateCursorForState(false, bCardHovered);
        return;
    }

    const float DragThreshold = GetDragThreshold();

    if (CardDragOffset < -DragThreshold)
    {
        SubmitResponse(true);
    }
    else if (CardDragOffset > DragThreshold)
    {
        SubmitResponse(false);
    }
    else
    {
        CardDragOffset = 0.0f;
        InvalidateWidget();
    }

    bIsDragging = false;
    bCardHovered = false;
    UpdateCursorForState(false, false);
}

void FFeatureStroopProgram::InvalidateWidget() const
{
    if (CanvasWidget.IsValid())
    {
        CanvasWidget.Pin()->Invalidate(EInvalidateWidget::Paint);
    }
}

float FFeatureStroopProgram::GetDragThreshold() const
{
    const float Base = GetProgramSize().X * 0.22f;
    return FMath::Clamp(Base, 150.0f, 260.0f);
}

