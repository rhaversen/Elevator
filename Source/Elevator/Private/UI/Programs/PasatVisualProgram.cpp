#include "UI/Programs/PasatVisualProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

REGISTER_SCREEN_PROGRAM(FPasatVisualProgram, "pasat_visual")

// ============================================================================
// SBudgetTickerWidget Implementation
// ============================================================================

void SBudgetTickerWidget::Construct(const FArguments& InArgs)
{
    PrimaryColor = InArgs._PrimaryColor;
    DimColor = InArgs._DimColor;
    LineThickness = InArgs._LineThickness;
    Program = InArgs._Program;
}

FVector2D SBudgetTickerWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return FVector2D::ZeroVector;
}

int32 SBudgetTickerWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (!Program) return LayerId;
    
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FLinearColor Primary = PrimaryColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const FLinearColor Dim = DimColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const float Thickness = LineThickness.Get();
    const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
    
    const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    const int32 SmallFontSize = 14;
    const int32 TickerFontSize = 80;
    const int32 MediumFontSize = 36;
    const FSlateFontInfo SmallFont = FCoreStyle::GetDefaultFontStyle("Mono", SmallFontSize);
    const FSlateFontInfo TickerFont = FCoreStyle::GetDefaultFontStyle("Mono", TickerFontSize);
    const FSlateFontInfo MediumFont = FCoreStyle::GetDefaultFontStyle("Bold", MediumFontSize);
    
    // Layout constants
    const float CenterX = Size.X * 0.5f;
    
    // Ticker layout - larger and more central
    const float TickerHeight = 140.0f;
    const float TickerY = (Size.Y - TickerHeight) * 0.4f;
    
    const float OptionsY = Size.Y * 0.75f;
    const float OptionWidth = Size.X / 5.0f;
    const float OptionHeight = 80.0f;
    
    // === Draw ticker tape header (like stock ticker) ===
    {
        // Ticker tape background lines (top and bottom)
        const float TapeTop = TickerY;
        const float TapeBottom = TickerY + TickerHeight;
        const float TapeLeft = 0.0f;
        const float TapeRight = Size.X;
        
        // Top line with sprocket holes
        TArray<FVector2D> TopLine;
        TopLine.Add(FVector2D(TapeLeft, TapeTop));
        TopLine.Add(FVector2D(TapeRight, TapeTop));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            TopLine, DrawEffects, Dim * 0.6f, false, Thickness * 2.0f);
        
        // Bottom line
        TArray<FVector2D> BottomLine;
        BottomLine.Add(FVector2D(TapeLeft, TapeBottom));
        BottomLine.Add(FVector2D(TapeRight, TapeBottom));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            BottomLine, DrawEffects, Dim * 0.6f, false, Thickness * 2.0f);
        
        // Draw sprocket holes along top and bottom edge
        const float HoleSize = 8.0f;
        const float HoleSpacing = 30.0f;
        const float TopHoleY = TapeTop + 10.0f;
        const float BottomHoleY = TapeBottom - 10.0f - HoleSize;
        
        for (float x = TapeLeft + 15.0f; x < TapeRight; x += HoleSpacing)
        {
            // Top hole
            TArray<FVector2D> HoleT;
            HoleT.Add(FVector2D(x, TopHoleY));
            HoleT.Add(FVector2D(x + HoleSize, TopHoleY));
            HoleT.Add(FVector2D(x + HoleSize, TopHoleY + HoleSize));
            HoleT.Add(FVector2D(x, TopHoleY + HoleSize));
            HoleT.Add(FVector2D(x, TopHoleY));
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                HoleT, DrawEffects, Dim * 0.3f, true, Thickness);
                
            // Bottom hole
            TArray<FVector2D> HoleB;
            HoleB.Add(FVector2D(x, BottomHoleY));
            HoleB.Add(FVector2D(x + HoleSize, BottomHoleY));
            HoleB.Add(FVector2D(x + HoleSize, BottomHoleY + HoleSize));
            HoleB.Add(FVector2D(x, BottomHoleY + HoleSize));
            HoleB.Add(FVector2D(x, BottomHoleY));
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                HoleB, DrawEffects, Dim * 0.3f, true, Thickness);
        }
        
        // Draw ALL numbers in the array - rightmost is newest
        const float NumCenterY = TapeTop + (TickerHeight * 0.5f);
        const float TapeWidth = TapeRight - TapeLeft - 40.0f;  // Leave some margin
        const int32 NumCount = Program->NumberHistory.Num();
        
        // Calculate spacing based on how many numbers we have
        // Start with fixed spacing, shrink if needed to fit all numbers
        const float MaxSpacing = 90.0f;
        const float MinSpacing = 50.0f;
        float NumSpacing = FMath::Max(MinSpacing, FMath::Min(MaxSpacing, TapeWidth / FMath::Max(1, NumCount)));
        
        // Position numbers from left to right, oldest to newest
        const float StartX = TapeLeft + 40.0f;
        
        for (int32 i = 0; i < NumCount; ++i)
        {
            const int32 Number = Program->NumberHistory[i];
            const float NumX = StartX + i * NumSpacing;
            
            // Calculate alpha - last two are bright, others fade
            float Alpha;
            const int32 DistFromEnd = NumCount - 1 - i;  // 0 = newest, 1 = second newest, etc.
            if (DistFromEnd == 0)
            {
                Alpha = 1.0f;  // Current number (newest)
            }
            else if (DistFromEnd == 1)
            {
                Alpha = 0.8f;  // Previous number
            }
            else
            {
                // Fade older numbers
                Alpha = FMath::Max(0.2f, 0.5f - (DistFromEnd - 2) * 0.1f);
            }
            
            const FString NumStr = FString::Printf(TEXT("%d"), Number);
            const FVector2D NumSize = FontMeasure->Measure(NumStr, TickerFont);
            const FVector2D NumPos(NumX - NumSize.X * 0.5f, NumCenterY - NumSize.Y * 0.5f);
            FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, 
                AllottedGeometry.ToPaintGeometry(FVector2f(NumSize), FSlateLayoutTransform(FVector2f(NumPos))),
                NumStr, TickerFont, DrawEffects, Primary * Alpha);
        }
        
        // Bracket under the last two numbers
        if (NumCount >= 2)
        {
            const float BracketY = TapeBottom + 15.0f;
            const float BracketHeight = 15.0f;
            const float SecondLastX = StartX + (NumCount - 2) * NumSpacing;
            const float LastX = StartX + (NumCount - 1) * NumSpacing;
            
            // Draw bracket under the two numbers to add
            TArray<FVector2D> Bracket;
            Bracket.Add(FVector2D(SecondLastX - 25.0f, BracketY));
            Bracket.Add(FVector2D(SecondLastX - 25.0f, BracketY + BracketHeight));
            Bracket.Add(FVector2D(LastX + 25.0f, BracketY + BracketHeight));
            Bracket.Add(FVector2D(LastX + 25.0f, BracketY));
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                Bracket, DrawEffects, Primary, false, Thickness * 2.0f);
        }
    }
    
    // === Draw sum options ===
    {
        for (int32 i = 0; i < 5; ++i)
        {
            const float OptLeft = i * OptionWidth;
            const float OptTop = OptionsY;
            const float OptCenterX = OptLeft + OptionWidth * 0.5f;
            const float OptCenterY = OptTop + OptionHeight * 0.5f;
            const bool bHovered = (Program->HoveredOptionIndex == i);
            
            const float BoxPadding = 8.0f;
            const FVector2D BoxTopLeft(OptLeft + BoxPadding, OptTop);
            const FVector2D BoxBottomRight(OptLeft + OptionWidth - BoxPadding, OptTop + OptionHeight);
            
            // Fill if hovered
            if (bHovered)
            {
                const FPaintGeometry FillGeom = AllottedGeometry.ToPaintGeometry(
                    FVector2f(BoxBottomRight.X - BoxTopLeft.X, BoxBottomRight.Y - BoxTopLeft.Y),
                    FSlateLayoutTransform(FVector2f(BoxTopLeft))
                );
                FSlateDrawElement::MakeBox(OutDrawElements, LayerId, FillGeom,
                    FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, Primary);
            }
            
            // Draw box outline
            TArray<FVector2D> BoxPoints;
            BoxPoints.Add(BoxTopLeft);
            BoxPoints.Add(FVector2D(BoxBottomRight.X, BoxTopLeft.Y));
            BoxPoints.Add(BoxBottomRight);
            BoxPoints.Add(FVector2D(BoxTopLeft.X, BoxBottomRight.Y));
            BoxPoints.Add(BoxTopLeft);
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                BoxPoints, DrawEffects, bHovered ? FLinearColor::Black : Primary, true, Thickness);
            
            // Draw number
            const int32 OptionValue = Program->SumOptions.IsValidIndex(i) ? Program->SumOptions[i] : 0;
            const FString NumStr = FString::Printf(TEXT("%d"), OptionValue);
            const FVector2D NumSize = FontMeasure->Measure(NumStr, MediumFont);
            const FVector2D NumPos(OptCenterX - NumSize.X * 0.5f, OptCenterY - NumSize.Y * 0.5f);
            FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2, 
                AllottedGeometry.ToPaintGeometry(FVector2f(NumSize), FSlateLayoutTransform(FVector2f(NumPos))),
                NumStr, MediumFont, DrawEffects, bHovered ? FLinearColor::Black : Primary);
        }
    }
    
    return LayerId + 3;
}

// ============================================================================
// FPasatVisualProgram Implementation
// ============================================================================

FPasatVisualProgram::FPasatVisualProgram()
    : FTrialProgramBase(12)  // 12 trials
{
    SetTrialTitle(FText::FromString(TEXT("Paced Visual Serial Addition Test")));
    // Initialize ticker with a sequence of random single-digit numbers
    // Start with just 2 numbers as requested
    NumberHistory.Empty();
    for (int32 i = 0; i < 2; ++i)
    {
        NumberHistory.Add(FMath::RandRange(1, 9));
    }
    
    GenerateNewTrial();
}

void FPasatVisualProgram::GenerateNewTrial()
{
    // The two numbers to add are the last two in the history
    const int32 PreviousNumber = NumberHistory.Num() >= 2 ? NumberHistory[NumberHistory.Num() - 2] : 0;
    const int32 CurrentNumber = NumberHistory.Num() >= 1 ? NumberHistory.Last() : 0;
    
    // Generate 5 options including the correct sum
    const int32 CorrectSum = PreviousNumber + CurrentNumber;
    SumOptions.Empty();
    SumOptions.Add(CorrectSum - 2);
    SumOptions.Add(CorrectSum - 1);
    SumOptions.Add(CorrectSum);
    SumOptions.Add(CorrectSum + 1);
    SumOptions.Add(CorrectSum + 2);
    
    // Shuffle options so correct answer isn't always in the middle
    for (int32 i = SumOptions.Num() - 1; i > 0; --i)
    {
        const int32 j = FMath::RandRange(0, i);
        SumOptions.Swap(i, j);
    }
    
    InvalidateWidget();
}

void FPasatVisualProgram::AdvanceToNextNumber()
{
    // Simply add a new random number to the array
    NumberHistory.Add(FMath::RandRange(1, 9));
}

int32 FPasatVisualProgram::GetSumButtonAtPosition(const FVector2D& LocalPos, const FVector2D& WidgetSize) const
{
    // Match layout from OnPaint exactly
    const float OptionsY = WidgetSize.Y * 0.75f;
    const float OptionWidth = WidgetSize.X / 5.0f;
    const float OptionHeight = 80.0f;
    const float BoxPadding = 8.0f;
    
    // Check Y bounds
    if (LocalPos.Y < OptionsY || LocalPos.Y > OptionsY + OptionHeight)
    {
        return -1;
    }
    
    // Check which column (accounting for padding)
    for (int32 i = 0; i < 5; ++i)
    {
        const float OptLeft = i * OptionWidth + BoxPadding;
        const float OptRight = (i + 1) * OptionWidth - BoxPadding;
        if (LocalPos.X >= OptLeft && LocalPos.X <= OptRight)
        {
            return i;
        }
    }
    
    return -1;
}

void FPasatVisualProgram::EnterSum(int32 OptionIndex)
{
    if (AreAllTrialsComplete()) return;
    if (!SumOptions.IsValidIndex(OptionIndex)) return;
    
    const int32 SelectedSum = SumOptions[OptionIndex];
    const int32 PreviousNumber = NumberHistory.Num() >= 2 ? NumberHistory[NumberHistory.Num() - 2] : 0;
    const int32 CurrentNumber = NumberHistory.Num() >= 1 ? NumberHistory.Last() : 0;
    const int32 CorrectSum = PreviousNumber + CurrentNumber;
    
    if (SelectedSum == CorrectSum)
    {
        // Correct! First advance the trial
        const bool bAllComplete = AdvanceTrial();
        if (bAllComplete)
        {
            SetTaskComplete(true);
        }
        else
        {
            // Not complete yet - add a new number to the ticker for the next trial
            AdvanceToNextNumber();
            // Generate new answer options for the new pair
            GenerateNewTrial();
        }
    }
    else
    {
        FailTrial();
        // On failure, clear and reset the array to two new numbers
        NumberHistory.Empty();
        NumberHistory.Add(FMath::RandRange(1, 9));
        NumberHistory.Add(FMath::RandRange(1, 9));
        GenerateNewTrial();
    }
    
    InvalidateWidget();
}

void FPasatVisualProgram::InvalidateWidget() const
{
    if (const TSharedPtr<SWidget> Widget = TickerWidget.Pin())
    {
        Widget->Invalidate(EInvalidateWidget::Paint);
    }
}

TSharedRef<SWidget> FPasatVisualProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();

    TSharedRef<SBudgetTickerWidget> Ticker = SNew(SBudgetTickerWidget)
        .PrimaryColor(Style.GetPrimaryColor())
        .DimColor(Style.GetDimColor())
        .LineThickness(Style.LineThickness)
        .Program(this);

    TickerWidget = Ticker;
    return Ticker;
}

void FPasatVisualProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || AreAllTrialsComplete())
    {
        HoveredOptionIndex = -1;
        return;
    }
    
    const TSharedPtr<SWidget> Widget = TickerWidget.Pin();
    if (!Widget.IsValid()) return;
    
    const FGeometry& Geometry = Widget->GetCachedGeometry();
    const FVector2D LocalPos = Geometry.AbsoluteToLocal(Event.ScreenPixelPosition);
    const FVector2D WidgetSize = Geometry.GetLocalSize();
    
    const int32 NewHovered = GetSumButtonAtPosition(LocalPos, WidgetSize);
    if (NewHovered != HoveredOptionIndex)
    {
        HoveredOptionIndex = NewHovered;
        InvalidateWidget();
    }
    
    UpdateCursorForState(false, HoveredOptionIndex >= 0);
}

void FPasatVisualProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || AreAllTrialsComplete()) return;
    
    ActivePointerKey = Event.TriggerKey;
    
    if (HoveredOptionIndex >= 0)
    {
        EnterSum(HoveredOptionIndex);
    }
}

void FPasatVisualProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    ActivePointerKey = EKeys::Invalid;
}

