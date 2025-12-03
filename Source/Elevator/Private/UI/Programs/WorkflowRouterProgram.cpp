#include "UI/Programs/WorkflowRouterProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"

REGISTER_SCREEN_PROGRAM(FWorkflowRouterProgram, "WorkflowRouter")

namespace
{
    constexpr float NodeRadiusNormalized = 0.075f;
    constexpr int32 WorkflowCircleSegments = 32;
    constexpr double FailureFadeDurationSeconds = 0.35;
    constexpr double SuccessAdvanceDelaySeconds = 0.45;
}

// ============================================================================
// STrailWidget Implementation
// ============================================================================

void STrailWidget::Construct(const FArguments& InArgs)
{
    PrimaryColor = InArgs._PrimaryColor;
    DimColor = InArgs._DimColor;
    LineThickness = InArgs._LineThickness;
    Program = InArgs._Program;
}

FVector2D STrailWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return FVector2D(400.0f, 300.0f);
}

int32 STrailWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (!Program) return LayerId;
    
    Program->ProcessPendingAdvance();

    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const float NodeRadius = NodeRadiusNormalized * FMath::Min(Size.X, Size.Y);
    const FLinearColor Primary = PrimaryColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const FLinearColor Dim = DimColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint();
    const float Thickness = FMath::Max(LineThickness.Get(), NodeRadius * 0.12f);
    const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;
    const double Now = FPlatformTime::Seconds();
    const float FailureAlpha = Program->GetFailureFadeAlpha(Now);
    
    const auto& Nodes = Program->Nodes;
    const int32 CurrentTarget = Program->CurrentTarget;
    const bool bDragging = Program->bDragging;
    const FVector2D CursorLocal = Program->CursorLocalPosition;

    if (Program->HasFailureFade())
    {
        if (FailureAlpha > 0.0f)
        {
            FLinearColor FadeColor = Primary;
            FadeColor.A *= FailureAlpha;

            for (const FWorkflowRouterProgram::FTrailSegment& Segment : Program->FailureSegments)
            {
                TArray<FVector2D> FadePoints;
                FadePoints.Add(FVector2D(Segment.StartNormalized.X * Size.X, Segment.StartNormalized.Y * Size.Y));
                FadePoints.Add(FVector2D(Segment.EndNormalized.X * Size.X, Segment.EndNormalized.Y * Size.Y));

                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    LayerId,
                    AllottedGeometry.ToPaintGeometry(),
                    FadePoints,
                    DrawEffects,
                    FadeColor,
                    true,
                    Thickness
                );
            }

            Program->InvalidateTrail();
        }
        else
        {
            Program->ClearFailureFade();
        }
    }
    
    // Draw connection lines between visited nodes
    for (int32 i = 1; i < CurrentTarget && i < Nodes.Num(); ++i)
    {
        const FVector2D From = FVector2D(
            Nodes[i-1].NormalizedPosition.X * Size.X,
            Nodes[i-1].NormalizedPosition.Y * Size.Y
        );
        const FVector2D To = FVector2D(
            Nodes[i].NormalizedPosition.X * Size.X,
            Nodes[i].NormalizedPosition.Y * Size.Y
        );
        
        TArray<FVector2D> LinePoints;
        LinePoints.Add(From);
        LinePoints.Add(To);
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            LinePoints,
            DrawEffects,
            Dim,
            true,
            Thickness
        );
    }
    
    // Draw line from last visited node to cursor if dragging
    if (bDragging && CurrentTarget > 0 && CurrentTarget < Nodes.Num())
    {
        const FVector2D From = FVector2D(
            Nodes[CurrentTarget-1].NormalizedPosition.X * Size.X,
            Nodes[CurrentTarget-1].NormalizedPosition.Y * Size.Y
        );
        const FVector2D To = CursorLocal;
        
        TArray<FVector2D> LinePoints;
        LinePoints.Add(From);
        LinePoints.Add(To);
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            LinePoints,
            DrawEffects,
            Primary,
            true,
            Thickness
        );
    }
    
    // Draw each node circle and label
    const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        const auto& Node = Nodes[i];
        const FVector2D Center = FVector2D(
            Node.NormalizedPosition.X * Size.X,
            Node.NormalizedPosition.Y * Size.Y
        );
        
        // Determine color based on state
        const FLinearColor NodeColor = Node.bVisited ? Dim : Primary;
        
        // Draw circle
        TArray<FVector2D> CirclePoints;
        for (int32 s = 0; s <= WorkflowCircleSegments; ++s)
        {
            const float Angle = (static_cast<float>(s) / WorkflowCircleSegments) * 2.0f * PI;
            CirclePoints.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * NodeRadius);
        }
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(),
            CirclePoints,
            DrawEffects,
            NodeColor,
            true,
            Thickness
        );
        
        // Draw label centered in circle
        const int32 FontSize = FMath::Clamp(static_cast<int32>(NodeRadius * 0.85f), 18, 54);
        const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
        const FVector2D TextSize = FontMeasure->Measure(Node.Label, Font);
        const FVector2D TextPos = Center - TextSize * 0.5f;
        
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TextPos))),
            Node.Label,
            Font,
            DrawEffects,
            NodeColor
        );
    }
    
    return LayerId + 3;
}

// ============================================================================
// FWorkflowRouterProgram Implementation
// ============================================================================

FWorkflowRouterProgram::FWorkflowRouterProgram()
    : FTrialProgramBase(5)
{
    SetTrialTitle(FText::FromString(TEXT("Workflow Router")));
    GenerateNodes();
}

void FWorkflowRouterProgram::GenerateNewTrial()
{
    CurrentPhase = ERoutePhase::PhaseA;
    GenerateNodes();
}

void FWorkflowRouterProgram::GenerateNodes()
{
    Nodes.Empty();
    CurrentTarget = 0;
    bDragging = false;
    bPendingAdvance = false;
    PendingAdvanceAction = EAdvanceAction::None;
    PendingAdvanceTime = 0.0;
    FailureSegments.Reset();
    FailureFadeStartTime = 0.0;
    
    bAlternating = (CurrentPhase == ERoutePhase::PhaseB);
    
    // MinDistance must be at least 2x radius to prevent overlap, plus small buffer
    const float MinDistance = NodeRadiusNormalized * 2.5f;  // ~0.1875, ensures no overlap
    const float Margin = NodeRadiusNormalized + 0.02f;      // Keep circles fully inside bounds
    const int32 NodeCount = GetCurrentTrial() + 2;  // Trial 0 = 2 nodes, Trial 6 = 8 nodes
    
    if (CurrentPhase == ERoutePhase::PhaseA)
    {
        // Serial mode: generate positions for numbers only and store them
        SerialNodePositions.Empty();
        
        for (int32 i = 0; i < NodeCount; ++i)
        {
            FVector2D NewPos = FVector2D(0.5f, 0.5f);
            bool bValidPosition = false;
            int32 Attempts = 0;
            
            while (!bValidPosition && Attempts < 100)
            {
                NewPos = FVector2D(
                    FMath::FRandRange(Margin, 1.0f - Margin),
                    FMath::FRandRange(Margin, 1.0f - Margin)
                );
                
                bValidPosition = true;
                for (const FVector2D& Existing : SerialNodePositions)
                {
                    if (FVector2D::Distance(NewPos, Existing) < MinDistance)
                    {
                        bValidPosition = false;
                        break;
                    }
                }
                Attempts++;
            }
            
            SerialNodePositions.Add(NewPos);
            
            FRouteNode Node;
            Node.NormalizedPosition = NewPos;
            Node.Label = FString::Printf(TEXT("%d"), i + 1);
            Node.bVisited = false;
            Nodes.Add(Node);
        }
    }
    else
    {
        // Multiplex mode: reuse number positions, add letters at new random positions
        TArray<FVector2D> AllPositions = SerialNodePositions;
        TArray<FVector2D> LetterPositions;
        
        // Generate new positions for letters (same count as numbers)
        for (int32 i = 0; i < NodeCount; ++i)
        {
            FVector2D NewPos = FVector2D(0.5f, 0.5f);
            bool bValidPosition = false;
            int32 Attempts = 0;
            
            while (!bValidPosition && Attempts < 100)
            {
                NewPos = FVector2D(
                    FMath::FRandRange(Margin, 1.0f - Margin),
                    FMath::FRandRange(Margin, 1.0f - Margin)
                );
                
                bValidPosition = true;
                for (const FVector2D& Existing : AllPositions)
                {
                    if (FVector2D::Distance(NewPos, Existing) < MinDistance)
                    {
                        bValidPosition = false;
                        break;
                    }
                }
                for (const FVector2D& Existing : LetterPositions)
                {
                    if (FVector2D::Distance(NewPos, Existing) < MinDistance)
                    {
                        bValidPosition = false;
                        break;
                    }
                }
                Attempts++;
            }
            
            LetterPositions.Add(NewPos);
        }
        
        // Interleave: 1, A, 2, B, 3, C, ...
        for (int32 i = 0; i < NodeCount; ++i)
        {
            // Add number node
            FRouteNode NumNode;
            NumNode.NormalizedPosition = SerialNodePositions[i];
            NumNode.Label = FString::Printf(TEXT("%d"), i + 1);
            NumNode.bVisited = false;
            Nodes.Add(NumNode);
            
            // Add letter node
            if (i < LetterPositions.Num())
            {
                FRouteNode LetterNode;
                LetterNode.NormalizedPosition = LetterPositions[i];
                LetterNode.Label = FString::Printf(TEXT("%c"), 'A' + i);
                LetterNode.bVisited = false;
                Nodes.Add(LetterNode);
            }
        }
    }

    InvalidateTrail();
}

void FWorkflowRouterProgram::ResetTrial()
{
    for (auto& Node : Nodes)
    {
        Node.bVisited = false;
    }
    CurrentTarget = 0;
    bDragging = false;
    InvalidateTrail();
}

void FWorkflowRouterProgram::CheckNodeIntersection(const FVector2D& NormalizedPos)
{
    if (!bDragging || GetTaskComplete()) return;
    
    // Check if we're intersecting the current target node
    if (CurrentTarget < Nodes.Num())
    {
        const float Distance = FVector2D::Distance(NormalizedPos, Nodes[CurrentTarget].NormalizedPosition);
        if (Distance < NodeRadiusNormalized)
        {
            // Hit the correct node!
            Nodes[CurrentTarget].bVisited = true;
            CurrentTarget++;
            InvalidateTrail();
            
            if (CurrentTarget >= Nodes.Num())
            {
                // Trial complete!
                bDragging = false;
                if (CurrentPhase == ERoutePhase::PhaseA)
                {
                    ScheduleAdvance(EAdvanceAction::PhaseB);
                }
                else
                {
                    ScheduleAdvance(EAdvanceAction::NextTrial);
                }
            }
        }
    }
}

void FWorkflowRouterProgram::InvalidateTrail() const
{
    if (const TSharedPtr<SWidget> Trail = TrailWidget.Pin())
    {
        Trail->Invalidate(EInvalidateWidget::Paint);
    }
}

void FWorkflowRouterProgram::ScheduleAdvance(EAdvanceAction Action)
{
    if (Action == EAdvanceAction::None)
    {
        return;
    }

    bPendingAdvance = true;
    PendingAdvanceAction = Action;
    PendingAdvanceTime = FPlatformTime::Seconds() + SuccessAdvanceDelaySeconds;
    InvalidateTrail();
}

void FWorkflowRouterProgram::ProcessPendingAdvance()
{
    if (!bPendingAdvance)
    {
        return;
    }

    const double Now = FPlatformTime::Seconds();
    if (Now < PendingAdvanceTime)
    {
        InvalidateTrail();
        return;
    }

    bPendingAdvance = false;
    const EAdvanceAction Action = PendingAdvanceAction;
    PendingAdvanceAction = EAdvanceAction::None;

    switch (Action)
    {
    case EAdvanceAction::PhaseB:
        CurrentPhase = ERoutePhase::PhaseB;
        GenerateNodes();
        break;
    case EAdvanceAction::NextTrial:
        CurrentPhase = ERoutePhase::PhaseA;
        AdvanceTrial();  // Handles incrementing trial and calling GenerateNewTrial or SetTaskComplete
        break;
    default:
        break;
    }
}

void FWorkflowRouterProgram::StartFailureFade()
{
    FailureSegments.Reset();

    if (Nodes.Num() == 0)
    {
        return;
    }

    // Add segments for visited path
    if (CurrentTarget > 1)
    {
        for (int32 i = 1; i < CurrentTarget; ++i)
        {
            const FTrailSegment Segment{
                Nodes[i - 1].NormalizedPosition,
                Nodes[i].NormalizedPosition
            };
            FailureSegments.Add(Segment);
        }
    }

    if (CurrentTarget > 0)
    {
        const FTrailSegment CursorSegment{
            Nodes[CurrentTarget - 1].NormalizedPosition,
            CursorNormalizedPosition
        };
        FailureSegments.Add(CursorSegment);
    }

    FailureFadeStartTime = FPlatformTime::Seconds();
    InvalidateTrail();
}

void FWorkflowRouterProgram::ClearFailureFade()
{
    if (FailureSegments.Num() > 0)
    {
        FailureSegments.Reset();
        FailureFadeStartTime = 0.0;
        InvalidateTrail();
    }
}

float FWorkflowRouterProgram::GetFailureFadeAlpha(double Now) const
{
    if (FailureSegments.Num() == 0 || FailureFadeStartTime <= 0.0)
    {
        return 0.0f;
    }

    const double Elapsed = Now - FailureFadeStartTime;
    if (Elapsed <= 0.0)
    {
        return 1.0f;
    }

    const double Ratio = 1.0 - (Elapsed / FailureFadeDurationSeconds);
    return static_cast<float>(FMath::Clamp(Ratio, 0.0, 1.0));
}

FText FWorkflowRouterProgram::GetStatusText() const
{
    if (GetTaskComplete())
    {
        return FText::FromString(TEXT("ROUTER SEALED"));
    }

    const bool bPhaseSwitchPending = bPendingAdvance && PendingAdvanceAction == EAdvanceAction::PhaseB;
    const ERoutePhase DisplayPhase = bPhaseSwitchPending ? ERoutePhase::PhaseB : CurrentPhase;
    const TCHAR* PhaseLabel = (DisplayPhase == ERoutePhase::PhaseA) ? TEXT("ROUTE-A") : TEXT("ROUTE-B");
    const int32 DisplayTrial = FMath::Clamp(GetCurrentTrial() + 1, 1, GetTotalTrials());

    return FText::FromString(FString::Printf(TEXT("RUN %02d // %s"), DisplayTrial, PhaseLabel));
}

TSharedRef<SWidget> FWorkflowRouterProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();

    TSharedPtr<STrailWidget> Trail;

    TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
        // Phase Indicator
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, Style.GetSmallPadding())
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(Style.GetSmallPadding(), 0)
            [SNew(STextBlock)
                .Text_Lambda([this]() {
                    return CurrentPhase == ERoutePhase::PhaseA ? FText::FromString(TEXT("[ SERIAL ]")) : FText::FromString(TEXT("  SERIAL  "));
                })
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", Style.TextSize))
                .ColorAndOpacity_Lambda([this, Style]() {
                    return CurrentPhase == ERoutePhase::PhaseA ? Style.GetPrimaryColor() : Style.GetDimColor();
                })]
            + SHorizontalBox::Slot().AutoWidth().Padding(Style.GetSmallPadding(), 0)
            [SNew(STextBlock)
                .Text_Lambda([this]() {
                    return CurrentPhase == ERoutePhase::PhaseB ? FText::FromString(TEXT("[ MULTIPLEX ]")) : FText::FromString(TEXT("  MULTIPLEX  "));
                })
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", Style.TextSize))
                .ColorAndOpacity_Lambda([this, Style]() {
                    return CurrentPhase == ERoutePhase::PhaseB ? Style.GetPrimaryColor() : Style.GetDimColor();
                })]
        ]

        // Action Status
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, Style.GetSmallPadding())
        [SNew(STextBlock)
            .Text_Lambda([this]() {
                if (GetTaskComplete()) return FText::FromString(TEXT("ROUTING COMPLETE"));
                if (HasFailureFade()) return FText::FromString(TEXT("SIGNAL LOST"));
                if (bPendingAdvance) return FText::FromString(TEXT("LINK ESTABLISHED"));
                if (bDragging) return FText::FromString(TEXT("TRACING SIGNAL..."));
                return FText::FromString(TEXT("WAITING FOR INPUT"));
            })
            .Font(FCoreStyle::GetDefaultFontStyle("Mono", Style.TextSize))
            .ColorAndOpacity(Style.GetPrimaryColor())]

        + SVerticalBox::Slot().FillHeight(1.0f).Padding(Style.GetSmallPadding())
        [SAssignNew(Trail, STrailWidget)
            .PrimaryColor(Style.GetPrimaryColor())
            .DimColor(Style.GetDimColor())
            .LineThickness(2.0f)
            .Program(this)];

    TrailWidget = Trail;
    return Content;
}

void FWorkflowRouterProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || GetTaskComplete())
    {
        ResetCursor();
        return;
    }
    
    FVector2D LocalPosition;
    CursorNormalizedPosition = GetNormalizedPosition(Event.ScreenPixelPosition, &LocalPosition);
    CursorLocalPosition = LocalPosition;
    
    if (bDragging)
    {
        CheckNodeIntersection(CursorNormalizedPosition);
        SetCursorGrabbing();
        InvalidateTrail();
    }
    else
    {
        // Check if hovering over first node
        if (CurrentTarget == 0 && Nodes.Num() > 0)
        {
            const float Distance = FVector2D::Distance(CursorNormalizedPosition, Nodes[0].NormalizedPosition);
            if (Distance < NodeRadiusNormalized)
            {
                SetCursorHand();
                return;
            }
        }
        ResetCursor();
        InvalidateTrail();
    }
}

void FWorkflowRouterProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || GetTaskComplete()) return;
    if (bPendingAdvance) return;
    
    FVector2D LocalPosition;
    CursorNormalizedPosition = GetNormalizedPosition(Event.ScreenPixelPosition, &LocalPosition);
    CursorLocalPosition = LocalPosition;
    
    // Can only start by clicking on the first node
    if (CurrentTarget == 0 && Nodes.Num() > 0)
    {
        const float Distance = FVector2D::Distance(CursorNormalizedPosition, Nodes[0].NormalizedPosition);
        if (Distance < NodeRadiusNormalized)
        {
            bDragging = true;
            Nodes[0].bVisited = true;
            CurrentTarget = 1;
            SetCursorGrabbing();
            InvalidateTrail();
        }
    }
}

void FWorkflowRouterProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (GetTaskComplete()) return;
    
    if (bDragging && CurrentTarget < Nodes.Num())
    {
        StartFailureFade();
        ResetTrial();
    }
    
    ResetCursor();
    InvalidateTrail();
}

FVector2D FWorkflowRouterProgram::GetNormalizedPosition(const FVector2D& PixelPosition, FVector2D* OutLocalPosition) const
{
    const TSharedPtr<SWidget> Trail = TrailWidget.Pin();
    if (!Trail.IsValid())
    {
        if (OutLocalPosition)
        {
            *OutLocalPosition = FVector2D::ZeroVector;
        }
        return FVector2D(0.5f, 0.5f);
    }

    const FGeometry& Geometry = Trail->GetCachedGeometry();
    const FVector2D LocalPos = Geometry.AbsoluteToLocal(PixelPosition);
    if (OutLocalPosition)
    {
        *OutLocalPosition = LocalPos;
    }
    const FVector2D Size = Geometry.GetLocalSize();

    if (Size.X <= KINDA_SMALL_NUMBER || Size.Y <= KINDA_SMALL_NUMBER)
    {
        if (OutLocalPosition)
        {
            *OutLocalPosition = FVector2D::ZeroVector;
        }
        return FVector2D(0.5f, 0.5f);
    }

    return FVector2D(
        FMath::Clamp(LocalPos.X / Size.X, 0.0f, 1.0f),
        FMath::Clamp(LocalPos.Y / Size.Y, 0.0f, 1.0f)
    );
}

