#include "UI/Programs/TabRestoreOrderProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "HAL/PlatformTime.h"

namespace
{
    const FText TimeoutReplayText = FText::FromString(TEXT("Timeout. Rewinding stack..."));
    const FText MismatchReplayText = FText::FromString(TEXT("Bad pointer. Rewinding stack..."));
}

REGISTER_SCREEN_PROGRAM(FTabRestoreOrderProgram, "TabRestoreOrder")

FTabRestoreOrderProgram::FTabRestoreOrderProgram()
    : FTrialProgramBase(MaxLength)
{
    SetTrialTitle(FText::FromString(TEXT("PROCESS RESTORE")));
    SetTaskComplete(false);
    GenerateSequence();
}

void FTabRestoreOrderProgram::GenerateNewTrial()
{
    SetTaskComplete(false);
    GenerateSequence();
}

void FTabRestoreOrderProgram::GenerateSequence()
{
    SequenceLength = FMath::Clamp(GetCurrentTrial() + 1, 1, MaxLength);
    TargetSequence.Empty();
    PlayerSequence.Empty();

    for (int32 Index = 0; Index < SequenceLength; ++Index)
    {
        TargetSequence.Add(FMath::RandRange(0, NumBlocks - 1));
    }

    Phase = EPhase::ShowingSequence;
    HoveredBlock = -1;
    HighlightedBlock = -1;
    FeedbackBlock = -1;
    ShowingIndex = -1;
    bFlashVisible = false;
    bAdvanceAfterFeedback = false;
    bFailAfterFeedback = false;

    const double Now = FPlatformTime::Seconds();
    NextPhaseTime = Now + IntroPauseDuration;
    ResponseStartTime = 0.0;
    ResponseDuration = 0.0;

    const int32 AddressCount = TargetSequence.Num();
    const FText& FormatText = (AddressCount == 1)
        ? FText::FromString(TEXT("Reading {0} address..."))
        : FText::FromString(TEXT("Reading {0} addresses..."));
    StatusMessage = FText::Format(FormatText, FText::AsNumber(AddressCount));

    SetTaskComplete(false);
}

void FTabRestoreOrderProgram::UpdateState()
{
    const double Now = FPlatformTime::Seconds();

    switch (Phase)
    {
    case EPhase::ShowingSequence:
        if (Now >= NextPhaseTime)
        {
            if (!bFlashVisible)
            {
                ++ShowingIndex;
                if (ShowingIndex >= TargetSequence.Num())
                {
                    StartInputPhase();
                }
                else
                {
                    HighlightedBlock = TargetSequence[ShowingIndex];
                    bFlashVisible = true;
                    NextPhaseTime = Now + HighlightDuration;
                }
            }
            else
            {
                HighlightedBlock = -1;
                bFlashVisible = false;
                NextPhaseTime = Now + InterStimulusDuration;
            }
        }
        break;

    case EPhase::AwaitingInput:
        if (ResponseDuration > 0.0)
        {
            const double Elapsed = Now - ResponseStartTime;
            if (Elapsed >= ResponseDuration)
            {
                const int32 ExpectedBlock = TargetSequence.IsValidIndex(PlayerSequence.Num()) ? TargetSequence[PlayerSequence.Num()] : -1;
                StartFeedback(false, ExpectedBlock, TOptional<FText>(TimeoutReplayText));
            }
        }
        break;

    case EPhase::Feedback:
        if (Now >= NextPhaseTime)
        {
            if (bFailAfterFeedback)
            {
                bFailAfterFeedback = false;
                FailTrial();
                return;
            }

            if (bAdvanceAfterFeedback)
            {
                bAdvanceAfterFeedback = false;
                const bool bCompletedAll = AdvanceTrial();
                if (bCompletedAll)
                {
                    StatusMessage = FText::FromString(TEXT("Process restored."));
                }
                return;
            }

            if (!IsTaskComplete())
            {
                GenerateSequence();
            }
        }
        break;
    }
}

void FTabRestoreOrderProgram::StartInputPhase()
{
    Phase = EPhase::AwaitingInput;
    HighlightedBlock = -1;
    FeedbackBlock = -1;
    bFlashVisible = false;
    ShowingIndex = -1;
    bAdvanceAfterFeedback = false;
    bFailAfterFeedback = false;

    const double Now = FPlatformTime::Seconds();
    ResponseStartTime = Now;
    ResponseDuration = TargetSequence.Num() * ResponseTimePerItem;

    StatusMessage = FText::Format(FText::FromString(TEXT("Re-stack the session ({0}/{1})")), FText::AsNumber(PlayerSequence.Num()), FText::AsNumber(TargetSequence.Num()));
}

void FTabRestoreOrderProgram::StartFeedback(bool bSuccess, int32 ErrorBlock, TOptional<FText> CustomMessage)
{
    Phase = EPhase::Feedback;
    HighlightedBlock = -1;
    FeedbackBlock = bSuccess ? -1 : ErrorBlock;
    bFlashVisible = false;
    ShowingIndex = -1;

    const double Now = FPlatformTime::Seconds();
    NextPhaseTime = Now + FeedbackDuration;
    ResponseStartTime = 0.0;
    ResponseDuration = 0.0;

    if (CustomMessage.IsSet())
    {
        StatusMessage = CustomMessage.GetValue();
    }
    else if (bSuccess)
    {
        StatusMessage = FText::FromString(TEXT("Stack valid. Extending..."));
    }
    else
    {
        StatusMessage = FText::FromString(TEXT("Stack fault. Rewinding..."));
    }

    bAdvanceAfterFeedback = bSuccess;
    bFailAfterFeedback = !bSuccess;
}

const TArray<FTabRestoreOrderProgram::FTabDescriptor>& FTabRestoreOrderProgram::GetTabDescriptors()
{
    static const TArray<FTabDescriptor> Descriptors = {
        {FText::FromString(TEXT("PID.7"))},
        {FText::FromString(TEXT("SYS.0"))},
        {FText::FromString(TEXT("USR.3"))},
        {FText::FromString(TEXT("TMP.1"))},
        {FText::FromString(TEXT("VAR.9"))},
        {FText::FromString(TEXT("OPT.4"))},
        {FText::FromString(TEXT("ETC.2"))},
        {FText::FromString(TEXT("DEV.6"))},
        {FText::FromString(TEXT("MNT.8"))}
    };
    return Descriptors;
}

const FTabRestoreOrderProgram::FTabDescriptor& FTabRestoreOrderProgram::GetTabDescriptor(int32 BlockIndex) const
{
    const TArray<FTabDescriptor>& Descriptors = GetTabDescriptors();
    check(Descriptors.Num() > 0);
    const int32 SafeIndex = (Descriptors.Num() > 0) ? (BlockIndex % Descriptors.Num()) : 0;
    return Descriptors[SafeIndex];
}

int32 FTabRestoreOrderProgram::GetBlockAtPosition(const FVector2D& Position) const
{
    const TSharedPtr<SWidget> Grid = GridWidget.Pin();
    if (!Grid.IsValid())
    {
        return -1;
    }

    const FGeometry& Geometry = Grid->GetCachedGeometry();
    const FVector2D LocalPos = Geometry.AbsoluteToLocal(Position);
    const FVector2D Size = Geometry.GetLocalSize();

    if (Size.X <= KINDA_SMALL_NUMBER || Size.Y <= KINDA_SMALL_NUMBER)
    {
        return -1;
    }

    if (LocalPos.X < 0.0f || LocalPos.Y < 0.0f || LocalPos.X > Size.X || LocalPos.Y > Size.Y)
    {
        return -1;
    }

    const float CellWidth = Size.X / 3.0f;
    const float CellHeight = Size.Y / 3.0f;

    const int32 Col = FMath::Clamp(static_cast<int32>(LocalPos.X / CellWidth), 0, 2);
    const int32 Row = FMath::Clamp(static_cast<int32>(LocalPos.Y / CellHeight), 0, 2);

    return Row * 3 + Col;
}

void FTabRestoreOrderProgram::TapBlock(int32 BlockIndex)
{
    if (Phase != EPhase::AwaitingInput || BlockIndex < 0)
    {
        return;
    }

    PlayerSequence.Add(BlockIndex);

    const int32 CurrentIndex = PlayerSequence.Num() - 1;
    if (!TargetSequence.IsValidIndex(CurrentIndex) || TargetSequence[CurrentIndex] != BlockIndex)
    {
        StartFeedback(false, BlockIndex, TOptional<FText>(MismatchReplayText));
        return;
    }

    if (PlayerSequence.Num() == TargetSequence.Num())
    {
        const bool bFinalTrial = (GetCurrentTrial() + 1 >= GetTotalTrials());
        if (bFinalTrial)
        {
            StartFeedback(true, -1, TOptional<FText>(FText::FromString(TEXT("Process restored."))));
        }
        else
        {
            StartFeedback(true);
        }
        return;
    }

    StatusMessage = FText::Format(FText::FromString(TEXT("Rebuild stack ({0}/{1})")), FText::AsNumber(PlayerSequence.Num()), FText::AsNumber(TargetSequence.Num()));
}

FText FTabRestoreOrderProgram::GetStatusText() const
{
    return StatusMessage;
}

float FTabRestoreOrderProgram::GetResponseProgress() const
{
    if (Phase != EPhase::AwaitingInput || ResponseDuration <= 0.0)
    {
        return 0.0f;
    }

    const double Now = FPlatformTime::Seconds();
    const double Elapsed = Now - ResponseStartTime;
    return FMath::Clamp(static_cast<float>(Elapsed / ResponseDuration), 0.0f, 1.0f);
}

FSlateColor FTabRestoreOrderProgram::GetBlockTextColor(int32 BlockIndex) const
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    
    if (GetTaskComplete())
    {
        return Style.GetDimColor();
    }
    
    if (Phase == EPhase::Feedback && FeedbackBlock == BlockIndex && FeedbackBlock != -1)
    {
        return Style.GetErrorColor();
    }
    
    // During preview, highlighted block is BRIGHT, others are very dim
    if (Phase == EPhase::ShowingSequence)
    {
        if (HighlightedBlock == BlockIndex && bFlashVisible)
        {
            // Extra bright for highlighted block during preview
            return FSlateColor(FLinearColor::Green * 1.5f);
        }
        else
        {
            // Very dim for non-highlighted blocks during preview
            return FSlateColor(FLinearColor::Green * 0.15f);
        }
    }
    
    if (Phase == EPhase::AwaitingInput)
    {
        if (HoveredBlock == BlockIndex)
        {
            return Style.GetPrimaryColor();
        }
    }
    
    return Style.GetDimColor();
}

TSharedRef<SWidget> FTabRestoreOrderProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const int32 BlockFontSize = Style.TextSize + 4;

    TSharedPtr<SBorder> GridContainer;
    
    // Pure text-based block widget - no borders, just text with brackets
    auto MakeBlockWidget = [this, &Style, BlockFontSize](int32 BlockIndex) -> TSharedRef<SWidget>
    {
        const FTabDescriptor& Descriptor = GetTabDescriptor(BlockIndex);
        const FText LabelText = Descriptor.Label;

        return SNew(SBox)
            .Padding(FMargin(Style.GetSmallPadding()))
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [SNew(STextBlock)
                .Text_Lambda([this, BlockIndex, LabelText]() {
                    FString BlockStr;
                    bool bIsHighlighted = (Phase == EPhase::ShowingSequence && HighlightedBlock == BlockIndex && bFlashVisible);
                    bool bIsPreviewDim = (Phase == EPhase::ShowingSequence && !(HighlightedBlock == BlockIndex && bFlashVisible));
                    bool bIsHovered = (Phase == EPhase::AwaitingInput && HoveredBlock == BlockIndex);
                    bool bIsError = (Phase == EPhase::Feedback && FeedbackBlock == BlockIndex && FeedbackBlock != -1);
                    
                    if (bIsHighlighted)
                    {
                        // Active during preview - bright and visible
                        BlockStr = FString::Printf(TEXT("[%s]"), *LabelText.ToString());
                    }
                    else if (bIsError)
                    {
                        // Wrong selection
                        BlockStr = FString::Printf(TEXT("!%s!"), *LabelText.ToString());
                    }
                    else if (bIsPreviewDim)
                    {
                        // Background during preview - blank
                        BlockStr = TEXT("  ---  ");
                    }
                    else if (bIsHovered)
                    {
                        // Hovering - lifting up
                        BlockStr = FString::Printf(TEXT("/%s\\"), *LabelText.ToString());
                    }
                    else
                    {
                        // Normal state
                        BlockStr = FString::Printf(TEXT("|%s|"), *LabelText.ToString());
                    }
                    return FText::FromString(BlockStr);
                })
                .Justification(ETextJustify::Center)
                .Font(FCoreStyle::GetDefaultFontStyle("Mono", BlockFontSize))
                .ColorAndOpacity_Lambda([this, BlockIndex]() -> FSlateColor { return GetBlockTextColor(BlockIndex); })];
    };

    TSharedRef<SUniformGridPanel> GridPanel = SNew(SUniformGridPanel)
        .SlotPadding(FMargin(Style.GetSmallPadding() * 0.5f));

    for (int32 Row = 0; Row < 3; ++Row)
    {
        for (int32 Col = 0; Col < 3; ++Col)
        {
            const int32 BlockIndex = Row * 3 + Col;
            GridPanel->AddSlot(Col, Row)[MakeBlockWidget(BlockIndex)];
        }
    }

    TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, Style.GetSmallPadding())
        [SNew(STextBlock)
            .Text_Lambda([this]() { return GetStatusText(); })
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", Style.TextSize))
            .ColorAndOpacity(Style.GetPrimaryColor())]
        + SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(Style.GetSmallPadding())
        [SNew(SBox)
            .WidthOverride(TAttribute<FOptionalSize>::CreateLambda([this]()
            {
                return FOptionalSize(GetProgramSize().X * (GridAnchorRight - GridAnchorLeft));
            }))
            .HeightOverride(TAttribute<FOptionalSize>::CreateLambda([this]()
            {
                return FOptionalSize(GetProgramSize().Y * (GridAnchorBottom - GridAnchorTop));
            }))
            [SAssignNew(GridContainer, SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("NoBrush"))
                .Padding(FMargin(0.0f))
                [GridPanel]
            ]]
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, Style.GetSmallPadding())
        [BuildTimerBarWidget([this]() { return GetResponseProgress(); })];

    GridWidget = GridContainer;
    return Content;
}

void FTabRestoreOrderProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || GetTaskComplete())
    {
        return;
    }

    UpdateState();
    HoveredBlock = GetBlockAtPosition(Event.ProgramPixelPosition);
    const bool bAllowInput = Phase == EPhase::AwaitingInput && HoveredBlock >= 0;
    UpdateCursorForState(false, bAllowInput);
}

void FTabRestoreOrderProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor || GetTaskComplete())
    {
        return;
    }

    ActivePointerKey = Event.TriggerKey;
    UpdateState();

    if (Phase == EPhase::AwaitingInput && HoveredBlock >= 0)
    {
        TapBlock(HoveredBlock);
    }
}

void FTabRestoreOrderProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    ActivePointerKey = EKeys::Invalid;
    if (!bHandledByPreProcessor)
    {
        UpdateState();
    }
}
