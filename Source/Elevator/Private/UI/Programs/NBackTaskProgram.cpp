#include "UI/Programs/NBackTaskProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "Internationalization/Text.h"

REGISTER_SCREEN_PROGRAM(FNBackTaskProgram, "n_back_task")

// ============================================================
// SFollowUpMarkerWidget
// ============================================================

void SFollowUpMarkerWidget::Construct(const FArguments& InArgs)
{
    Program = InArgs._Program;
}

FVector2D SFollowUpMarkerWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return Program ? Program->GetProgramSize() : FVector2D(400, 300);
}

int32 SFollowUpMarkerWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
    const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (!Program) return LayerId;
    
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FLinearColor Primary = Program->GetProgramStyle().GetPrimaryColor();
    const FLinearColor Dim = Program->GetProgramStyle().GetDimColor().GetSpecifiedColor();
    const FLinearColor Background = Program->GetProgramStyle().GetBackgroundColor();
    const float Thickness = Program->GetProgramStyle().LineThickness;

    auto MeasureText = [](const FString& Text, const FSlateFontInfo& Font) -> FVector2D
    {
        FVector2D Size(0.0f, 0.0f);

        if (FSlateApplication::IsInitialized())
        {
            if (FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer())
            {
                const TSharedRef<FSlateFontMeasure> FontMeasure = Renderer->GetFontMeasureService();
                const UE::Slate::FDeprecateVector2DResult MeasureResult = FontMeasure->Measure(FText::FromString(Text), Font, 1.0f);
                const FVector2f MeasureFloat = UE::Slate::CastToVector2f(MeasureResult);
                Size = FVector2D(MeasureFloat.X, MeasureFloat.Y);
            }
        }

        if (Size.IsNearlyZero())
        {
            const float FontSize = static_cast<float>(Font.Size);
            const float ApproxWidth = FontSize * 0.6f * Text.Len();
            Size = FVector2D(ApproxWidth, FontSize);
        }

        return Size;
    };

    auto DrawCenteredText = [&](const FString& Text, const FSlateFontInfo& Font, const FVector2D& Center,
        int32 InLayer, const FLinearColor& Color)
    {
        const FVector2D TextSize = MeasureText(Text, Font);
        const FVector2D TopLeft = Center - TextSize * 0.5f;
        FSlateDrawElement::MakeText(OutDrawElements, InLayer,
            AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TopLeft))),
            Text, Font, ESlateDrawEffect::None, Color);
    };
    
    // Check if complete
    if (Program->GetTaskComplete())
    {
        FSlateFontInfo CompleteFont = FCoreStyle::GetDefaultFontStyle("Bold", 32);
        const FString Completion = TEXT("Monitor clear.");
        FSlateDrawElement::MakeText(OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2f(300, 44), FSlateLayoutTransform(FVector2f(Size.X * 0.5f - 100.0f, Size.Y * 0.45f))),
            Completion, CompleteFont, ESlateDrawEffect::None, Primary);
        return LayerId + 1;
    }
    
    // ========================================
    // CANDIDATE - positioned above equals sign, in column with n-2
    // ========================================
    FSlateFontInfo CandidateFont = FCoreStyle::GetDefaultFontStyle("Bold", 72);
    
    // Get candidate code (current trial's code that should match n-2)
    FString CandidateCode = TEXT("-");
    if (Program->TrialIndex >= 0 && Program->TrialIndex < Program->Sequence.Num())
    {
        CandidateCode = Program->Sequence[Program->TrialIndex];
    }
    
    // Position candidate above the equals sign (which is above n-2)
    // Equal spacing: 40px gap between candidate bottom and equals top, 40px gap between equals bottom and n-2 top
    FVector2D Slot3Pos = Program->GetSlotPosition(3, 1.0f);
    const float CandidateY = Slot3Pos.Y - 242.0f;  // Above equals sign with equal spacing
    FVector2D CandidateSize(200, 130);  // Same size as slots
    float CandidateLeft = Slot3Pos.X - CandidateSize.X * 0.5f;
    float CandidateTop = CandidateY - CandidateSize.Y * 0.5f;
    
    TArray<FVector2D> CandidateRect;
    CandidateRect.Add(FVector2D(CandidateLeft, CandidateTop));
    CandidateRect.Add(FVector2D(CandidateLeft + CandidateSize.X, CandidateTop));
    CandidateRect.Add(FVector2D(CandidateLeft + CandidateSize.X, CandidateTop + CandidateSize.Y));
    CandidateRect.Add(FVector2D(CandidateLeft, CandidateTop + CandidateSize.Y));
    CandidateRect.Add(FVector2D(CandidateLeft, CandidateTop));
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
        CandidateRect, ESlateDrawEffect::None, Primary, true, Thickness * 1.5f);
    
    // Center text in candidate box using precise measurement
    DrawCenteredText(CandidateCode, CandidateFont, FVector2D(Slot3Pos.X, CandidateY), LayerId + 1, Primary);
    
    // ========================================
    // CONVEYOR: 4 slots [n+1] [n+0] [n-1] [n-2]
    // n+1, n+0 visible; n-1, n-2 show "?"
    // ========================================
    const FVector2D SlotSize = Program->GetSlotSize();
    FSlateFontInfo CodeFont = FCoreStyle::GetDefaultFontStyle("Bold", 72);
    float AnimProgress = Program->GetAnimProgress();
    
    // Calculate fade for the n+0 -> n-1 transition (slot 1 becoming slot 2)
    // During animation, text fades out and ? fades in
    float TransitionAlpha = Program->GetTransitionAlpha();  // 0 = showing text, 1 = showing ?
    const FString QuestionMarkSymbol = TEXT("?");
    const FString DashSymbol = TEXT("-");

    // Draw 4 slots: n+1 (fading in), n+0, n-1, n-2 (plus n-3 fading out below)
    for (int32 SlotIdx = 0; SlotIdx < 4; ++SlotIdx)
    {
        // SlotIdx 0 = n+1 (fading in as it arrives)
        // SlotIdx 1 = n+0
        // SlotIdx 2 = n-1 (transition: text fades to ?)
        // SlotIdx 3 = n-2
        // We also draw SlotIdx 4 = n-3 (outgoing, fading out to right) separately below
        
        bool bIsIncomingSlot = (SlotIdx == 0);  // n+1 fading in as it arrives
        
        FVector2D SlotCenter = Program->GetSlotPosition(SlotIdx, AnimProgress);
        
        const float SlotLeft = SlotCenter.X - SlotSize.X * 0.5f;
        const float SlotTop = SlotCenter.Y - SlotSize.Y * 0.5f;
        
        // n-1 and n-2 are hidden (slots 2 and 3)
        bool bIsHidden = (SlotIdx >= 2);
        
        // Calculate alpha for incoming slot (n+1 fading in as it arrives)
        float SlotAlpha = 1.0f;
        if (bIsIncomingSlot)
        {
            SlotAlpha = TransitionAlpha;  // Fade in as animation progresses (0->1)
        }
        
        // Slot outline
        TArray<FVector2D> SlotRect;
        SlotRect.Add(FVector2D(SlotLeft, SlotTop));
        SlotRect.Add(FVector2D(SlotLeft + SlotSize.X, SlotTop));
        SlotRect.Add(FVector2D(SlotLeft + SlotSize.X, SlotTop + SlotSize.Y));
        SlotRect.Add(FVector2D(SlotLeft, SlotTop + SlotSize.Y));
        SlotRect.Add(FVector2D(SlotLeft, SlotTop));
        
        FLinearColor OutlineColor = Dim * 0.6f;
        OutlineColor.A = SlotAlpha;
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            SlotRect, ESlateDrawEffect::None, OutlineColor, true, Thickness * 1.2f);
        
        // Slot content
        // Get sequence index for this slot
        // n+1 = TrialIndex + 1, n+0 = TrialIndex, n-1 = TrialIndex - 1, n-2 = TrialIndex - 2
        int32 SeqIdx = Program->TrialIndex + (1 - SlotIdx);
        
        // Slot 2 (n-1) is the transition zone: text fades out, ? fades in
        // During animation, the item from n+0 is moving into n-1
        bool bIsTransitionSlot = (SlotIdx == 2);
        
        if (bIsHidden)
        {
            // Check if n-2 should be revealed (showing feedback after MATCH click)
            bool bRevealN2 = (SlotIdx == 3) && Program->bShowingFeedback;
            
            // For slot 2 (n-1), show both fading text AND fading ?
            if (bIsTransitionSlot)
            {
                // Fade in the ?
                FLinearColor QColor = Dim * 0.5f;
                QColor.A = TransitionAlpha;
                DrawCenteredText(QuestionMarkSymbol, CodeFont, SlotCenter, LayerId + 1, QColor);
                
                // Also fade out the text that's moving into this slot
                if (SeqIdx >= 0 && SeqIdx < Program->Sequence.Num())
                {
                    FLinearColor TextColor = Dim;
                    TextColor.A = 1.0f - TransitionAlpha;
                    DrawCenteredText(Program->Sequence[SeqIdx], CodeFont, SlotCenter, LayerId + 1, TextColor);
                }
            }
            else if (bRevealN2 && SeqIdx >= 0 && SeqIdx < Program->Sequence.Num())
            {
                // Reveal n-2 in green (like original)
                FLinearColor RevealColor = Primary;  // Always green
                DrawCenteredText(Program->Sequence[SeqIdx], CodeFont, SlotCenter, LayerId + 1, RevealColor);
            }
            else
            {
                // Slot 3 (n-2) just shows ? at normal brightness
                DrawCenteredText(QuestionMarkSymbol, CodeFont, SlotCenter, LayerId + 1, Dim);
            }
        }
        else if (SeqIdx >= 0 && SeqIdx < Program->Sequence.Num())
        {
            // Visible slots show text - apply fade alpha for incoming slot
            FLinearColor TextColor = Dim;
            TextColor.A = SlotAlpha;
            DrawCenteredText(Program->Sequence[SeqIdx], CodeFont, SlotCenter, LayerId + 1, TextColor);
        }
        else
        {
            // Empty slot - dash centered
            FLinearColor DashColor = Dim * 0.3f;
            DashColor.A = SlotAlpha;
            DrawCenteredText(DashSymbol, CodeFont, SlotCenter, LayerId + 1, DashColor);
        }
    }
    
    // Draw n-3 slot (outgoing, fading out) during animation
    if (TransitionAlpha < 1.0f)
    {
        FVector2D OutgoingCenter = Program->GetSlotPosition(4, AnimProgress);
        const float OutgoingLeft = OutgoingCenter.X - SlotSize.X * 0.5f;
        const float OutgoingTop = OutgoingCenter.Y - SlotSize.Y * 0.5f;
        float OutgoingAlpha = 1.0f - TransitionAlpha;  // Fade out as animation progresses
        
        // Outline
        TArray<FVector2D> OutgoingRect;
        OutgoingRect.Add(FVector2D(OutgoingLeft, OutgoingTop));
        OutgoingRect.Add(FVector2D(OutgoingLeft + SlotSize.X, OutgoingTop));
        OutgoingRect.Add(FVector2D(OutgoingLeft + SlotSize.X, OutgoingTop + SlotSize.Y));
        OutgoingRect.Add(FVector2D(OutgoingLeft, OutgoingTop + SlotSize.Y));
        OutgoingRect.Add(FVector2D(OutgoingLeft, OutgoingTop));
        
        FLinearColor OutgoingOutlineColor = Dim * 0.6f;
        OutgoingOutlineColor.A = OutgoingAlpha;
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
            OutgoingRect, ESlateDrawEffect::None, OutgoingOutlineColor, true, Thickness * 1.2f);
        
        // Content - show ? fading out
        FLinearColor OutgoingQColor = Dim;
        OutgoingQColor.A = OutgoingAlpha;
        DrawCenteredText(QuestionMarkSymbol, CodeFont, OutgoingCenter, LayerId + 1, OutgoingQColor);
    }
    
    // ========================================
    // EQUALS SIGN button between candidate and n-2
    // ========================================
    const FVector2D MatchCenter = Program->GetMatchButtonCenter();
    const FVector2D MatchSize = Program->GetMatchButtonSize();
    
    bool bCanClick = !Program->bAlreadyResponded;
    // Normal state: dimmed but visible; Hovered: bright primary
    FLinearColor EqualsColor = Program->bMatchHovered && bCanClick ? Primary : Dim * 0.7f;
    
    // Draw equals sign as two horizontal lines (big)
    float LineWidth = 70.0f;
    float LineHeight = 10.0f;
    float LineGap = 22.0f;
    
    // Draw a subtle box outline around the equals sign to show it's clickable
    float BoxPadding = 15.0f;
    float BoxWidth = LineWidth + BoxPadding * 2.0f;
    float BoxHeight = LineGap + LineHeight + BoxPadding * 2.0f;
    TArray<FVector2D> ButtonOutline;
    ButtonOutline.Add(FVector2D(MatchCenter.X - BoxWidth * 0.5f, MatchCenter.Y - BoxHeight * 0.5f));
    ButtonOutline.Add(FVector2D(MatchCenter.X + BoxWidth * 0.5f, MatchCenter.Y - BoxHeight * 0.5f));
    ButtonOutline.Add(FVector2D(MatchCenter.X + BoxWidth * 0.5f, MatchCenter.Y + BoxHeight * 0.5f));
    ButtonOutline.Add(FVector2D(MatchCenter.X - BoxWidth * 0.5f, MatchCenter.Y + BoxHeight * 0.5f));
    ButtonOutline.Add(FVector2D(MatchCenter.X - BoxWidth * 0.5f, MatchCenter.Y - BoxHeight * 0.5f));
    FLinearColor OutlineColor = Program->bMatchHovered && bCanClick ? Primary : Dim * 0.4f;
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
        ButtonOutline, ESlateDrawEffect::None, OutlineColor, true, Thickness);
    
    // Top line of equals
    TArray<FVector2D> TopLine;
    TopLine.Add(FVector2D(MatchCenter.X - LineWidth * 0.5f, MatchCenter.Y - LineGap * 0.5f - LineHeight * 0.5f));
    TopLine.Add(FVector2D(MatchCenter.X + LineWidth * 0.5f, MatchCenter.Y - LineGap * 0.5f - LineHeight * 0.5f));
    TopLine.Add(FVector2D(MatchCenter.X + LineWidth * 0.5f, MatchCenter.Y - LineGap * 0.5f + LineHeight * 0.5f));
    TopLine.Add(FVector2D(MatchCenter.X - LineWidth * 0.5f, MatchCenter.Y - LineGap * 0.5f + LineHeight * 0.5f));
    TopLine.Add(FVector2D(MatchCenter.X - LineWidth * 0.5f, MatchCenter.Y - LineGap * 0.5f - LineHeight * 0.5f));
    
    // Bottom line of equals
    TArray<FVector2D> BottomLine;
    BottomLine.Add(FVector2D(MatchCenter.X - LineWidth * 0.5f, MatchCenter.Y + LineGap * 0.5f - LineHeight * 0.5f));
    BottomLine.Add(FVector2D(MatchCenter.X + LineWidth * 0.5f, MatchCenter.Y + LineGap * 0.5f - LineHeight * 0.5f));
    BottomLine.Add(FVector2D(MatchCenter.X + LineWidth * 0.5f, MatchCenter.Y + LineGap * 0.5f + LineHeight * 0.5f));
    BottomLine.Add(FVector2D(MatchCenter.X - LineWidth * 0.5f, MatchCenter.Y + LineGap * 0.5f + LineHeight * 0.5f));
    BottomLine.Add(FVector2D(MatchCenter.X - LineWidth * 0.5f, MatchCenter.Y + LineGap * 0.5f - LineHeight * 0.5f));
    
    // Fill the equals sign bars
    FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
        AllottedGeometry.ToPaintGeometry(FVector2f(LineWidth, LineHeight), 
            FSlateLayoutTransform(FVector2f(MatchCenter.X - LineWidth * 0.5f, MatchCenter.Y - LineGap * 0.5f - LineHeight * 0.5f))),
        FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, EqualsColor);
    FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
        AllottedGeometry.ToPaintGeometry(FVector2f(LineWidth, LineHeight), 
            FSlateLayoutTransform(FVector2f(MatchCenter.X - LineWidth * 0.5f, MatchCenter.Y + LineGap * 0.5f - LineHeight * 0.5f))),
        FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, EqualsColor);
    
    return LayerId + 1;
}

// ============================================================
// FNBackTaskProgram
// ============================================================

FNBackTaskProgram::FNBackTaskProgram()
    : FTrialProgramBase(TrialsRequired)
{
    SetTrialTitle(FText::FromString(TEXT("N-Back Task")));
    GenerateSequence();
    StartTrialTimer(TimePerTrial);
}

void FNBackTaskProgram::GenerateNewTrial()
{
    AdvanceSequence();
}

void FNBackTaskProgram::GenerateSequence()
{
    Sequence.Empty();
    MatchesGenerated = 0;
    NonMatchesGenerated = 0;
    SetTaskComplete(false);
    
    // Generate initial items (need at least 4 for the display: n+1, n+0, n-1, n-2)
    // We start at TrialIndex=2, so we need items 0,1,2,3 initially
    for (int32 i = 0; i < 4; ++i)
    {
        GenerateNextItem();
    }
    
    // Start at trial 2 so we have n-2 history
    TrialIndex = 2;
    bAlreadyResponded = false;
    bShowingFeedback = false;
    bLastMatchCorrect = false;
    bAdvanceTrialQueued = false;
    bFailTrialQueued = false;
}

void FNBackTaskProgram::GenerateNextItem()
{
    static const TArray<FString> Pool = {TEXT("TKT"), TEXT("REF"), TEXT("ACK"), TEXT("SYN"), TEXT("FIN")};
    
    const int32 Index = Sequence.Num();

    // Seed the first two entries with random values (no n-2 history yet)
    if (Index < 2)
    {
        Sequence.Add(Pool[FMath::RandRange(0, Pool.Num() - 1)]);
        return;
    }

    auto MakeNonMatchingItem = [&](int32 HistoryOffset) -> FString
    {
        FString Candidate;
        do
        {
            Candidate = Pool[FMath::RandRange(0, Pool.Num() - 1)];
        } while (Candidate == Sequence[HistoryOffset]);
        return Candidate;
    };

    const int32 BalanceTolerance = 1; // Keep match/non-match counts within this delta

    bool bShouldMatch = false;
    if (MatchesGenerated < NonMatchesGenerated - BalanceTolerance)
    {
        bShouldMatch = true;
    }
    else if (NonMatchesGenerated < MatchesGenerated - BalanceTolerance)
    {
        bShouldMatch = false;
    }
    else
    {
        bShouldMatch = FMath::RandBool();
    }

    if (bShouldMatch)
    {
        // Copy the string before adding to avoid self-referencing during potential reallocation
        FString MatchItem = Sequence[Index - 2];
        Sequence.Add(MatchItem);
        MatchesGenerated++;
    }
    else
    {
        Sequence.Add(MakeNonMatchingItem(Index - 2));
        NonMatchesGenerated++;
    }
}

float FNBackTaskProgram::GetAnimProgress() const
{
    if (GetTaskComplete()) return 1.0f;
    return GetTimerProgress();
}

void FNBackTaskProgram::AdvanceSequence()
{
    TrialIndex++;
    StartTrialTimer(TimePerTrial);
    bAlreadyResponded = false;
    bShowingFeedback = false;  // Clear feedback when advancing
    
    // Generate next item on the fly
    GenerateNextItem();
    
    InvalidateWidget();
}

void FNBackTaskProgram::InvalidateWidget() const
{
    if (CanvasWidget.IsValid())
    {
        CanvasWidget.Pin()->Invalidate(EInvalidateWidget::Paint);
    }
}

FVector2D FNBackTaskProgram::GetSlotPosition(int32 SlotIndex, float AnimProgress) const
{
    FVector2D Size = GetProgramSize();
    if (TSharedPtr<SFollowUpMarkerWidget> Widget = CanvasWidget.Pin())
    {
        Size = Widget->GetCachedGeometry().GetLocalSize();
    }
    
    // Slot positions, evenly spaced (200px wide slots need ~230px spacing)
    // SlotIndex -1 = n+2 (incoming, off-left), 0 = n+1 (leftmost visible), 
    // 3 = n-2 (rightmost visible), 4 = n-3 (outgoing, off-right)
    float SlotSpacing = 230.0f;
    float TotalWidth = SlotSpacing * 3.0f;  // Distance from slot 0 to slot 3 center
    float BaseX = Size.X * 0.5f - TotalWidth * 0.5f;
    float TargetX = BaseX + SlotIndex * SlotSpacing;
    
    // Animate: slots move from left to right during trial
    // At start (AnimProgress=0), they're at their "previous" position (one slot left)
    // At end (AnimProgress=1), they're at target position
    // Use ease in-out (smooth start and end)
    float t = FMath::Min(AnimProgress * 3.0f, 1.0f);  // Animation completes in first third of trial
    float EasedProgress = t < 0.5f 
        ? 4.0f * t * t * t  // Ease in
        : 1.0f - FMath::Pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;  // Ease out
    float AnimOffset = SlotSpacing * (1.0f - EasedProgress);
    
    return FVector2D(TargetX - AnimOffset, Size.Y * 0.65f);  // Lower on screen for room above
}

float FNBackTaskProgram::GetTransitionAlpha() const
{
    // Returns 0-1 alpha for text->? fade during animation
    // Fade happens during animation period with ease-out curve (fast start, slow end)
    float t = FMath::Min(GetAnimProgress() * 3.0f, 1.0f);
    // Ease-out quadratic: starts fast, slows down at end
    return 1.0f - (1.0f - t) * (1.0f - t);
}

FVector2D FNBackTaskProgram::GetSlotSize() const
{
    return FVector2D(200.0f, 130.0f);
}

FVector2D FNBackTaskProgram::GetMatchButtonCenter() const
{
    // Between candidate and n-2 slot (in a vertical column)
    // Equal spacing: 40px gap above and below the equals button
    // Equals visual half-height ~16px, slot half-height 65px
    // Equals bottom at Slot3Pos.Y - 65 - 40 = Slot3Pos.Y - 105
    // Equals center at Slot3Pos.Y - 105 - 16 = Slot3Pos.Y - 121
    FVector2D Slot3Pos = GetSlotPosition(3, 1.0f);  // Use settled position
    return FVector2D(Slot3Pos.X, Slot3Pos.Y - 121.0f);
}

FVector2D FNBackTaskProgram::GetMatchButtonSize() const
{
    return FVector2D(80.0f, 80.0f);  // Square for equals sign
}

bool FNBackTaskProgram::IsPointInRect(const FVector2D& Point, const FVector2D& Center, const FVector2D& Size) const
{
    return FMath::Abs(Point.X - Center.X) <= Size.X * 0.5f && FMath::Abs(Point.Y - Center.Y) <= Size.Y * 0.5f;
}

TSharedRef<SWidget> FNBackTaskProgram::BuildTrialBody()
{
    TSharedPtr<SFollowUpMarkerWidget> Canvas;
    TSharedRef<SWidget> Widget = SAssignNew(Canvas, SFollowUpMarkerWidget).Program(this);
    CanvasWidget = Canvas;
    
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1.0f)
        [
            Widget
        ]
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, GetProgramStyle().GetLargePadding())
        [
            BuildTimerBarWidget([this]() { return GetTimerProgress(); })
        ];
}

void FNBackTaskProgram::OnTick(float DeltaTime)
{
    FTrialProgramBase::OnTick(DeltaTime);

    if (GetTaskComplete())
    {
        return;
    }

    if (GetAnimProgress() >= 1.0f)
    {
        if (bAdvanceTrialQueued)
        {
            bAdvanceTrialQueued = false;
            AdvanceTrial();
        }
        else if (bFailTrialQueued)
        {
            bFailTrialQueued = false;
            FailTrial();
        }
        else
        {
            AdvanceSequence();
        }
    }
}

void FNBackTaskProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || GetTaskComplete()) return;
    
    FVector2D Pos = Event.ProgramPixelPosition;
    if (TSharedPtr<SFollowUpMarkerWidget> Widget = CanvasWidget.Pin())
    {
        Pos = Widget->GetCachedGeometry().AbsoluteToLocal(Event.ScreenPixelPosition);
    }
    
    bMatchHovered = IsPointInRect(Pos, GetMatchButtonCenter(), GetMatchButtonSize());
    
    UpdateCursorForState(false, bMatchHovered && !bAlreadyResponded);
    
    // Always invalidate for animation
    InvalidateWidget();
}

void FNBackTaskProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || GetTaskComplete()) return;
    if (bAlreadyResponded) return;
    
    ActivePointerKey = Event.TriggerKey;
    
    if (bMatchHovered)
    {
        bMatchPressed = true;
        bAlreadyResponded = true;
        bShowingFeedback = true;  // Show the n-2 reveal
        
        // Check if candidate (TrialIndex) matches n-2 (TrialIndex - 2)
        bool bIsMatch = false;
        if (TrialIndex >= 2 && TrialIndex < Sequence.Num())
        {
            bIsMatch = (Sequence[TrialIndex] == Sequence[TrialIndex - 2]);
        }
        
        bLastMatchCorrect = bIsMatch;  // Store for feedback display
        
        if (bIsMatch)
        {
            bAdvanceTrialQueued = true;
            bFailTrialQueued = false;
        }
        else
        {
            bFailTrialQueued = true;
            bAdvanceTrialQueued = false;
        }
        
        InvalidateWidget();
    }
}

void FNBackTaskProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    bMatchPressed = false;
    ActivePointerKey = EKeys::Invalid;
    UpdateCursorForState(false, bMatchHovered && !bAlreadyResponded);
}

