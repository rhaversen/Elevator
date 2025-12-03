#include "UI/Programs/StopSignalVisualProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "HAL/PlatformTime.h"

REGISTER_SCREEN_PROGRAM(FStopSignalVisualProgram, "stop_signal_visual")

FStopSignalVisualProgram::FStopSignalVisualProgram()
    : FTrialProgramBase(25)
{
    SetTrialTitle(FText::FromString(TEXT("Stop-Signal Task")));
    GenerateNewTrial();
}

void FStopSignalVisualProgram::GenerateNewTrial()
{
    // 25% of trials will have a stop signal
    bIsStopTrial = FMath::RandRange(0, 100) < 25;
    
    if (bIsStopTrial)
    {
        // 30% of stop trials start with STOP visible immediately
        // Rest have variable delay (100-400ms)
        if (FMath::RandRange(0, 100) < 30)
        {
            StopSignalDelay = 0.0f;  // STOP visible from start
        }
        else
        {
            StopSignalDelay = FMath::RandRange(0.1f, 0.4f);
        }
    }
    else
    {
        StopSignalDelay = 999.0f;  // Never show stop on go trials
    }
    
    StartTrialTimer(1.5f);
}

void FStopSignalVisualProgram::OnTrialTimerExpired()
{
    if (bIsStopTrial)
    {
        // Neutral: Successfully inhibited response on stop trial - just generate new trial
        // No progress gained, no penalty
        GenerateNewTrial();
    }
    else
    {
        // Wrong: Failed to click SEND in time on go trial - reset
        FailTrial();
    }
}

bool FStopSignalVisualProgram::IsStopSignalVisible() const
{
    if (!bIsStopTrial) return false;
    float Elapsed = GetTimerProgress() * 1.5f;
    return Elapsed >= StopSignalDelay;
}

void FStopSignalVisualProgram::ClickSend()
{
    if (AreAllTrialsComplete()) return;
    
    if (IsStopSignalVisible())
    {
        // Wrong: Clicked SEND after stop signal appeared - failed to inhibit
        FailTrial();
    }
    else
    {
        // Correct: Clicked SEND before stop signal (or on a go trial)
        AdvanceTrial();
    }
}

TSharedRef<SWidget> FStopSignalVisualProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const int32 LargeTextSize = Style.TextSize * 2;

    return SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(STextBlock).Text_Lambda([this]() {
                    static const FText StopText = FText::FromString(TEXT("STOP"));
                    static const FText ArrowText = FText::FromString(TEXT(">>>"));
                    return IsStopSignalVisible() ? StopText : ArrowText;
                }).Font(FCoreStyle::GetDefaultFontStyle("Bold", LargeTextSize * 3))
                .ColorAndOpacity_Lambda([this]() -> FSlateColor {
                    if (IsStopSignalVisible())
                        return GetProgramStyle().GetErrorColor();
                    return GetProgramStyle().GetPrimaryColor();
                })
            ]
            // Timer bar
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, Style.GetLargePadding())
            [
                BuildTimerBarWidget(
                    [this]() { return GetTimerProgress(); },
                    [this]() -> FSlateColor {
                        if (IsStopSignalVisible())
                            return GetProgramStyle().GetErrorColor();
                        return GetProgramStyle().GetPrimaryColor();
                    })
            ]
        ];
}

TSharedRef<SWidget> FStopSignalVisualProgram::BuildTrialFooter()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, GetProgramStyle().GetSmallPadding())
        [
            BuildButton(
                FText::FromString(TEXT("SEND")),
                TAttribute<bool>::CreateLambda([this]() { return bSendHovered; }),
                TAttribute<bool>::CreateLambda([this]() { return false; })
            )
        ];
}

void FStopSignalVisualProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor) return;
    
    // SEND button hitbox - bottom 25% of screen, center 50%
    const FVector2D Size = GetProgramSize();
    bool bInButtonY = Event.ProgramPixelPosition.Y > Size.Y * 0.75f;
    bool bInButtonX = Event.ProgramPixelPosition.X > Size.X * 0.25f && Event.ProgramPixelPosition.X < Size.X * 0.75f;
    bSendHovered = bInButtonY && bInButtonX;
    
    UpdateCursorForState(false, bSendHovered);
}

void FStopSignalVisualProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor) return;
    ActivePointerKey = Event.TriggerKey;
    
    if (bSendHovered)
    {
        ClickSend();
    }
}

void FStopSignalVisualProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    ActivePointerKey = EKeys::Invalid;
}

