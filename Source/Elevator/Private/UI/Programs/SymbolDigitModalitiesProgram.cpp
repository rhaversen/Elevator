#include "UI/Programs/SymbolDigitModalitiesProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

REGISTER_SCREEN_PROGRAM(FSymbolDigitModalitiesProgram, "symbol_digit_modalities")

// ============================================================================
// SCodeSheetWidget Implementation
// ============================================================================

void SCodeSheetWidget::Construct(const FArguments& InArgs)
{
    PrimaryColor = InArgs._PrimaryColor;
    DimColor = InArgs._DimColor;
    LineThickness = InArgs._LineThickness;
    Program = InArgs._Program;
}

FVector2D SCodeSheetWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return FVector2D::ZeroVector;
}

int32 SCodeSheetWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (!Program) return LayerId;
    
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FLinearColor Primary = PrimaryColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const FLinearColor Dim = DimColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const float Thickness = LineThickness.Get();
    const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
    
    const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    const int32 KeyFontSize = 38;
    const int32 DigitFontSize = 32;
    const int32 CurrentSymbolFontSize = 72;
    const int32 KeypadFontSize = 36;
    const FSlateFontInfo KeyFont = FCoreStyle::GetDefaultFontStyle("Bold", KeyFontSize);
    const FSlateFontInfo DigitFont = FCoreStyle::GetDefaultFontStyle("Bold", DigitFontSize);
    const FSlateFontInfo CurrentSymbolFont = FCoreStyle::GetDefaultFontStyle("Bold", CurrentSymbolFontSize);
    const FSlateFontInfo KeypadFont = FCoreStyle::GetDefaultFontStyle("Bold", KeypadFontSize);
    
    // Layout constants
    const float KeyRowY = 35.0f;
    const float KeyCellWidth = Size.X / 11.0f;  // Tighter spacing between keys
    const float KeyStartX = (Size.X - KeyCellWidth * 9.0f) * 0.5f;  // Center the keys
    const float KeySymbolRowY = KeyRowY;
    const float KeyDigitRowY = KeyRowY + 55.0f;
    
    const float CurrentSymbolY = Size.Y * 0.32f;
    const float CurrentSymbolWidth = 80.0f;  // Fixed width to prevent layout shift
    
    const float KeypadWidth = Size.X * 0.85f;
    const float KeypadHeight = Size.Y * 0.42f;
    const float KeypadLeft = (Size.X - KeypadWidth) * 0.5f;
    const float KeypadTop = Size.Y - KeypadHeight - 15.0f;
    
    // === Draw Key (two horizontal rows: symbols on top, digits below) ===
    {
        // Draw frame around key legend
        const float FramePadding = 12.0f;
        const float FrameTop = KeyRowY - FramePadding;
        const float FrameBottom = KeyDigitRowY + DigitFontSize + FramePadding + 10.0f;  // Extra space below digits
        const float FrameLeft = KeyStartX - FramePadding;
        const float FrameRight = KeyStartX + (KeyCellWidth * 9.0f) + FramePadding;
        
        TArray<FVector2D> Frame;
        Frame.Add(FVector2D(FrameLeft, FrameTop));
        Frame.Add(FVector2D(FrameRight, FrameTop));
        Frame.Add(FVector2D(FrameRight, FrameBottom));
        Frame.Add(FVector2D(FrameLeft, FrameBottom));
        Frame.Add(FVector2D(FrameLeft, FrameTop));
        
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            Frame, DrawEffects, Dim * 0.5f, true, Thickness);

        for (int32 i = 0; i < 9; ++i)
        {
            const float CellCenterX = KeyStartX + KeyCellWidth * (i + 0.5f);
            
            // Draw symbol
            const FString& Symbol = Program->KeySymbols[i];
            const FVector2D SymbolSize = FontMeasure->Measure(Symbol, KeyFont);
            const FVector2D SymbolPos(CellCenterX - SymbolSize.X * 0.5f, KeySymbolRowY);
            FSlateDrawElement::MakeText(OutDrawElements, LayerId, 
                AllottedGeometry.ToPaintGeometry(FVector2f(SymbolSize), FSlateLayoutTransform(FVector2f(SymbolPos))),
                Symbol, KeyFont, DrawEffects, Primary);
            
            // Draw digit below
            const FString DigitStr = FString::Printf(TEXT("%d"), i + 1);
            const FVector2D DigitSize = FontMeasure->Measure(DigitStr, DigitFont);
            const FVector2D DigitPos(CellCenterX - DigitSize.X * 0.5f, KeyDigitRowY);
            FSlateDrawElement::MakeText(OutDrawElements, LayerId, 
                AllottedGeometry.ToPaintGeometry(FVector2f(DigitSize), FSlateLayoutTransform(FVector2f(DigitPos))),
                DigitStr, DigitFont, DrawEffects, Dim);
        }
    }
    
    // === Draw Current Symbol (large, centered, with brackets) ===
    {
        const FString& CurrentSymbol = Program->KeySymbols[Program->CurrentSymbolIndex];
        const FVector2D SymbolSize = FontMeasure->Measure(CurrentSymbol, CurrentSymbolFont);
        const float CenterX = Size.X * 0.5f;
        const FVector2D SymbolPos(CenterX - SymbolSize.X * 0.5f, CurrentSymbolY);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, 
            AllottedGeometry.ToPaintGeometry(FVector2f(SymbolSize), FSlateLayoutTransform(FVector2f(SymbolPos))),
            CurrentSymbol, CurrentSymbolFont, DrawEffects, Primary);
        
        // Draw corner brackets
        const float BracketSize = 20.0f;
        const float AreaSize = 140.0f;
        const float AreaLeft = CenterX - AreaSize * 0.5f;
        const float AreaTop = CurrentSymbolY - 10.0f;
        const float AreaRight = CenterX + AreaSize * 0.5f;
        const float AreaBottom = CurrentSymbolY + SymbolSize.Y + 10.0f;
        
        // Top Left
        TArray<FVector2D> TL;
        TL.Add(FVector2D(AreaLeft, AreaTop + BracketSize));
        TL.Add(FVector2D(AreaLeft, AreaTop));
        TL.Add(FVector2D(AreaLeft + BracketSize, AreaTop));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), TL, DrawEffects, Primary, false, Thickness * 2.0f);
        
        // Top Right
        TArray<FVector2D> TR;
        TR.Add(FVector2D(AreaRight - BracketSize, AreaTop));
        TR.Add(FVector2D(AreaRight, AreaTop));
        TR.Add(FVector2D(AreaRight, AreaTop + BracketSize));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), TR, DrawEffects, Primary, false, Thickness * 2.0f);
        
        // Bottom Left
        TArray<FVector2D> BL;
        BL.Add(FVector2D(AreaLeft, AreaBottom - BracketSize));
        BL.Add(FVector2D(AreaLeft, AreaBottom));
        BL.Add(FVector2D(AreaLeft + BracketSize, AreaBottom));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), BL, DrawEffects, Primary, false, Thickness * 2.0f);
        
        // Bottom Right
        TArray<FVector2D> BR;
        BR.Add(FVector2D(AreaRight - BracketSize, AreaBottom));
        BR.Add(FVector2D(AreaRight, AreaBottom));
        BR.Add(FVector2D(AreaRight, AreaBottom - BracketSize));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), BR, DrawEffects, Primary, false, Thickness * 2.0f);
    }
    
    // === Draw Keypad (3x3 grid with stylized appearance) ===
    {
        const float KeyWidth = KeypadWidth / 3.0f;
        const float KeyHeight = KeypadHeight / 3.0f;
        const float InnerPadding = 4.0f;  // Small gap between keys
        
        // Draw outer border around entire keypad
        TArray<FVector2D> OuterBorder;
        OuterBorder.Add(FVector2D(KeypadLeft, KeypadTop));
        OuterBorder.Add(FVector2D(KeypadLeft + KeypadWidth, KeypadTop));
        OuterBorder.Add(FVector2D(KeypadLeft + KeypadWidth, KeypadTop + KeypadHeight));
        OuterBorder.Add(FVector2D(KeypadLeft, KeypadTop + KeypadHeight));
        OuterBorder.Add(FVector2D(KeypadLeft, KeypadTop));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            OuterBorder, DrawEffects, Primary, true, Thickness * 1.5f);
        
        // Keypad layout: rows of [1,2,3], [4,5,6], [7,8,9]
        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
            {
                const int32 Digit = Row * 3 + Col + 1;  // 1-9
                const float KeyLeft = KeypadLeft + Col * KeyWidth + InnerPadding;
                const float KeyTop = KeypadTop + Row * KeyHeight + InnerPadding;
                const float KeyInnerWidth = KeyWidth - InnerPadding * 2.0f;
                const float KeyInnerHeight = KeyHeight - InnerPadding * 2.0f;
                const float KeyCenterX = KeyLeft + KeyInnerWidth * 0.5f;
                const float KeyCenterY = KeyTop + KeyInnerHeight * 0.5f;
                const bool bHovered = (Program->HoveredDigit == Digit);
                
                const FVector2D BoxTopLeft(KeyLeft, KeyTop);
                const FVector2D BoxBottomRight(KeyLeft + KeyInnerWidth, KeyTop + KeyInnerHeight);
                
                // Fill if hovered
                if (bHovered)
                {
                    const FPaintGeometry FillGeom = AllottedGeometry.ToPaintGeometry(
                        FVector2f(KeyInnerWidth, KeyInnerHeight),
                        FSlateLayoutTransform(FVector2f(BoxTopLeft))
                    );
                    FSlateDrawElement::MakeBox(OutDrawElements, LayerId, FillGeom,
                        FCoreStyle::Get().GetBrush("WhiteBrush"), DrawEffects, Primary);
                }
                
                // Draw inner box outline
                TArray<FVector2D> BoxPoints;
                BoxPoints.Add(BoxTopLeft);
                BoxPoints.Add(FVector2D(BoxBottomRight.X, BoxTopLeft.Y));
                BoxPoints.Add(BoxBottomRight);
                BoxPoints.Add(FVector2D(BoxTopLeft.X, BoxBottomRight.Y));
                BoxPoints.Add(BoxTopLeft);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                    BoxPoints, DrawEffects, bHovered ? FLinearColor::Black : Primary, true, Thickness);
                
                // Draw corner accents (small L-shapes in corners)
                if (!bHovered)
                {
                    const float AccentSize = 8.0f;
                    // Top-left corner
                    TArray<FVector2D> TL;
                    TL.Add(FVector2D(BoxTopLeft.X, BoxTopLeft.Y + AccentSize));
                    TL.Add(BoxTopLeft);
                    TL.Add(FVector2D(BoxTopLeft.X + AccentSize, BoxTopLeft.Y));
                    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                        TL, DrawEffects, Dim, false, Thickness * 2.0f);
                    
                    // Top-right corner
                    TArray<FVector2D> TR;
                    TR.Add(FVector2D(BoxBottomRight.X - AccentSize, BoxTopLeft.Y));
                    TR.Add(FVector2D(BoxBottomRight.X, BoxTopLeft.Y));
                    TR.Add(FVector2D(BoxBottomRight.X, BoxTopLeft.Y + AccentSize));
                    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                        TR, DrawEffects, Dim, false, Thickness * 2.0f);
                    
                    // Bottom-left corner
                    TArray<FVector2D> BL;
                    BL.Add(FVector2D(BoxTopLeft.X, BoxBottomRight.Y - AccentSize));
                    BL.Add(FVector2D(BoxTopLeft.X, BoxBottomRight.Y));
                    BL.Add(FVector2D(BoxTopLeft.X + AccentSize, BoxBottomRight.Y));
                    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                        BL, DrawEffects, Dim, false, Thickness * 2.0f);
                    
                    // Bottom-right corner
                    TArray<FVector2D> BR;
                    BR.Add(FVector2D(BoxBottomRight.X - AccentSize, BoxBottomRight.Y));
                    BR.Add(BoxBottomRight);
                    BR.Add(FVector2D(BoxBottomRight.X, BoxBottomRight.Y - AccentSize));
                    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                        BR, DrawEffects, Dim, false, Thickness * 2.0f);
                }
                
                // Draw digit (larger font)
                const int32 KeyDigitFontSize = 44;
                const FSlateFontInfo KeyDigitFont = FCoreStyle::GetDefaultFontStyle("Bold", KeyDigitFontSize);
                const FString DigitStr = FString::Printf(TEXT("%d"), Digit);
                const FVector2D DigitSize = FontMeasure->Measure(DigitStr, KeyDigitFont);
                const FVector2D DigitPos(KeyCenterX - DigitSize.X * 0.5f, KeyCenterY - DigitSize.Y * 0.5f);
                FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2, 
                    AllottedGeometry.ToPaintGeometry(FVector2f(DigitSize), FSlateLayoutTransform(FVector2f(DigitPos))),
                    DigitStr, KeyDigitFont, DrawEffects, bHovered ? FLinearColor::Black : Primary);
            }
        }
    }
    
    return LayerId + 3;
}

// ============================================================================
// FSymbolDigitModalitiesProgram Implementation
// ============================================================================

FSymbolDigitModalitiesProgram::FSymbolDigitModalitiesProgram()
    : FTrialProgramBase(12)  // 12 trials
{
    SetTrialTitle(FText::FromString(TEXT("Symbol Digit Modalities Test")));
    // Initialize the 9 key symbols - ASCII chars
    KeySymbols = {
        TEXT("#"),
        TEXT("@"),
        TEXT("!"),
        TEXT("$"),
        TEXT("%"),
        TEXT("&"),
        TEXT("*"),
        TEXT("+"),
        TEXT("?")
    };
    GenerateNewTrial();
}

void FSymbolDigitModalitiesProgram::GenerateNewTrial()
{
    // Pick a random symbol for this trial
    CurrentSymbolIndex = FMath::RandRange(0, 8);
    InvalidateWidget();
}

int32 FSymbolDigitModalitiesProgram::GetDigitButtonAtPosition(const FVector2D& LocalPos, const FVector2D& WidgetSize) const
{
    // Keypad layout matching OnPaint - centered, large, rectangular cells
    const float KeypadWidth = WidgetSize.X * 0.8f;
    const float KeypadHeight = WidgetSize.Y * 0.45f;
    const float KeypadLeft = (WidgetSize.X - KeypadWidth) * 0.5f;
    const float KeypadTop = WidgetSize.Y - KeypadHeight - 10.0f;
    const float KeyWidth = KeypadWidth / 3.0f;
    const float KeyHeight = KeypadHeight / 3.0f;
    
    // Check if within keypad bounds
    if (LocalPos.X < KeypadLeft || LocalPos.X > KeypadLeft + KeypadWidth)
    {
        return -1;
    }
    if (LocalPos.Y < KeypadTop || LocalPos.Y > KeypadTop + KeypadHeight)
    {
        return -1;
    }
    
    const int32 Col = static_cast<int32>((LocalPos.X - KeypadLeft) / KeyWidth);
    const int32 Row = static_cast<int32>((LocalPos.Y - KeypadTop) / KeyHeight);
    
    if (Col >= 0 && Col < 3 && Row >= 0 && Row < 3)
    {
        return Row * 3 + Col + 1;  // 1-9
    }
    
    return -1;
}

void FSymbolDigitModalitiesProgram::EnterDigit(int32 Digit)
{
    if (AreAllTrialsComplete()) return;
    
    const int32 CorrectDigit = CurrentSymbolIndex + 1;
    
    if (Digit == CorrectDigit)
    {
        if (AdvanceTrial())
        {
            SetTaskComplete(true);
        }
    }
    else
    {
        FailTrial();
    }
    
    InvalidateWidget();
}

void FSymbolDigitModalitiesProgram::InvalidateWidget() const
{
    if (const TSharedPtr<SWidget> Widget = SheetWidget.Pin())
    {
        Widget->Invalidate(EInvalidateWidget::Paint);
    }
}

TSharedRef<SWidget> FSymbolDigitModalitiesProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();

    TSharedRef<SCodeSheetWidget> Sheet = SNew(SCodeSheetWidget)
        .PrimaryColor(Style.GetPrimaryColor())
        .DimColor(Style.GetDimColor())
        .LineThickness(Style.LineThickness)
        .Program(this);

    SheetWidget = Sheet;
    return Sheet;
}

void FSymbolDigitModalitiesProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || AreAllTrialsComplete())
    {
        HoveredDigit = -1;
        return;
    }
    
    const TSharedPtr<SWidget> Widget = SheetWidget.Pin();
    if (!Widget.IsValid()) return;
    
    const FGeometry& Geometry = Widget->GetCachedGeometry();
    const FVector2D LocalPos = Geometry.AbsoluteToLocal(Event.ScreenPixelPosition);
    const FVector2D WidgetSize = Geometry.GetLocalSize();
    
    const int32 NewHovered = GetDigitButtonAtPosition(LocalPos, WidgetSize);
    if (NewHovered != HoveredDigit)
    {
        HoveredDigit = NewHovered;
        InvalidateWidget();
    }
    
    UpdateCursorForState(false, HoveredDigit > 0);
}

void FSymbolDigitModalitiesProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || AreAllTrialsComplete()) return;
    
    ActivePointerKey = Event.TriggerKey;
    
    if (HoveredDigit > 0)
    {
        EnterDigit(HoveredDigit);
    }
}

void FSymbolDigitModalitiesProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    ActivePointerKey = EKeys::Invalid;
}

