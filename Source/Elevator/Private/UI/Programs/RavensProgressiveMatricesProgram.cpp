#include "UI/Programs/RavensProgressiveMatricesProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

REGISTER_SCREEN_PROGRAM(FRavensProgressiveMatricesProgram, "ravens_progressive_matrices")

// ============================================================================
// STemplateMatrixWidget Implementation
// ============================================================================

void STemplateMatrixWidget::Construct(const FArguments& InArgs)
{
    PrimaryColor = InArgs._PrimaryColor;
    DimColor = InArgs._DimColor;
    LineThickness = InArgs._LineThickness;
    Program = InArgs._Program;
}

FVector2D STemplateMatrixWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return FVector2D::ZeroVector;
}

int32 STemplateMatrixWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (!Program) return LayerId;
    
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FLinearColor Primary = PrimaryColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const FLinearColor Dim = DimColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const float Thickness = LineThickness.Get();
    const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
    
    const float CenterX = Size.X * 0.5f;
    const float CenterY = Size.Y * 0.5f;
    
    // Layout constants - large 3x3 grid, centered with options below (scaled to available space)
    const float CandidateRows = 2.0f;
    const float BaseGridCellSize = 158.0f;
    const float BaseCandidateRowHeight = BaseGridCellSize;  // Match grid cell size for square candidates
    const float BaseGap = 28.0f;
    const float BaseCandidatePadding = 12.0f;
    const float BaseGridSize = BaseGridCellSize * 3.0f;
    const float BaseTotalHeight = BaseGridSize + BaseGap + (BaseCandidateRowHeight * CandidateRows);

    const float ScaleX = Size.X / FMath::Max(1.0f, (BaseGridSize + BaseCandidatePadding * 2.0f + 48.0f));
    const float ScaleY = Size.Y / FMath::Max(1.0f, (BaseTotalHeight + 60.0f));
    const float Scale = FMath::Max(0.4f, FMath::Min3(1.0f, ScaleX, ScaleY));

    const float GridCellSize = BaseGridCellSize * Scale;
    const float GridSize = GridCellSize * 3.0f;
    const float CandidateRowHeight = BaseCandidateRowHeight * Scale;
    const float Gap = BaseGap * Scale;
    const float CandidatePadding = BaseCandidatePadding * Scale;
    
    // Total height of content
    const float TotalHeight = GridSize + Gap + (CandidateRowHeight * CandidateRows);
    const float ContentTop = CenterY - TotalHeight * 0.5f;
    
    const float GridLeft = CenterX - GridSize * 0.5f;
    const float GridTop = ContentTop;
    
    // Candidate options - 3 columns x 2 rows, aligned with grid width
    const float CandidateStartY = GridTop + GridSize + Gap;
    const float CandidateColWidth = GridSize / 3.0f;
    
    // === Draw 3x3 Matrix Grid ===
    {
        // Draw grid lines
        for (int32 i = 0; i <= 3; ++i)
        {
            // Horizontal lines
            TArray<FVector2D> HLine;
            HLine.Add(FVector2D(GridLeft, GridTop + i * GridCellSize));
            HLine.Add(FVector2D(GridLeft + GridSize, GridTop + i * GridCellSize));
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                HLine, DrawEffects, Dim * 0.5f, false, Thickness);
            
            // Vertical lines
            TArray<FVector2D> VLine;
            VLine.Add(FVector2D(GridLeft + i * GridCellSize, GridTop));
            VLine.Add(FVector2D(GridLeft + i * GridCellSize, GridTop + GridSize));
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                VLine, DrawEffects, Dim * 0.5f, false, Thickness);
        }
        
        // Draw shapes in cells
        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
            {
                const int32 CellIdx = Row * 3 + Col;
                const float CellCenterX = GridLeft + Col * GridCellSize + GridCellSize * 0.5f;
                const float CellCenterY = GridTop + Row * GridCellSize + GridCellSize * 0.5f;
                const FVector2D CellCenter(CellCenterX, CellCenterY);
                
                if (CellIdx == 8)
                {
                    // Blank cell - draw question mark
                    const FString QMark = TEXT("?");
                    const int32 FontSize = FMath::Max(36, FMath::RoundToInt(72.0f * Scale));
                    const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
                    FVector2D QSize(0.0f, 0.0f);
                    if (FSlateApplication::IsInitialized())
                    {
                        if (FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer())
                        {
                            if (TSharedPtr<FSlateFontMeasure> FontMeasure = Renderer->GetFontMeasureService())
                            {
                                QSize = FontMeasure->Measure(QMark, Font);
                            }
                        }
                    }
                    if (QSize.IsNearlyZero())
                    {
                        const float Approx = static_cast<float>(FontSize);
                        QSize = FVector2D(Approx * 0.6f, Approx);
                    }

                    const FVector2D QPos(CellCenterX - QSize.X * 0.5f, CellCenterY - QSize.Y * 0.5f);
                    FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
                        AllottedGeometry.ToPaintGeometry(FVector2f(QSize), FSlateLayoutTransform(FVector2f(QPos))),
                        QMark, Font, DrawEffects, Dim * 0.6f);
                }
                else if (Program->MatrixCells.IsValidIndex(CellIdx) && Program->MatrixCells[CellIdx] >= 0)
                {
                    Program->DrawShape(OutDrawElements, LayerId + 1, AllottedGeometry,
                        Program->MatrixCells[CellIdx], CellCenter, GridCellSize * 0.6f, Primary, Thickness);
                }
            }
        }
    }
    
    // === Draw Candidate Options (3x2 grid of tall rectangles) ===
    {
        for (int32 i = 0; i < Program->CandidateTiles.Num(); ++i)
        {
            const int32 Col = i % 3;
            const int32 Row = i / 3;
            
            const float BoxLeft = GridLeft + Col * CandidateColWidth + CandidatePadding;
            const float BoxRight = GridLeft + (Col + 1) * CandidateColWidth - CandidatePadding;
            const float BoxTop = CandidateStartY + Row * CandidateRowHeight + CandidatePadding;
            const float BoxBottom = CandidateStartY + (Row + 1) * CandidateRowHeight - CandidatePadding;
            
            const float OptCenterX = (BoxLeft + BoxRight) * 0.5f;
            const float OptCenterY = (BoxTop + BoxBottom) * 0.5f;
            const bool bHovered = (Program->HoveredOption == i);
            
            const FVector2D BoxTopLeft(BoxLeft, BoxTop);
            const FVector2D BoxBottomRight(BoxRight, BoxBottom);
            
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
            
            // Draw shape inside
            const FLinearColor ShapeColor = bHovered ? FLinearColor::Black : Primary;
            const float ShapeSize = FMath::Min(BoxRight - BoxLeft, BoxBottom - BoxTop) * 0.6f;
            Program->DrawShape(OutDrawElements, LayerId + 2, AllottedGeometry,
                Program->CandidateTiles[i], FVector2D(OptCenterX, OptCenterY), ShapeSize, ShapeColor, Thickness);
        }
    }
    
    return LayerId + 3;
}

// ============================================================================
// FRavensProgressiveMatricesProgram Implementation
// ============================================================================

FRavensProgressiveMatricesProgram::FRavensProgressiveMatricesProgram()
    : FTrialProgramBase(15)  // 10 puzzles
{
    SetTrialTitle(FText::FromString(TEXT("Raven's Progressive Matrices")));
    
    // Initialize usage counts for 12 patterns
    PatternUsageCounts.Init(0, 12);
    
    GenerateNewTrial();
}

void FRavensProgressiveMatricesProgram::GenerateNewTrial()
{
    MatrixCells.Empty();
    CandidateTiles.Empty();
    
    // Generate Raven's-style matrix patterns
    // IMPORTANT: Patterns must be deducible from what's visible on screen
    // We cannot use "increment" or "sequence" logic because shapes have no inherent order
    // Valid patterns: repetition, matching, completion (each row/col has same set)
    
    // Find least used patterns
    int32 MinUsage = MAX_int32;
    for (int32 Count : PatternUsageCounts)
    {
        if (Count < MinUsage) MinUsage = Count;
    }
    
    TArray<int32> Candidates;
    for (int32 i = 0; i < PatternUsageCounts.Num(); ++i)
    {
        if (PatternUsageCounts[i] == MinUsage)
        {
            // Avoid repeating the exact same pattern immediately if possible
            if (i != LastPatternType || PatternUsageCounts.Num() == 1)
            {
                Candidates.Add(i);
            }
        }
    }
    
    // If we filtered out everything (e.g. only one candidate and it was the last one), put it back
    if (Candidates.Num() == 0)
    {
        for (int32 i = 0; i < PatternUsageCounts.Num(); ++i)
        {
            if (PatternUsageCounts[i] == MinUsage) Candidates.Add(i);
        }
    }
    
    int32 PatternType = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
    PatternUsageCounts[PatternType]++;
    LastPatternType = PatternType;
    
    // Get 3 distinct shapes for the pattern
    TArray<int32> AllShapes;
    for (int32 i = 0; i < 6; ++i) AllShapes.Add(i);
    for (int32 i = AllShapes.Num() - 1; i > 0; --i)
    {
        const int32 j = FMath::RandRange(0, i);
        AllShapes.Swap(i, j);
    }
    const int32 A = AllShapes[0];
    const int32 B = AllShapes[1];
    const int32 C = AllShapes[2];
    
    switch (PatternType)
    {
        case 0:
        {
            // Pattern: Latin Square - each row and column contains A, B, C exactly once
            // The player can deduce the answer by checking what's missing in row 3 AND column 3
            // Row 0: A  B  C
            // Row 1: B  C  A
            // Row 2: C  A  ?
            // Row 2 has C, A -> needs B
            // Col 2 has C, A -> needs B
            // Answer: B
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(C);
            MatrixCells.Add(B); MatrixCells.Add(C); MatrixCells.Add(A);
            MatrixCells.Add(C); MatrixCells.Add(A); MatrixCells.Add(-1);
            CorrectAnswer = B;
            break;
        }
        case 1:
        {
            // Pattern: Another Latin Square variant
            // Row 0: A  B  C
            // Row 1: C  A  B
            // Row 2: B  C  ?
            // Row 2 has B, C -> needs A
            // Col 2 has C, B -> needs A
            // Answer: A
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(C);
            MatrixCells.Add(C); MatrixCells.Add(A); MatrixCells.Add(B);
            MatrixCells.Add(B); MatrixCells.Add(C); MatrixCells.Add(-1);
            CorrectAnswer = A;
            break;
        }
        case 2:
        {
            // Pattern: Symmetric matrix (transpose equals itself)
            // Cell[i,j] == Cell[j,i]
            //   A  B  C
            //   B  A  B
            //   C  B  ?
            // Position [2,2] - by symmetry, check [2,2] which is on diagonal
            // Looking at diagonal: A, A, ? -> pattern is A on diagonal
            // Answer: A
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(C);
            MatrixCells.Add(B); MatrixCells.Add(A); MatrixCells.Add(B);
            MatrixCells.Add(C); MatrixCells.Add(B); MatrixCells.Add(-1);
            CorrectAnswer = A;
            break;
        }
        case 3:
        {
            // Pattern: Each row has the same 3 shapes, but in different arrangements
            // This is a permutation pattern - player must identify what's missing from row 3
            // Row 0: A  B  C
            // Row 1: B  A  C  
            // Row 2: C  B  ?
            // Row 2 has C, B -> needs A
            // Col 2 has C, C -> but checking row is clearer, needs A
            // Answer: A
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(C);
            MatrixCells.Add(B); MatrixCells.Add(A); MatrixCells.Add(C);
            MatrixCells.Add(C); MatrixCells.Add(B); MatrixCells.Add(-1);
            CorrectAnswer = A;
            break;
        }
        case 4:
        {
            // Pattern: Constant Rows
            // Row 0: A  A  A
            // Row 1: B  B  B
            // Row 2: C  C  ?
            // Answer: C
            MatrixCells.Add(A); MatrixCells.Add(A); MatrixCells.Add(A);
            MatrixCells.Add(B); MatrixCells.Add(B); MatrixCells.Add(B);
            MatrixCells.Add(C); MatrixCells.Add(C); MatrixCells.Add(-1);
            CorrectAnswer = C;
            break;
        }
        case 5:
        {
            // Pattern: Constant Columns
            // Row 0: A  B  C
            // Row 1: A  B  C
            // Row 2: A  B  ?
            // Answer: C
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(C);
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(C);
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(-1);
            CorrectAnswer = C;
            break;
        }
        case 6:
        {
            // Pattern: Checkerboard / X
            // Row 0: A  B  A
            // Row 1: B  A  B
            // Row 2: A  B  ?
            // Answer: A
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(A);
            MatrixCells.Add(B); MatrixCells.Add(A); MatrixCells.Add(B);
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(-1);
            CorrectAnswer = A;
            break;
        }
        case 7:
        {
            // Pattern: Center Cross
            // Row 0: B  A  B
            // Row 1: A  A  A
            // Row 2: B  A  ?
            // Answer: B
            MatrixCells.Add(B); MatrixCells.Add(A); MatrixCells.Add(B);
            MatrixCells.Add(A); MatrixCells.Add(A); MatrixCells.Add(A);
            MatrixCells.Add(B); MatrixCells.Add(A); MatrixCells.Add(-1);
            CorrectAnswer = B;
            break;
        }
        case 8:
        {
            // Pattern: Cyclic Shift Left
            // Row 0: A  B  C
            // Row 1: B  C  A
            // Row 2: C  A  ?
            // Answer: B
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(C);
            MatrixCells.Add(B); MatrixCells.Add(C); MatrixCells.Add(A);
            MatrixCells.Add(C); MatrixCells.Add(A); MatrixCells.Add(-1);
            CorrectAnswer = B;
            break;
        }
        case 9:
        {
            // Pattern: Cyclic Shift Right
            // Row 0: A  B  C
            // Row 1: C  A  B
            // Row 2: B  C  ?
            // Answer: A
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(C);
            MatrixCells.Add(C); MatrixCells.Add(A); MatrixCells.Add(B);
            MatrixCells.Add(B); MatrixCells.Add(C); MatrixCells.Add(-1);
            CorrectAnswer = A;
            break;
        }
        case 10:
        {
            // Pattern: Vertical Mirror (Col 0 == Col 2)
            // Row 0: A  B  A
            // Row 1: C  A  C
            // Row 2: B  C  ?
            // Answer: B
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(A);
            MatrixCells.Add(C); MatrixCells.Add(A); MatrixCells.Add(C);
            MatrixCells.Add(B); MatrixCells.Add(C); MatrixCells.Add(-1);
            CorrectAnswer = B;
            break;
        }
        case 11:
        {
            // Pattern: Diamond / Center Unique
            // Row 0: A  B  A
            // Row 1: B  C  B
            // Row 2: A  B  ?
            // Answer: A
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(A);
            MatrixCells.Add(B); MatrixCells.Add(C); MatrixCells.Add(B);
            MatrixCells.Add(A); MatrixCells.Add(B); MatrixCells.Add(-1);
            CorrectAnswer = A;
            break;
        }
    }
    
    // Generate 6 candidate tiles
    // Include the correct answer plus the other shapes used in the puzzle
    // Plus some shapes NOT in the puzzle as harder distractors
    CandidateTiles.Empty();
    CandidateTiles.Add(CorrectAnswer);
    
    // Add the other shapes used in the matrix (A, B, C but not the answer)
    if (A != CorrectAnswer) CandidateTiles.Add(A);
    if (B != CorrectAnswer) CandidateTiles.Add(B);
    if (C != CorrectAnswer) CandidateTiles.Add(C);
    
    // Fill remaining slots with unused shapes (harder distractors)
    for (int32 i = 3; i < 6 && CandidateTiles.Num() < 6; ++i)
    {
        if (AllShapes[i] != CorrectAnswer)
        {
            CandidateTiles.Add(AllShapes[i]);
        }
    }
    
    // Ensure we have exactly 6
    while (CandidateTiles.Num() < 6)
    {
        for (int32 i = 0; i < 6 && CandidateTiles.Num() < 6; ++i)
        {
            if (!CandidateTiles.Contains(i))
            {
                CandidateTiles.Add(i);
            }
        }
    }
    
    // Sort candidates to have a constant layout (always 0-5 in order)
    CandidateTiles.Sort();
    
    HoveredOption = -1;
    InvalidateWidget();
}

void FRavensProgressiveMatricesProgram::DrawShape(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
    int32 ShapeType, const FVector2D& Center, float Size, const FLinearColor& Color, float Thickness) const
{
    const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
    const float HalfSize = Size * 0.7f;
    
    TArray<FVector2D> Points;
    
    switch (ShapeType)
    {
        case 0: // Square
        {
            Points.Add(FVector2D(Center.X - HalfSize, Center.Y - HalfSize));
            Points.Add(FVector2D(Center.X + HalfSize, Center.Y - HalfSize));
            Points.Add(FVector2D(Center.X + HalfSize, Center.Y + HalfSize));
            Points.Add(FVector2D(Center.X - HalfSize, Center.Y + HalfSize));
            Points.Add(FVector2D(Center.X - HalfSize, Center.Y - HalfSize));
            break;
        }
        case 1: // Diamond
        {
            Points.Add(FVector2D(Center.X, Center.Y - HalfSize));
            Points.Add(FVector2D(Center.X + HalfSize, Center.Y));
            Points.Add(FVector2D(Center.X, Center.Y + HalfSize));
            Points.Add(FVector2D(Center.X - HalfSize, Center.Y));
            Points.Add(FVector2D(Center.X, Center.Y - HalfSize));
            break;
        }
        case 2: // Triangle
        {
            Points.Add(FVector2D(Center.X, Center.Y - HalfSize));
            Points.Add(FVector2D(Center.X + HalfSize, Center.Y + HalfSize * 0.7f));
            Points.Add(FVector2D(Center.X - HalfSize, Center.Y + HalfSize * 0.7f));
            Points.Add(FVector2D(Center.X, Center.Y - HalfSize));
            break;
        }
        case 3: // Circle (octagon approximation)
        {
            const int32 Segments = 12;
            for (int32 i = 0; i <= Segments; ++i)
            {
                const float Angle = (float)i / (float)Segments * 2.0f * PI;
                Points.Add(FVector2D(Center.X + FMath::Cos(Angle) * HalfSize, Center.Y + FMath::Sin(Angle) * HalfSize));
            }
            break;
        }
        case 4: // Cross / Plus
        {
            const float ArmWidth = HalfSize * 0.4f;
            // Draw as two rectangles forming a cross
            TArray<FVector2D> Horiz, Vert;
            Horiz.Add(FVector2D(Center.X - HalfSize, Center.Y - ArmWidth));
            Horiz.Add(FVector2D(Center.X + HalfSize, Center.Y - ArmWidth));
            Horiz.Add(FVector2D(Center.X + HalfSize, Center.Y + ArmWidth));
            Horiz.Add(FVector2D(Center.X - HalfSize, Center.Y + ArmWidth));
            Horiz.Add(FVector2D(Center.X - HalfSize, Center.Y - ArmWidth));
            
            Vert.Add(FVector2D(Center.X - ArmWidth, Center.Y - HalfSize));
            Vert.Add(FVector2D(Center.X + ArmWidth, Center.Y - HalfSize));
            Vert.Add(FVector2D(Center.X + ArmWidth, Center.Y + HalfSize));
            Vert.Add(FVector2D(Center.X - ArmWidth, Center.Y + HalfSize));
            Vert.Add(FVector2D(Center.X - ArmWidth, Center.Y - HalfSize));
            
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                Horiz, DrawEffects, Color, true, Thickness);
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
                Vert, DrawEffects, Color, true, Thickness);
            return;
        }
        case 5: // Hexagon
        {
            for (int32 i = 0; i <= 6; ++i)
            {
                const float Angle = (float)i / 6.0f * 2.0f * PI - PI / 2.0f;
                Points.Add(FVector2D(Center.X + FMath::Cos(Angle) * HalfSize, Center.Y + FMath::Sin(Angle) * HalfSize));
            }
            break;
        }
        default:
            return;
    }
    
    if (Points.Num() > 0)
    {
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(),
            Points, DrawEffects, Color, true, Thickness);
    }
}

int32 FRavensProgressiveMatricesProgram::GetCandidateAtPosition(const FVector2D& LocalPos, const FVector2D& WidgetSize) const
{
    // Match layout from OnPaint exactly - 3x2 grid of candidates, centered
    const float CenterX = WidgetSize.X * 0.5f;
    const float CenterY = WidgetSize.Y * 0.5f;

    const float CandidateRows = 2.0f;
    const float BaseGridCellSize = 158.0f;
    const float BaseCandidateRowHeight = BaseGridCellSize;  // Match grid cell size for square candidates
    const float BaseGap = 28.0f;
    const float BaseCandidatePadding = 12.0f;
    const float BaseGridSize = BaseGridCellSize * 3.0f;
    const float BaseTotalHeight = BaseGridSize + BaseGap + (BaseCandidateRowHeight * CandidateRows);

    const float ScaleX = WidgetSize.X / FMath::Max(1.0f, (BaseGridSize + BaseCandidatePadding * 2.0f + 48.0f));
    const float ScaleY = WidgetSize.Y / FMath::Max(1.0f, (BaseTotalHeight + 60.0f));
    const float Scale = FMath::Max(0.4f, FMath::Min3(1.0f, ScaleX, ScaleY));

    const float GridCellSize = BaseGridCellSize * Scale;
    const float GridSize = GridCellSize * 3.0f;
    const float CandidateRowHeight = BaseCandidateRowHeight * Scale;
    const float Gap = BaseGap * Scale;
    const float CandidatePadding = BaseCandidatePadding * Scale;

    const float TotalHeight = GridSize + Gap + (CandidateRowHeight * CandidateRows);
    const float ContentTop = CenterY - TotalHeight * 0.5f;

    const float GridLeft = CenterX - GridSize * 0.5f;
    const float GridTop = ContentTop;

    const float CandidateStartY = GridTop + GridSize + Gap;
    const float CandidateColWidth = GridSize / 3.0f;
    
    // Check each of the 6 candidate boxes (3 cols x 2 rows)
    for (int32 i = 0; i < 6; ++i)
    {
        const int32 Col = i % 3;
        const int32 Row = i / 3;
        
        const float BoxLeft = GridLeft + Col * CandidateColWidth + CandidatePadding;
        const float BoxRight = GridLeft + (Col + 1) * CandidateColWidth - CandidatePadding;
        const float BoxTop = CandidateStartY + Row * CandidateRowHeight + CandidatePadding;
        const float BoxBottom = CandidateStartY + (Row + 1) * CandidateRowHeight - CandidatePadding;
        
        if (LocalPos.X >= BoxLeft && LocalPos.X <= BoxRight &&
            LocalPos.Y >= BoxTop && LocalPos.Y <= BoxBottom)
        {
            return i;
        }
    }
    
    return -1;
}

void FRavensProgressiveMatricesProgram::SelectOption(int32 Index)
{
    if (AreAllTrialsComplete()) return;
    if (Index < 0 || Index >= CandidateTiles.Num()) return;
    
    if (CandidateTiles[Index] == CorrectAnswer)
    {
        // Correct!
        if (AdvanceTrial())
        {
            SetTaskComplete(true);
        }
        else
        {
            GenerateNewTrial();
        }
    }
    else
    {
        // Wrong - generate new puzzle
        FailTrial();
        GenerateNewTrial();
    }
}

void FRavensProgressiveMatricesProgram::InvalidateWidget() const
{
    if (const TSharedPtr<SWidget> Widget = MatrixWidget.Pin())
    {
        Widget->Invalidate(EInvalidateWidget::Paint);
    }
}

TSharedRef<SWidget> FRavensProgressiveMatricesProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();

    TSharedRef<STemplateMatrixWidget> Matrix = SNew(STemplateMatrixWidget)
        .PrimaryColor(Style.GetPrimaryColor())
        .DimColor(Style.GetDimColor())
        .LineThickness(Style.LineThickness)
        .Program(this);

    MatrixWidget = Matrix;
    return Matrix;
}

void FRavensProgressiveMatricesProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || AreAllTrialsComplete())
    {
        HoveredOption = -1;
        return;
    }
    
    const TSharedPtr<SWidget> Widget = MatrixWidget.Pin();
    if (!Widget.IsValid()) return;
    
    const FGeometry& Geometry = Widget->GetCachedGeometry();
    const FVector2D LocalPos = Geometry.AbsoluteToLocal(Event.ScreenPixelPosition);
    const FVector2D WidgetSize = Geometry.GetLocalSize();
    
    const int32 NewHovered = GetCandidateAtPosition(LocalPos, WidgetSize);
    if (NewHovered != HoveredOption)
    {
        HoveredOption = NewHovered;
        InvalidateWidget();
    }
    
    UpdateCursorForState(false, HoveredOption >= 0);
}

void FRavensProgressiveMatricesProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || AreAllTrialsComplete()) return;
    
    ActivePointerKey = Event.TriggerKey;
    
    if (HoveredOption >= 0)
    {
        SelectOption(HoveredOption);
    }
}

void FRavensProgressiveMatricesProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    ActivePointerKey = EKeys::Invalid;
}

