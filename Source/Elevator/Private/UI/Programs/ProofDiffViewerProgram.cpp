#include "UI/Programs/ProofDiffViewerProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "HAL/PlatformTime.h"

REGISTER_SCREEN_PROGRAM(FProofDiffViewerProgram, "ProofDiffViewer")

namespace
{
    constexpr int32 ProofDiffCircleSegments = 16;
    
    // Draw a symbol at the given center position
    // All symbols are the same size - subtle variations only
    // Symbol types: 0=Vertical lines, 1=Horizontal lines, 2=Diagonal /, 3=Diagonal \, 4=Cross, 5=Grid
    void DrawSymbol(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
        const FVector2D& Center, float Size, int32 Symbol, const FLinearColor& Color, float Thickness)
    {
        const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
        const float HalfSize = Size * 0.3f;
        
        switch (Symbol)
        {
        case 0: // Two vertical lines
            {
                TArray<FVector2D> Line1, Line2;
                Line1.Add(Center + FVector2D(-HalfSize * 0.5f, -HalfSize));
                Line1.Add(Center + FVector2D(-HalfSize * 0.5f, HalfSize));
                Line2.Add(Center + FVector2D(HalfSize * 0.5f, -HalfSize));
                Line2.Add(Center + FVector2D(HalfSize * 0.5f, HalfSize));
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    Line1, DrawEffects, Color, true, Thickness);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    Line2, DrawEffects, Color, true, Thickness);
            }
            break;
        case 1: // Two horizontal lines
            {
                TArray<FVector2D> Line1, Line2;
                Line1.Add(Center + FVector2D(-HalfSize, -HalfSize * 0.5f));
                Line1.Add(Center + FVector2D(HalfSize, -HalfSize * 0.5f));
                Line2.Add(Center + FVector2D(-HalfSize, HalfSize * 0.5f));
                Line2.Add(Center + FVector2D(HalfSize, HalfSize * 0.5f));
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    Line1, DrawEffects, Color, true, Thickness);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    Line2, DrawEffects, Color, true, Thickness);
            }
            break;
        case 2: // Diagonal / (two parallel)
            {
                TArray<FVector2D> Line1, Line2;
                Line1.Add(Center + FVector2D(-HalfSize * 0.7f, HalfSize));
                Line1.Add(Center + FVector2D(HalfSize * 0.3f, -HalfSize));
                Line2.Add(Center + FVector2D(-HalfSize * 0.3f, HalfSize));
                Line2.Add(Center + FVector2D(HalfSize * 0.7f, -HalfSize));
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    Line1, DrawEffects, Color, true, Thickness);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    Line2, DrawEffects, Color, true, Thickness);
            }
            break;
        case 3: // Diagonal \ (two parallel)
            {
                TArray<FVector2D> Line1, Line2;
                Line1.Add(Center + FVector2D(-HalfSize * 0.7f, -HalfSize));
                Line1.Add(Center + FVector2D(HalfSize * 0.3f, HalfSize));
                Line2.Add(Center + FVector2D(-HalfSize * 0.3f, -HalfSize));
                Line2.Add(Center + FVector2D(HalfSize * 0.7f, HalfSize));
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    Line1, DrawEffects, Color, true, Thickness);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    Line2, DrawEffects, Color, true, Thickness);
            }
            break;
        case 4: // Cross (+)
            {
                TArray<FVector2D> LineH, LineV;
                LineH.Add(Center + FVector2D(-HalfSize, 0));
                LineH.Add(Center + FVector2D(HalfSize, 0));
                LineV.Add(Center + FVector2D(0, -HalfSize));
                LineV.Add(Center + FVector2D(0, HalfSize));
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    LineH, DrawEffects, Color, true, Thickness);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    LineV, DrawEffects, Color, true, Thickness);
            }
            break;
        case 5: // X
        default:
            {
                TArray<FVector2D> Line1, Line2;
                Line1.Add(Center + FVector2D(-HalfSize, -HalfSize));
                Line1.Add(Center + FVector2D(HalfSize, HalfSize));
                Line2.Add(Center + FVector2D(HalfSize, -HalfSize));
                Line2.Add(Center + FVector2D(-HalfSize, HalfSize));
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    Line1, DrawEffects, Color, true, Thickness);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                    Line2, DrawEffects, Color, true, Thickness);
            }
            break;
        }
    }
}

// ============================================================================
// SProofDiffWidget Implementation
// ============================================================================

void SProofDiffWidget::Construct(const FArguments& InArgs)
{
    PrimaryColor = InArgs._PrimaryColor;
    DimColor = InArgs._DimColor;
    LineThickness = InArgs._LineThickness;
    Program = InArgs._Program;
}

FVector2D SProofDiffWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    // Widget fills available space from parent - no specific size requirement
    return FVector2D::ZeroVector;
}

int32 SProofDiffWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (!Program) return LayerId;
    
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FLinearColor Primary = PrimaryColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const FLinearColor Dim = DimColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const float Thickness = LineThickness.Get();
    const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
    
    // Grid fills the entire widget - no centering offset
    const float CellWidth = Size.X / FProofDiffViewerProgram::GridCols;
    const float CellHeight = Size.Y / FProofDiffViewerProgram::GridRows;
    
    // Check if we're in a blank state
    const bool bIsBlank = (Program->ViewState == FProofDiffViewerProgram::EViewState::BlankBeforeB ||
                           Program->ViewState == FProofDiffViewerProgram::EViewState::BlankBeforeA);
    const bool bShowingB = (Program->ViewState == FProofDiffViewerProgram::EViewState::ShowingB);
    
    // Draw grid lines (always visible, even during blank)
    for (int32 Col = 0; Col <= FProofDiffViewerProgram::GridCols; ++Col)
    {
        TArray<FVector2D> Line;
        Line.Add(FVector2D(Col * CellWidth, 0));
        Line.Add(FVector2D(Col * CellWidth, Size.Y));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            Line, DrawEffects, Dim * 0.5f, true, Thickness * 0.5f);
    }
    for (int32 Row = 0; Row <= FProofDiffViewerProgram::GridRows; ++Row)
    {
        TArray<FVector2D> Line;
        Line.Add(FVector2D(0, Row * CellHeight));
        Line.Add(FVector2D(Size.X, Row * CellHeight));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            Line, DrawEffects, Dim * 0.5f, true, Thickness * 0.5f);
    }
    
    // Draw hover/selection indicators (always visible, even during blank)
    for (int32 Row = 0; Row < FProofDiffViewerProgram::GridRows; ++Row)
    {
        for (int32 Col = 0; Col < FProofDiffViewerProgram::GridCols; ++Col)
        {
            const int32 CellIdx = Row * FProofDiffViewerProgram::GridCols + Col;
            const bool bIsSelected = (Program->SelectedCell == CellIdx);
            const bool bIsHovered = (Program->HoveredCell == CellIdx);
            
            if (bIsSelected || bIsHovered)
            {
                // Draw bracket corners for hover/selection indicator
                const float InsetX = CellWidth * 0.08f;
                const float InsetY = CellHeight * 0.08f;
                const float CornerLen = FMath::Min(CellWidth, CellHeight) * 0.25f;
                const FVector2D TL(Col * CellWidth + InsetX, Row * CellHeight + InsetY);
                const FVector2D TR((Col + 1) * CellWidth - InsetX, Row * CellHeight + InsetY);
                const FVector2D BL(Col * CellWidth + InsetX, (Row + 1) * CellHeight - InsetY);
                const FVector2D BR((Col + 1) * CellWidth - InsetX, (Row + 1) * CellHeight - InsetY);
                const FLinearColor IndicatorColor = bIsSelected ? Primary : Primary * 0.7f;
                const float IndicatorThickness = bIsSelected ? Thickness * 1.5f : Thickness;
                
                // Top-left corner
                TArray<FVector2D> Corner1; Corner1.Add(TL + FVector2D(0, CornerLen)); Corner1.Add(TL); Corner1.Add(TL + FVector2D(CornerLen, 0));
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Corner1, DrawEffects, IndicatorColor, false, IndicatorThickness);
                // Top-right corner
                TArray<FVector2D> Corner2; Corner2.Add(TR + FVector2D(-CornerLen, 0)); Corner2.Add(TR); Corner2.Add(TR + FVector2D(0, CornerLen));
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Corner2, DrawEffects, IndicatorColor, false, IndicatorThickness);
                // Bottom-left corner
                TArray<FVector2D> Corner3; Corner3.Add(BL + FVector2D(0, -CornerLen)); Corner3.Add(BL); Corner3.Add(BL + FVector2D(CornerLen, 0));
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Corner3, DrawEffects, IndicatorColor, false, IndicatorThickness);
                // Bottom-right corner
                TArray<FVector2D> Corner4; Corner4.Add(BR + FVector2D(-CornerLen, 0)); Corner4.Add(BR); Corner4.Add(BR + FVector2D(0, -CornerLen));
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Corner4, DrawEffects, IndicatorColor, false, IndicatorThickness);
            }
        }
    }
    
    // During blank state, don't draw any symbols
    if (bIsBlank)
    {
        return LayerId + 3;
    }
    
    // Draw cell contents (symbols)
    for (int32 Row = 0; Row < FProofDiffViewerProgram::GridRows; ++Row)
    {
        for (int32 Col = 0; Col < FProofDiffViewerProgram::GridCols; ++Col)
        {
            const int32 CellIdx = Row * FProofDiffViewerProgram::GridCols + Col;
            const FVector2D CellCenter(
                Col * CellWidth + CellWidth * 0.5f,
                Row * CellHeight + CellHeight * 0.5f
            );
            
            // Determine which symbol to show
            int32 Symbol = 0;
            if (CellIdx < Program->CellContents.Num())
            {
                const auto& Content = Program->CellContents[CellIdx];
                if (CellIdx == Program->DifferenceIndex && bShowingB && Content.AltSymbol >= 0)
                {
                    Symbol = Content.AltSymbol;
                }
                else
                {
                    Symbol = Content.BaseSymbol;
                }
            }
            
            // Determine color based on state
            FLinearColor CellColor = Dim;
            const bool bIsSelected = (Program->SelectedCell == CellIdx);
            const bool bIsHovered = (Program->HoveredCell == CellIdx);
            if (bIsSelected || bIsHovered)
            {
                CellColor = bIsSelected ? Primary : Primary * 0.9f;
            }
            
            // Use smaller of cell dimensions for symbol size
            const float SymbolSize = FMath::Min(CellWidth, CellHeight);
            DrawSymbol(OutDrawElements, LayerId + 2, AllottedGeometry, CellCenter, SymbolSize, Symbol, CellColor, Thickness);
        }
    }
    
    return LayerId + 3;
}

// ============================================================================
// FProofDiffViewerProgram Implementation
// ============================================================================

FProofDiffViewerProgram::FProofDiffViewerProgram()
    : FTrialProgramBase(6)
{
    SetTrialTitle(FText::FromString(TEXT("Proof Diff Viewer")));
    LastStateChangeTime = FPlatformTime::Seconds();
    GenerateDifference();
}

void FProofDiffViewerProgram::GenerateNewTrial()
{
    GenerateDifference();
}

void FProofDiffViewerProgram::UpdateFlicker()
{
    const double Now = FPlatformTime::Seconds();
    const double Elapsed = Now - LastStateChangeTime;
    
    bool bStateChanged = false;
    
    switch (ViewState)
    {
    case EViewState::ShowingA:
        if (Elapsed >= ViewDurationA)
        {
            ViewState = EViewState::BlankBeforeB;
            LastStateChangeTime = Now;
            bStateChanged = true;
        }
        break;
    case EViewState::BlankBeforeB:
        if (Elapsed >= GetBlankDuration(true))
        {
            ViewState = EViewState::ShowingB;
            LastStateChangeTime = Now;
            bStateChanged = true;
        }
        break;
    case EViewState::ShowingB:
        if (Elapsed >= ViewDurationB)
        {
            ViewState = EViewState::BlankBeforeA;
            LastStateChangeTime = Now;
            bStateChanged = true;
        }
        break;
    case EViewState::BlankBeforeA:
        if (Elapsed >= GetBlankDuration(false))
        {
            ViewState = EViewState::ShowingA;
            LastStateChangeTime = Now;
            bStateChanged = true;
        }
        break;
    }
    
    if (bStateChanged)
    {
        InvalidateWidget();
    }
}

void FProofDiffViewerProgram::GenerateDifference()
{
    // Generate random content for all cells
    CellContents.SetNum(GridSize);
    for (int32 i = 0; i < GridSize; ++i)
    {
        CellContents[i].BaseSymbol = FMath::RandRange(0, 5);
        CellContents[i].AltSymbol = -1; // No difference by default
    }
    
    // Pick a difference cell (different from previous)
    do
    {
        DifferenceIndex = FMath::RandRange(0, GridSize - 1);
    } while (DifferenceIndex == PreviousDifferenceIndex);
    
    PreviousDifferenceIndex = DifferenceIndex;
    
    // Set an alternative symbol for the difference cell (different from base)
    int32 AltSymbol;
    do
    {
        AltSymbol = FMath::RandRange(0, 5);
    } while (AltSymbol == CellContents[DifferenceIndex].BaseSymbol);
    
    CellContents[DifferenceIndex].AltSymbol = AltSymbol;
    
    // Reset state
    SelectedCell = -1;
    ViewState = EViewState::ShowingA;
    LastStateChangeTime = FPlatformTime::Seconds();

    const double DifficultyFactor = FMath::Clamp(static_cast<double>(GetCurrentTrial()) * 0.04, 0.0, 0.35);
    ViewDurationA = FMath::Max(0.45, 0.7 - DifficultyFactor * 0.35); // A stays longer but accelerates slightly over time
    ViewDurationB = FMath::Max(0.28, 0.45 - DifficultyFactor * 0.25); // B stays shorter overall
    BlankDurationBeforeB = FMath::Clamp(0.24 + DifficultyFactor * 0.4, 0.24, 0.6); // Longer delay before B (feels slower)
    BlankDurationBeforeA = FMath::Clamp(0.12 - DifficultyFactor * 0.25, 0.06, 0.2); // Shorter pause before A (feels faster)
    
    InvalidateWidget();
}

double FProofDiffViewerProgram::GetBlankDuration(bool bBeforeB) const
{
    const double TrialRamp = FMath::Clamp(static_cast<double>(GetCurrentTrial()) * 0.03, 0.0, 0.3);
    const double BaseDuration = bBeforeB ? BlankDurationBeforeB : BlankDurationBeforeA;
    const double Adjustment = bBeforeB ? TrialRamp * 0.35 : TrialRamp * 0.15;
    return FMath::Max(0.05, BaseDuration + Adjustment);
}

int32 FProofDiffViewerProgram::GetCellAtPosition(const FVector2D& NormalizedPos) const
{
    // Strict bounds check - must be strictly inside the grid (not at edges)
    if (NormalizedPos.X <= 0.0f || NormalizedPos.X >= 1.0f ||
        NormalizedPos.Y <= 0.0f || NormalizedPos.Y >= 1.0f)
    {
        return -1;
    }
    
    const int32 Col = static_cast<int32>(NormalizedPos.X * GridCols);
    const int32 Row = static_cast<int32>(NormalizedPos.Y * GridRows);
    
    // Extra safety check
    if (Col < 0 || Col >= GridCols || Row < 0 || Row >= GridRows)
    {
        return -1;
    }
    
    return Row * GridCols + Col;
}

void FProofDiffViewerProgram::InvalidateWidget() const
{
    if (const TSharedPtr<SWidget> Widget = DiffWidget.Pin())
    {
        Widget->Invalidate(EInvalidateWidget::Paint);
    }
}

TSharedRef<SWidget> FProofDiffViewerProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();

    TSharedPtr<SProofDiffWidget> GridWidget;

    TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
        // Scan status indicator
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, Style.GetSmallPadding())
        [SNew(STextBlock)
            .Text_Lambda([this]() {
                if (GetTaskComplete()) return FText::FromString(TEXT("[ VERSIONS RECONCILED ]"));
                switch (ViewState)
                {
                case EViewState::ShowingA:
                case EViewState::BlankBeforeA:
                    return FText::FromString(TEXT(">> SCANNING VERSION A <<"));
                case EViewState::ShowingB:
                case EViewState::BlankBeforeB:
                    return FText::FromString(TEXT(">> SCANNING VERSION B <<"));
                }
                return FText::GetEmpty();
            })
            .Font(FCoreStyle::GetDefaultFontStyle("Mono", Style.TextSize))
            .ColorAndOpacity(Style.GetPrimaryColor())]

        // Grid widget - fills available space
        + SVerticalBox::Slot().FillHeight(1.0f).Padding(Style.GetSmallPadding())
        [SAssignNew(GridWidget, SProofDiffWidget)
            .PrimaryColor(Style.GetPrimaryColor())
            .DimColor(Style.GetDimColor())
            .LineThickness(Style.LineThickness)
            .Program(this)];

    DiffWidget = GridWidget;
    return Content;
}

void FProofDiffViewerProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || GetTaskComplete()) return;
    
    // Time-based flicker state machine
    UpdateFlicker();
    
    // Get normalized position within the grid widget
    const TSharedPtr<SWidget> Widget = DiffWidget.Pin();
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
        
        const int32 NewHovered = GetCellAtPosition(NormalizedPos);
        if (NewHovered != HoveredCell)
        {
            HoveredCell = NewHovered;
            InvalidateWidget();
        }
    }
    
    UpdateCursorForState(false, HoveredCell >= 0);
}

void FProofDiffViewerProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || GetTaskComplete()) return;
    
    if (HoveredCell >= 0)
    {
        SelectedCell = HoveredCell;
        InvalidateWidget();
        
        if (HoveredCell == DifferenceIndex)
        {
            // Correct! Found the difference
            AdvanceTrial();
        }
        else
        {
            // Wrong - go back a trial and generate new difference
            FailTrial();
        }
    }
}

void FProofDiffViewerProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    // Nothing special on release
}

