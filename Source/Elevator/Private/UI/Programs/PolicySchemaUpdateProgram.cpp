#include "UI/Programs/PolicySchemaUpdateProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"

#define LOCTEXT_NAMESPACE "PolicySchemaUpdateProgram"

REGISTER_SCREEN_PROGRAM(FPolicySchemaUpdateProgram, "PolicySchemaUpdate")

namespace
{
    constexpr int32 CircleSegments = 24;
    constexpr double RecalibrateDelaySeconds = 0.8;
    
    // Draw a shape at the given center position
    // Shape: 0=Triangle, 1=Square, 2=Circle, 3=Diamond
    // Fill: 0=Solid, 1=Dither, 2=Outline
    void DrawShape(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
        const FVector2D& Center, float Size, int32 Shape, int32 Fill, const FLinearColor& Color, float Thickness)
    {
        const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
        TArray<FVector2D> Points;
        
        switch (Shape)
        {
        case 0: // Triangle
            Points.Add(Center + FVector2D(0, -Size * 0.5f));
            Points.Add(Center + FVector2D(-Size * 0.43f, Size * 0.35f));
            Points.Add(Center + FVector2D(Size * 0.43f, Size * 0.35f));
            Points.Add(Center + FVector2D(0, -Size * 0.5f)); // Close
            break;
        case 1: // Square
            Points.Add(Center + FVector2D(-Size * 0.4f, -Size * 0.4f));
            Points.Add(Center + FVector2D(Size * 0.4f, -Size * 0.4f));
            Points.Add(Center + FVector2D(Size * 0.4f, Size * 0.4f));
            Points.Add(Center + FVector2D(-Size * 0.4f, Size * 0.4f));
            Points.Add(Center + FVector2D(-Size * 0.4f, -Size * 0.4f)); // Close
            break;
        case 2: // Circle
            for (int32 i = 0; i <= CircleSegments; ++i)
            {
                const float Angle = (static_cast<float>(i) / CircleSegments) * 2.0f * PI;
                Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Size * 0.4f);
            }
            break;
        case 3: // Diamond
        default:
            Points.Add(Center + FVector2D(0, -Size * 0.5f));
            Points.Add(Center + FVector2D(Size * 0.35f, 0));
            Points.Add(Center + FVector2D(0, Size * 0.5f));
            Points.Add(Center + FVector2D(-Size * 0.35f, 0));
            Points.Add(Center + FVector2D(0, -Size * 0.5f)); // Close
            break;
        }
        
        // Draw outline
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            Geometry.ToPaintGeometry(),
            Points,
            DrawEffects,
            Color,
            true,
            Thickness
        );
        
        // For solid fill, draw additional inner lines (dense hatching)
        if (Fill == 0)
        {
            // Draw horizontal lines for solid effect
            const float HatchSpacing = Size * 0.08f;
            for (float Y = Center.Y - Size * 0.25f; Y <= Center.Y + Size * 0.25f; Y += HatchSpacing)
            {
                TArray<FVector2D> HatchLine;
                HatchLine.Add(FVector2D(Center.X - Size * 0.2f, Y));
                HatchLine.Add(FVector2D(Center.X + Size * 0.2f, Y));
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    HatchLine, DrawEffects, Color * 0.7f, true, Thickness * 0.5f);
            }
        }
        else if (Fill == 1)
        {
            // Dither: draw a small dot (avoids confusion with shapes)
            TArray<FVector2D> DotH, DotV;
            DotH.Add(Center + FVector2D(-Size * 0.03f, 0));
            DotH.Add(Center + FVector2D(Size * 0.03f, 0));
            DotV.Add(Center + FVector2D(0, -Size * 0.03f));
            DotV.Add(Center + FVector2D(0, Size * 0.03f));
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                DotH, DrawEffects, Color * 0.7f, true, Thickness * 1.5f);
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                DotV, DrawEffects, Color * 0.7f, true, Thickness * 1.5f);
        }
        // Fill == 2 is outline only (empty interior), already drawn
    }
    
    // Draw a card border and shapes inside
    void DrawCard(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
        const FVector2D& TopLeft, const FVector2D& CardSize, int32 Shape, int32 Fill, int32 Count,
        const FLinearColor& Color, float Thickness, bool bHighlight)
    {
        const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
        
        // Draw card border
        TArray<FVector2D> Border;
        Border.Add(TopLeft);
        Border.Add(TopLeft + FVector2D(CardSize.X, 0));
        Border.Add(TopLeft + CardSize);
        Border.Add(TopLeft + FVector2D(0, CardSize.Y));
        Border.Add(TopLeft);
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            Geometry.ToPaintGeometry(),
            Border,
            DrawEffects,
            bHighlight ? Color : Color * 0.5f,
            true,
            bHighlight ? Thickness * 1.5f : Thickness
        );
        
        // Draw shapes inside card
        const float ShapeSize = FMath::Min(CardSize.X / (Count + 0.5f), CardSize.Y * 0.7f);
        const float TotalWidth = ShapeSize * Count;
        const float StartX = TopLeft.X + (CardSize.X - TotalWidth) * 0.5f + ShapeSize * 0.5f;
        const float CenterY = TopLeft.Y + CardSize.Y * 0.5f;
        
        for (int32 i = 0; i < Count; ++i)
        {
            const FVector2D ShapeCenter(StartX + i * ShapeSize, CenterY);
            DrawShape(OutDrawElements, LayerId + 1, Geometry, ShapeCenter, ShapeSize, Shape, Fill,
                bHighlight ? Color : Color * 0.5f, Thickness);
        }
    }
}

// ============================================================================
// SPolicyCardWidget Implementation
// ============================================================================

void SPolicyCardWidget::Construct(const FArguments& InArgs)
{
    PrimaryColor = InArgs._PrimaryColor;
    DimColor = InArgs._DimColor;
    LineThickness = InArgs._LineThickness;
    Program = InArgs._Program;
}

FVector2D SPolicyCardWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return FVector2D(400.0f, 300.0f);
}

int32 SPolicyCardWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (!Program) return LayerId;
    
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FLinearColor Primary = PrimaryColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const FLinearColor Dim = DimColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const float Thickness = LineThickness.Get();
    const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
    
    // Layout: Current card at top (30% height), four piles below (70% height)
    const float CardAreaHeight = Size.Y * 0.35f;
    const float PileAreaTop = CardAreaHeight;
    const float PileAreaHeight = Size.Y - PileAreaTop;
    const float PileWidth = Size.X / 4.0f;
    
    // Draw current card centered at top
    const FVector2D CurrentCardSize(Size.X * 0.4f, CardAreaHeight * 0.8f);
    const FVector2D CurrentCardTopLeft((Size.X - CurrentCardSize.X) * 0.5f, CardAreaHeight * 0.1f);
    
    DrawCard(OutDrawElements, LayerId, AllottedGeometry,
        CurrentCardTopLeft, CurrentCardSize,
        Program->CurrentCard.Shape, Program->CurrentCard.Fill, Program->CurrentCard.Number,
        Primary, Thickness, true);
    
    // Draw four pile slots with exemplar cards
    for (int32 i = 0; i < 4; ++i)
    {
        const bool bHovered = (Program->HoveredPile == i);
        const FLinearColor PileColor = bHovered ? Primary : Dim;
        
        const FVector2D PileTopLeft(i * PileWidth + PileWidth * 0.05f, PileAreaTop + PileAreaHeight * 0.05f);
        const FVector2D PileSize(PileWidth * 0.9f, PileAreaHeight * 0.9f);
        
        // Draw exemplar card inside pile (single border, no outer pile border)
        if (i < Program->ExemplarCards.Num())
        {
            const auto& Exemplar = Program->ExemplarCards[i];
            
            DrawCard(OutDrawElements, LayerId + 2, AllottedGeometry,
                PileTopLeft, PileSize,
                Exemplar.Shape, Exemplar.Fill, Exemplar.Number,
                PileColor, Thickness, bHovered);
        }
    }
    
    return LayerId + 4;
}

// ============================================================================
// FPolicySchemaUpdateProgram Implementation
// ============================================================================

FPolicySchemaUpdateProgram::FPolicySchemaUpdateProgram()
    : FTrialProgramBase(4)  // 4 trials (rule shifts) with longer calibration each
{
    SetTrialTitle(LOCTEXT("PolicySchemaUpdateTitle", "Policy Schema Update"));
    // Set up exemplar cards for each pile - each unique in shape, fill, and number
    // Pile 0: 1x Triangle Solid
    // Pile 1: 2x Square Dither  
    // Pile 2: 3x Circle Outline
    // Pile 3: 4x Diamond Solid
    ExemplarCards.Add({0, 0, 1});  // Triangle, Solid, 1
    ExemplarCards.Add({1, 1, 2});  // Square, Dither, 2
    ExemplarCards.Add({2, 2, 3});  // Circle, Outline, 3
    ExemplarCards.Add({3, 0, 4});  // Diamond, Solid, 4
    
    GenerateCard();
}

void FPolicySchemaUpdateProgram::GenerateNewTrial()
{
    GenerateCard();
}

void FPolicySchemaUpdateProgram::GenerateCard()
{
    CurrentCard = {FMath::RandRange(0, 3), FMath::RandRange(0, 2), FMath::RandRange(1, 4)};
    bReadjusting = false;
    InvalidateWidget();
}

bool FPolicySchemaUpdateProgram::IsCorrectPile(int32 PileIndex) const
{
    if (PileIndex < 0 || PileIndex >= ExemplarCards.Num()) return false;
    
    const auto& Exemplar = ExemplarCards[PileIndex];
    switch (CurrentRule)
    {
        case ESortRule::Shape: return CurrentCard.Shape == Exemplar.Shape;
        case ESortRule::Fill: return CurrentCard.Fill == Exemplar.Fill;
        case ESortRule::Number: return CurrentCard.Number == Exemplar.Number;
    }
    return false;
}

int32 FPolicySchemaUpdateProgram::GetPileAtPosition(const FVector2D& NormalizedPos) const
{
    // Piles are in bottom portion of widget (from 35% to ~95% height)
    // PileAreaTop = 0.35, PileAreaHeight = 0.65, piles have 5% margin top/bottom
    const float PileTop = 0.35f + 0.65f * 0.05f;    // ~0.3825
    const float PileBottom = 0.35f + 0.65f * 0.95f; // ~0.9675
    
    if (NormalizedPos.Y < PileTop || NormalizedPos.Y > PileBottom) return -1;
    if (NormalizedPos.X < 0.0f || NormalizedPos.X > 1.0f) return -1;
    
    // Check X bounds within pile columns (each pile has 5% margin on sides)
    const float PileWidthNorm = 0.25f; // 1/4 of width
    const int32 PileIndex = FMath::Clamp(static_cast<int32>(NormalizedPos.X * 4.0f), 0, 3);
    
    // Each pile occupies column from (i*0.25 + 0.0125) to (i*0.25 + 0.2375)
    const float PileLeft = PileIndex * PileWidthNorm + PileWidthNorm * 0.05f;
    const float PileRight = PileIndex * PileWidthNorm + PileWidthNorm * 0.95f;
    
    if (NormalizedPos.X < PileLeft || NormalizedPos.X > PileRight) return -1;
    
    return PileIndex;
}

void FPolicySchemaUpdateProgram::DropOnPile(int32 PileIndex)
{
    if (PileIndex < 0 || PileIndex >= 4) return;
    if (bReadjusting) return;  // Ignore input during recalibrating phase
    
    bool bCorrect = IsCorrectPile(PileIndex);
    
    if (bCorrect)
    {
        CorrectStreak++;
        // Need a streak of 5 correct answers to "calibrate" and complete the trial
        if (CorrectStreak >= 5)
        {
            bReadjusting = true;  // Show readjusting state with full bar
            
            // Determine next rule (different from current)
            PendingNextRule = CurrentRule;
            while (PendingNextRule == CurrentRule)
            {
                PendingNextRule = static_cast<ESortRule>(FMath::RandRange(0, 2));
            }
            
            // Set timer for delayed advance
            PendingAdvanceTime = FPlatformTime::Seconds() + RecalibrateDelaySeconds;
            InvalidateWidget();
        }
        else
        {
            GenerateCard();  // Continue calibration phase
        }
    }
    else
    {
        CorrectStreak = 0;
        GenerateCard();  // Reset streak on failure, regenerate card
    }
}

void FPolicySchemaUpdateProgram::ProcessPendingAdvance()
{
    if (!bReadjusting) return;
    
    const double Now = FPlatformTime::Seconds();
    if (Now < PendingAdvanceTime)
    {
        InvalidateWidget();  // Keep refreshing to show recalibrating
        return;
    }
    
    // Timer elapsed - advance to next trial
    CurrentRule = PendingNextRule;
    CorrectStreak = 0;
    bReadjusting = false;
    
    AdvanceTrial();  // Handles completion check and calls GenerateNewTrial
}

void FPolicySchemaUpdateProgram::InvalidateWidget() const
{
    if (const TSharedPtr<SWidget> Widget = CardWidget.Pin())
    {
        Widget->Invalidate(EInvalidateWidget::Paint);
    }
}

TSharedRef<SWidget> FPolicySchemaUpdateProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const int32 LargeTextSize = Style.TextSize * 2;

    TSharedPtr<SPolicyCardWidget> CardArea;

    TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
        // Sync meter (large, prominent)
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, Style.GetSmallPadding())
        [SNew(STextBlock)
            .Text_Lambda([this]() {
                if (GetTaskComplete()) return FText::FromString(TEXT("|#####|"));

                FString SyncMeter;
                for (int32 i = 0; i < 5; ++i)
                {
                    SyncMeter += (i < CorrectStreak) ? TEXT("#") : TEXT(".");
                }

                if (bReadjusting) return FText::FromString(TEXT("|#####|"));

                return FText::FromString(FString::Printf(TEXT("|%s|"), *SyncMeter));
            })
            .Font(FCoreStyle::GetDefaultFontStyle("Mono", LargeTextSize))
            .ColorAndOpacity(Style.GetPrimaryColor())]

        // Status label (smaller, below)
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, Style.GetSmallPadding())
        [SNew(STextBlock)
            .Text_Lambda([this]() {
                if (GetTaskComplete()) return FText::FromString(TEXT("LOCKED"));
                if (bReadjusting) return FText::FromString(TEXT("RECALIBRATING"));
                return FText::FromString(TEXT("SYNCING"));
            })
            .Font(FCoreStyle::GetDefaultFontStyle("Mono", Style.TextSize))
            .ColorAndOpacity(Style.GetPrimaryColor())]

        // Card area with custom drawing
        + SVerticalBox::Slot().FillHeight(1.0f).Padding(Style.GetSmallPadding())
        [SAssignNew(CardArea, SPolicyCardWidget)
            .PrimaryColor(Style.GetPrimaryColor())
            .DimColor(Style.GetDimColor())
            .LineThickness(Style.LineThickness)
            .Program(this)];

    CardWidget = CardArea;
    return Content;
}

void FPolicySchemaUpdateProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || GetTaskComplete()) return;
    
    // Check if pending advance timer has elapsed
    ProcessPendingAdvance();
    
    // Get normalized position within the card widget area
    const TSharedPtr<SWidget> Widget = CardWidget.Pin();
    if (!Widget.IsValid()) return;
    
    const FGeometry& Geometry = Widget->GetCachedGeometry();
    const FVector2D LocalPos = Geometry.AbsoluteToLocal(Event.ScreenPixelPosition);
    const FVector2D WidgetSize = Geometry.GetLocalSize();
    
    if (WidgetSize.X > KINDA_SMALL_NUMBER && WidgetSize.Y > KINDA_SMALL_NUMBER)
    {
        const FVector2D NormalizedPos(
            FMath::Clamp(LocalPos.X / WidgetSize.X, 0.0f, 1.0f),
            FMath::Clamp(LocalPos.Y / WidgetSize.Y, 0.0f, 1.0f)
        );
        
        const int32 NewHovered = GetPileAtPosition(NormalizedPos);
        if (NewHovered != HoveredPile)
        {
            HoveredPile = NewHovered;
            InvalidateWidget();
        }
    }
    
    UpdateCursorForState(false, HoveredPile >= 0);
}

void FPolicySchemaUpdateProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || GetTaskComplete()) return;
    
    if (HoveredPile >= 0)
    {
        DropOnPile(HoveredPile);
    }
}

void FPolicySchemaUpdateProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    // Nothing special on release
}

#undef LOCTEXT_NAMESPACE
