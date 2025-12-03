#include "UI/Programs/ReyOsterriethCopyProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "HAL/PlatformTime.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

REGISTER_SCREEN_PROGRAM(FReyOsterriethCopyProgram, "rey_osterrieth_copy")

// ============================================================
// SDirectoryIndexRebuildWidget
// ============================================================

void SDirectoryIndexRebuildWidget::Construct(const FArguments& InArgs)
{
    Program = InArgs._Program;
}

FVector2D SDirectoryIndexRebuildWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return FVector2D::ZeroVector;
}

int32 SDirectoryIndexRebuildWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
    const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (!Program) return LayerId;
    
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    if (Size.IsNearlyZero())
    {
        return LayerId;
    }
    const FLinearColor Primary = Program->GetProgramStyle().GetPrimaryColor();
    const FLinearColor Dim = Program->GetProgramStyle().GetDimColor().GetSpecifiedColor();
    const float Thickness = Program->GetProgramStyle().LineThickness;
    const double Now = FPlatformTime::Seconds();
    
    const float CellSize = FReyOsterriethCopyProgram::CellSize;
    const int32 GridCols = FReyOsterriethCopyProgram::GridCols;
    const int32 GridRows = FReyOsterriethCopyProgram::GridRows;

    // Complete state
    if (Program->AreAllTrialsComplete())
    {
        FSlateFontInfo CompleteFont = FCoreStyle::GetDefaultFontStyle("Bold", 18);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2f(200, 30), FSlateLayoutTransform(FVector2f(Size.X * 0.5f - 80.0f, Size.Y * 0.45f))),
            TEXT("Index rebuilt"), CompleteFont, ESlateDrawEffect::None, Primary);
        return LayerId + 1;
    }

    // Grid area
    const FReyOsterriethCopyProgram::FCanvasLayout Layout = Program->BuildCanvasLayout(Size);
    const FVector2D GridOrigin = Layout.GridOrigin;
    const float GridWidth = Layout.GridWidth;
    const float GridHeight = Layout.GridHeight;
    
    // Draw grid border
    FVector2D GridTL = GridOrigin;
    TArray<FVector2D> GridBorder;
    GridBorder.Add(GridTL);
    GridBorder.Add(FVector2D(GridTL.X + GridWidth, GridTL.Y));
    GridBorder.Add(FVector2D(GridTL.X + GridWidth, GridTL.Y + GridHeight));
    GridBorder.Add(FVector2D(GridTL.X, GridTL.Y + GridHeight));
    GridBorder.Add(GridTL);
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
        GridBorder, ESlateDrawEffect::None, Primary * 0.6f, true, Thickness);
    
    // Draw grid lines
    for (int32 i = 1; i < GridCols; ++i)
    {
        float X = GridOrigin.X + i * CellSize;
        TArray<FVector2D> Line;
        Line.Add(FVector2D(X, GridOrigin.Y));
        Line.Add(FVector2D(X, GridOrigin.Y + GridHeight));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            Line, ESlateDrawEffect::None, Dim * 0.2f, true, 1.0f);
    }
    for (int32 i = 1; i < GridRows; ++i)
    {
        float Y = GridOrigin.Y + i * CellSize;
        TArray<FVector2D> Line;
        Line.Add(FVector2D(GridOrigin.X, Y));
        Line.Add(FVector2D(GridOrigin.X + GridWidth, Y));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            Line, ESlateDrawEffect::None, Dim * 0.2f, true, 1.0f);
    }
    
    // Helper lambda to draw primitives
    auto DrawPrimitive = [&](FVector2D Center, int32 Type, FLinearColor Color, float Scale = 1.0f, float LineWidth = 0.0f)
    {
        const float EffectiveThickness = (LineWidth > 0.0f) ? LineWidth : Thickness;
        float S = CellSize * 0.38f * Scale;
        
        switch (Type)
        {
            case 0: // Square
            {
                FVector2D TL(Center.X - S, Center.Y - S);
                TArray<FVector2D> Rect;
                Rect.Add(TL);
                Rect.Add(FVector2D(Center.X + S, Center.Y - S));
                Rect.Add(FVector2D(Center.X + S, Center.Y + S));
                Rect.Add(FVector2D(Center.X - S, Center.Y + S));
                Rect.Add(TL);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                    Rect, ESlateDrawEffect::None, Color, true, EffectiveThickness);
                break;
            }
            case 1: // Line (diagonal)
            {
                TArray<FVector2D> Line;
                Line.Add(FVector2D(Center.X - S, Center.Y - S));
                Line.Add(FVector2D(Center.X + S, Center.Y + S));
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                    Line, ESlateDrawEffect::None, Color, true, EffectiveThickness);
                break;
            }
            case 2: // Triangle
            {
                FVector2D Top(Center.X, Center.Y - S);
                TArray<FVector2D> Tri;
                Tri.Add(Top);
                Tri.Add(FVector2D(Center.X + S, Center.Y + S * 0.7f));
                Tri.Add(FVector2D(Center.X - S, Center.Y + S * 0.7f));
                Tri.Add(Top);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                    Tri, ESlateDrawEffect::None, Color, true, EffectiveThickness);
                break;
            }
            case 3: // Circle
            {
                TArray<FVector2D> Circle;
                const int32 Segments = 12;
                for (int32 Seg = 0; Seg <= Segments; ++Seg)
                {
                    float Angle = (float)Seg / Segments * 2.0f * PI;
                    Circle.Add(FVector2D(Center.X + FMath::Cos(Angle) * S, Center.Y + FMath::Sin(Angle) * S));
                }
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                    Circle, ESlateDrawEffect::None, Color, true, EffectiveThickness);
                break;
            }
        }
    };
    
    // Error flash overlay
    const float ErrorAlpha = Program->GetErrorFlashAlpha(Now);
    if (ErrorAlpha > 0.0f)
    {
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2f(GridWidth, GridHeight), FSlateLayoutTransform(FVector2f(GridOrigin.X, GridOrigin.Y))),
            FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor(1.0f, 0.2f, 0.2f, ErrorAlpha * 0.3f));
        Program->InvalidateWidget();
    }
    
    // Draw grid contents
    float PreviewAlpha = Program->bShowingTarget ? 1.0f : Program->GetPreviewFadeAlpha(Now);
    if (PreviewAlpha > 0.01f)
    {
        // Show target pattern (with fade to black)
        FLinearColor FadedPrimary = Primary * PreviewAlpha;  // Fade brightness
        for (const auto& Elem : Program->TargetPattern)
        {
            FVector2D CellCenter(
                GridOrigin.X + Elem.GridX * CellSize + CellSize * 0.5f,
                GridOrigin.Y + Elem.GridY * CellSize + CellSize * 0.5f
            );
            DrawPrimitive(CellCenter, Elem.Type, FadedPrimary);
        }
    }
    else if (!Program->bShowingTarget && Program->PreviewFadeStartTime > 0.0)
    {
        // Fade complete, stop ticking
        Program->PreviewFadeStartTime = 0.0;
    }

    if (!Program->bShowingTarget)
    {
        // Show user placements
        for (int32 i = 0; i < Program->UserGrid.Num(); ++i)
        {
            if (Program->UserGrid[i] >= 0)
            {
                int32 GX = i % GridCols;
                int32 GY = i / GridCols;
                FVector2D CellCenter(
                    GridOrigin.X + GX * CellSize + CellSize * 0.5f,
                    GridOrigin.Y + GY * CellSize + CellSize * 0.5f
                );
                DrawPrimitive(CellCenter, Program->UserGrid[i], Primary);
            }
        }
        
        if (Program->IsPreviewFadeActive(Now))
        {
            Program->InvalidateWidget();
        }
        
        // Highlight hovered cell when holding a shape (show preview)
        if (Program->HoveredGridCell >= 0 && Program->SelectedPrimitive >= 0)
        {
            int32 HX = Program->HoveredGridCell % GridCols;
            int32 HY = Program->HoveredGridCell / GridCols;
            FVector2D CellTL(GridOrigin.X + HX * CellSize, GridOrigin.Y + HY * CellSize);
            
            FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
                AllottedGeometry.ToPaintGeometry(FVector2f(CellSize, CellSize), FSlateLayoutTransform(FVector2f(CellTL.X, CellTL.Y))),
                FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, Primary * 0.1f);
            
            // Draw preview of shape to be placed
            FVector2D PreviewCenter(CellTL.X + CellSize * 0.5f, CellTL.Y + CellSize * 0.5f);
            DrawPrimitive(PreviewCenter, Program->SelectedPrimitive, Dim * 0.4f);
        }
    }
    
    // Palette area (always visible) - same cell size as grid
    {
        const FVector2D PaletteOrigin = Layout.PaletteOrigin;
        const float PaletteItemSize = CellSize;  // Same as grid cells
        const float PaletteSpacing = Layout.PaletteSpacing;

        for (int32 i = 0; i < 4; ++i)
        {
            const float ItemX = PaletteOrigin.X + i * PaletteSpacing + PaletteItemSize * 0.5f;
            const float ItemY = PaletteOrigin.Y + PaletteItemSize * 0.5f;

            const bool bSelected = (Program->SelectedPrimitive == i);
            const bool bHovered = (Program->HoveredPrimitive == i);

            const FLinearColor IconColor = bSelected ? Primary : (bHovered ? Primary * 0.85f : Dim);
            const float IconThickness = bSelected ? Thickness * 0.9f : Thickness * 0.6f;
            DrawPrimitive(FVector2D(ItemX, ItemY), i, IconColor, 1.0f, IconThickness);
        }
    }
    
    // Apply button (always visible)
    {
        const FVector2D ApplyCenter = Layout.ApplyButtonCenter;
        const float BtnWidth = FReyOsterriethCopyProgram::ApplyButtonWidth;
        const float BtnHeight = FReyOsterriethCopyProgram::ApplyButtonHeight;

        const bool bApplyClickable = Program->IsApplyButtonClickable();
        const bool bApplyHovered = bApplyClickable && Program->bHoveringApply;

        FLinearColor BorderColor;
        FLinearColor TextColor;
        float BorderThickness;

        if (!bApplyClickable)
        {
            BorderColor = Dim * 0.45f;
            TextColor = Dim * 0.6f;
            BorderThickness = Thickness;
        }
        else if (bApplyHovered)
        {
            BorderColor = Primary;
            TextColor = Primary;
            BorderThickness = Thickness * 1.7f;
        }
        else
        {
            BorderColor = Primary * 0.9f;
            TextColor = Primary;
            BorderThickness = Thickness * 1.35f;
        }

        FVector2D BtnTL(ApplyCenter.X - BtnWidth * 0.5f, ApplyCenter.Y - BtnHeight * 0.5f);
        TArray<FVector2D> BtnBorder;
        BtnBorder.Add(BtnTL);
        BtnBorder.Add(FVector2D(ApplyCenter.X + BtnWidth * 0.5f, ApplyCenter.Y - BtnHeight * 0.5f));
        BtnBorder.Add(FVector2D(ApplyCenter.X + BtnWidth * 0.5f, ApplyCenter.Y + BtnHeight * 0.5f));
        BtnBorder.Add(FVector2D(ApplyCenter.X - BtnWidth * 0.5f, ApplyCenter.Y + BtnHeight * 0.5f));
        BtnBorder.Add(BtnTL);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            BtnBorder, ESlateDrawEffect::None, BorderColor, true, BorderThickness);

        const FSlateFontInfo BtnFont = FCoreStyle::GetDefaultFontStyle("Bold", 18);
        static const FString ApplyText = TEXT("APPLY");
        FVector2D TextSize(0.0f, 0.0f);
        if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
        {
            const TSharedPtr<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
            if (FontMeasure.IsValid())
            {
                TextSize = FontMeasure->Measure(ApplyText, BtnFont);
            }
        }
        if (TextSize.IsNearlyZero())
        {
            TextSize = FVector2D(70.0f, 24.0f);
        }
        FVector2D TextPos(ApplyCenter.X - TextSize.X * 0.5f, ApplyCenter.Y - TextSize.Y * 0.5f);
        FSlateDrawElement::MakeText(OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2f(TextSize.X, TextSize.Y), FSlateLayoutTransform(FVector2f(TextPos.X, TextPos.Y))),
            ApplyText, BtnFont, ESlateDrawEffect::None, TextColor);
    }
    
    return LayerId + 5;
}

// ============================================================
// FReyOsterriethCopyProgram
// ============================================================

FReyOsterriethCopyProgram::FReyOsterriethCopyProgram()
    : FTrialProgramBase(8)
{
    SetTrialTitle(FText::FromString(TEXT("Rey-Osterrieth Complex Figure Test")));
    UserGrid.SetNum(GridCols * GridRows);
    for (int32& V : UserGrid) V = -1;
    GenerateNewTrial();
}

FReyOsterriethCopyProgram::FCanvasLayout FReyOsterriethCopyProgram::BuildCanvasLayout(const FVector2D& CanvasSize) const
{
    FCanvasLayout Layout;

    const float GridWidth = GridCols * CellSize;
    const float GridHeight = GridRows * CellSize;
    const FScreenProgramStyle& Style = GetProgramStyle();
    const float ButtonMargin = FMath::Max(12.0f, Style.GetSmallPadding());
    const float ButtonBandHeight = ApplyButtonHeight + ButtonMargin * 2.0f;

    Layout.GridWidth = GridWidth;
    Layout.GridHeight = GridHeight;

    const float TopMargin = Style.GetSmallPadding();
    const float BottomMargin = Style.GetSmallPadding();
    const float PaletteHeight = CellSize;
    const float TotalContentHeight = GridHeight + ButtonBandHeight + PaletteHeight;
    const float ExtraSpace = FMath::Max(0.0f, CanvasSize.Y - TopMargin - BottomMargin - TotalContentHeight);

    float GridTop = TopMargin + ExtraSpace * 0.2f;
    float GridBottom = GridTop + GridHeight;

    Layout.PaletteSpacing = CellSize + 10.0f;
    const float PaletteRowWidth = 4.0f * Layout.PaletteSpacing;
    float PaletteTop = GridBottom + ButtonBandHeight;
    float PaletteBottom = PaletteTop + PaletteHeight;

    if (PaletteBottom > CanvasSize.Y - BottomMargin)
    {
        const float Shift = PaletteBottom - (CanvasSize.Y - BottomMargin);
        GridTop = FMath::Max(TopMargin, GridTop - Shift);
        GridBottom = GridTop + GridHeight;
        PaletteTop = GridBottom + ButtonBandHeight;
        PaletteBottom = PaletteTop + PaletteHeight;
    }

    Layout.GridOrigin = FVector2D(FMath::Max(0.0f, (CanvasSize.X - GridWidth) * 0.5f), GridTop);
    Layout.PaletteOrigin = FVector2D(FMath::Max(0.0f, (CanvasSize.X - PaletteRowWidth) * 0.5f), PaletteTop);

    const float GridToPaletteGap = PaletteTop - GridBottom;
    const float MinY = GridBottom + ApplyButtonHeight * 0.5f + ButtonMargin * 0.5f;
    const float MaxY = PaletteTop - ApplyButtonHeight * 0.5f - ButtonMargin * 0.5f;
    float CenterY = GridToPaletteGap > 0.0f ? GridBottom + GridToPaletteGap * 0.5f : MinY;
    if (MinY <= MaxY)
    {
        CenterY = FMath::Clamp(CenterY, MinY, MaxY);
    }
    else
    {
        CenterY = MinY;
    }
    Layout.ApplyButtonCenter = FVector2D(CanvasSize.X * 0.5f, CenterY);

    return Layout;
}

int32 FReyOsterriethCopyProgram::FindGridCellAtPosition(const FVector2D& LocalPosition, const FCanvasLayout& Layout) const
{
    if (LocalPosition.X < Layout.GridOrigin.X || LocalPosition.Y < Layout.GridOrigin.Y)
    {
        return -1;
    }

    const float GridRight = Layout.GridOrigin.X + Layout.GridWidth;
    const float GridBottom = Layout.GridOrigin.Y + Layout.GridHeight;
    if (LocalPosition.X > GridRight || LocalPosition.Y > GridBottom)
    {
        return -1;
    }

    int32 CellX = FMath::FloorToInt((LocalPosition.X - Layout.GridOrigin.X) / CellSize);
    int32 CellY = FMath::FloorToInt((LocalPosition.Y - Layout.GridOrigin.Y) / CellSize);

    CellX = FMath::Clamp(CellX, 0, GridCols - 1);
    CellY = FMath::Clamp(CellY, 0, GridRows - 1);

    return CellY * GridCols + CellX;
}

int32 FReyOsterriethCopyProgram::FindPaletteItemAtPosition(const FVector2D& LocalPosition, const FCanvasLayout& Layout) const
{
    const float ItemSize = CellSize;
    if (LocalPosition.Y < Layout.PaletteOrigin.Y || LocalPosition.Y > Layout.PaletteOrigin.Y + ItemSize)
    {
        return -1;
    }

    for (int32 Index = 0; Index < 4; ++Index)
    {
        const float ItemCenterX = Layout.PaletteOrigin.X + Index * Layout.PaletteSpacing + ItemSize * 0.5f;
        if (FMath::Abs(LocalPosition.X - ItemCenterX) <= ItemSize * 0.5f)
        {
            return Index;
        }
    }

    return -1;
}

bool FReyOsterriethCopyProgram::IsOverApplyButton(const FVector2D& LocalPosition, const FCanvasLayout& Layout) const
{
    return FMath::Abs(LocalPosition.X - Layout.ApplyButtonCenter.X) <= ApplyButtonWidth * 0.5f &&
           FMath::Abs(LocalPosition.Y - Layout.ApplyButtonCenter.Y) <= ApplyButtonHeight * 0.5f;
}

bool FReyOsterriethCopyProgram::IsApplyButtonClickable() const
{
    return !bShowingTarget && !AreAllTrialsComplete();
}

void FReyOsterriethCopyProgram::GenerateNewTrial()
{
    TargetPattern.Empty();
    for (int32& V : UserGrid) V = -1;
    
    // Generate 3-5 elements for 3x3 grid
    int32 NumElements = FMath::RandRange(3, 5);
    
    TSet<int32> UsedCells;
    
    for (int32 i = 0; i < NumElements; ++i)
    {
        FPatternElement Elem;
        
        int32 Attempts = 0;
        do {
            Elem.GridX = FMath::RandRange(0, GridCols - 1);
            Elem.GridY = FMath::RandRange(0, GridRows - 1);
            Attempts++;
        } while (UsedCells.Contains(Elem.GridY * GridCols + Elem.GridX) && Attempts < 50);
        
        UsedCells.Add(Elem.GridY * GridCols + Elem.GridX);
        Elem.Type = FMath::RandRange(0, 3);
        
        TargetPattern.Add(Elem);
    }
    
    bShowingTarget = true;
    PreviewFadeStartTime = 0.0;
    ErrorFlashStartTime = 0.0;
    SelectedPrimitive = -1;
    HoveredPrimitive = -1;
    HoveredGridCell = -1;
    bHoveringApply = false;
    
    InvalidateWidget();
}

void FReyOsterriethCopyProgram::CheckAndApply()
{
    if (bShowingTarget || AreAllTrialsComplete()) return;
    
    // Check if user placements match target
    bool bAllMatch = true;
    for (const auto& Target : TargetPattern)
    {
        int32 TargetCell = Target.GridY * GridCols + Target.GridX;
        if (UserGrid[TargetCell] != Target.Type)
        {
            bAllMatch = false;
            break;
        }
    }
    
    // Check no extra placements
    int32 UserCount = 0;
    for (int32 V : UserGrid) if (V >= 0) UserCount++;
    
    if (bAllMatch && UserCount == TargetPattern.Num())
    {
        AdvanceTrial();
    }
    else
    {
        ErrorFlashStartTime = FPlatformTime::Seconds();  // Trigger error flash
        FailTrial();
    }
    
    InvalidateWidget();
}

void FReyOsterriethCopyProgram::InvalidateWidget() const
{
    if (CanvasWidget.IsValid())
    {
        CanvasWidget.Pin()->Invalidate(EInvalidateWidget::Paint);
    }
}

float FReyOsterriethCopyProgram::GetPreviewFadeAlpha(double Now) const
{
    if (bShowingTarget)
    {
        return 1.0f;
    }

    if (PreviewFadeStartTime <= 0.0)
    {
        return 0.0f;
    }

    const double Elapsed = Now - PreviewFadeStartTime;
    if (Elapsed <= 0.0)
    {
        return 1.0f;
    }

    const double Ratio = 1.0 - (Elapsed / PreviewFadeDurationSeconds);
    return static_cast<float>(FMath::Clamp(Ratio, 0.0, 1.0));
}

bool FReyOsterriethCopyProgram::IsPreviewFadeActive(double Now) const
{
    return PreviewFadeStartTime > 0.0 && (Now - PreviewFadeStartTime) < PreviewFadeDurationSeconds;
}

float FReyOsterriethCopyProgram::GetErrorFlashAlpha(double Now) const
{
    if (ErrorFlashStartTime <= 0.0)
    {
        return 0.0f;
    }

    const double Elapsed = Now - ErrorFlashStartTime;
    if (Elapsed <= 0.0)
    {
        return 1.0f;
    }

    const double Ratio = 1.0 - (Elapsed / ErrorFlashDurationSeconds);
    return static_cast<float>(FMath::Clamp(Ratio, 0.0, 1.0));
}

TSharedRef<SWidget> FReyOsterriethCopyProgram::BuildTrialBody()
{
    TSharedRef<SDirectoryIndexRebuildWidget> Canvas = SNew(SDirectoryIndexRebuildWidget)
        .Program(this);

    CanvasWidget = Canvas;
    return Canvas;
}

void FReyOsterriethCopyProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || AreAllTrialsComplete()) return;

    const TSharedPtr<SDirectoryIndexRebuildWidget> CanvasPtr = CanvasWidget.Pin();
    if (!CanvasPtr.IsValid()) return;

    const FGeometry& Geometry = CanvasPtr->GetCachedGeometry();
    const FVector2D WidgetSize = Geometry.GetLocalSize();
    if (WidgetSize.IsNearlyZero()) return;

    const FVector2D LocalPos = Geometry.AbsoluteToLocal(Event.ScreenPixelPosition);
    const FCanvasLayout Layout = BuildCanvasLayout(WidgetSize);

    // Always track palette hover, but grid only when not showing target
    const int32 NewHoveredPrimitive = FindPaletteItemAtPosition(LocalPos, Layout);
    const int32 NewHoveredCell = bShowingTarget ? -1 : FindGridCellAtPosition(LocalPos, Layout);
    const bool bApplyClickable = IsApplyButtonClickable();
    const bool bNewHoverApply = bApplyClickable && IsOverApplyButton(LocalPos, Layout);
    
    if (NewHoveredPrimitive != HoveredPrimitive || NewHoveredCell != HoveredGridCell || bNewHoverApply != bHoveringApply)
    {
        HoveredPrimitive = NewHoveredPrimitive;
        HoveredGridCell = NewHoveredCell;
        bHoveringApply = bNewHoverApply;
        InvalidateWidget();
    }
    
    bool bClickable = (HoveredPrimitive >= 0) || (HoveredGridCell >= 0) || (bApplyClickable && bHoveringApply);
    UpdateCursorForState(SelectedPrimitive >= 0, bClickable);
}

void FReyOsterriethCopyProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || AreAllTrialsComplete()) return;
    const TSharedPtr<SDirectoryIndexRebuildWidget> CanvasPtr = CanvasWidget.Pin();
    if (!CanvasPtr.IsValid()) return;

    const FGeometry& Geometry = CanvasPtr->GetCachedGeometry();
    const FVector2D WidgetSize = Geometry.GetLocalSize();
    if (WidgetSize.IsNearlyZero()) return;

    const FVector2D LocalPos = Geometry.AbsoluteToLocal(Event.ScreenPixelPosition);
    const FCanvasLayout Layout = BuildCanvasLayout(WidgetSize);

    // Check palette click to select/deselect a shape
    const int32 PaletteItem = FindPaletteItemAtPosition(LocalPos, Layout);
    if (PaletteItem >= 0)
    {
        // Toggle selection
        SelectedPrimitive = (SelectedPrimitive == PaletteItem) ? -1 : PaletteItem;
        // Grabbing a shape starts fading the preview
        if (SelectedPrimitive >= 0 && bShowingTarget)
        {
            bShowingTarget = false;
            PreviewFadeStartTime = FPlatformTime::Seconds();
        }
        InvalidateWidget();
        return;
    }
    
    // Cannot interact with grid while showing target (must grab a shape first)
    if (bShowingTarget)
    {
        return;
    }
    
    // Check apply button
    if (IsApplyButtonClickable() && IsOverApplyButton(LocalPos, Layout))
    {
        CheckAndApply();
        return;
    }
    
    // Check grid click
    const int32 Cell = FindGridCellAtPosition(LocalPos, Layout);
    if (Cell >= 0)
    {
        if (SelectedPrimitive >= 0)
        {
            // Place selected shape (or toggle off if same)
            if (UserGrid[Cell] == SelectedPrimitive)
            {
                UserGrid[Cell] = -1;
            }
            else
            {
                UserGrid[Cell] = SelectedPrimitive;
            }
        }
        else if (UserGrid[Cell] >= 0)
        {
            // No shape selected, remove existing shape
            UserGrid[Cell] = -1;
        }
        InvalidateWidget();
    }
}

void FReyOsterriethCopyProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    // Nothing needed - click-based interaction
}

