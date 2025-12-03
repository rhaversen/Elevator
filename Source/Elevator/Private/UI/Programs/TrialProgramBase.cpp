#include "UI/Programs/TrialProgramBase.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "HAL/PlatformTime.h"

#define LOCTEXT_NAMESPACE "TrialProgramBase"

FTrialProgramBase::FTrialProgramBase(int32 InTotalTrials)
    : TotalTrials(InTotalTrials)
{
}

FText FTrialProgramBase::GetTrialTitle() const
{
    if (bHasTrialTitle)
    {
        return TrialTitle;
    }

    return LOCTEXT("DefaultTrialTitle", "Trial Task");
}

void FTrialProgramBase::SetTrialTitle(const FText& InTitle)
{
    TrialTitle = InTitle;
    bHasTrialTitle = !InTitle.IsEmpty();
}


TSharedRef<SWidget> FTrialProgramBase::BuildTrialBody()
{
    checkf(false, TEXT("BuildTrialBody must be overridden when using the default trial layout."));
    return SNullWidget::NullWidget;
}

bool FTrialProgramBase::AdvanceTrial()
{
    StopTrialTimer();
    CurrentTrial++;
    if (CurrentTrial >= TotalTrials)
    {
        SetTaskComplete(true);
        return true;
    }
    GenerateNewTrial();
    return false;
}

void FTrialProgramBase::FailTrial()
{
    StopTrialTimer();
    CurrentTrial = 0;
    GenerateNewTrial();
}

void FTrialProgramBase::StartTrialTimer(float Duration)
{
    TrialDuration = Duration;
    TrialStartTime = FPlatformTime::Seconds();
    bTimerActive = true;
}

void FTrialProgramBase::StopTrialTimer()
{
    bTimerActive = false;
}

float FTrialProgramBase::GetTimerProgress() const
{
    if (!bTimerActive || TrialDuration <= 0.0f)
    {
        return 0.0f;
    }
    
    double Elapsed = FPlatformTime::Seconds() - TrialStartTime;
    return FMath::Clamp(static_cast<float>(Elapsed / TrialDuration), 0.0f, 1.0f);
}

void FTrialProgramBase::OnTrialTimerExpired()
{
    FailTrial();
}

FString FTrialProgramBase::GetProgressBarString() const
{
    return BuildProgressBarString(CurrentTrial, TotalTrials);
}

TSharedRef<SWidget> FTrialProgramBase::BuildProgressBar() const
{
    return BuildProgressBarWidget([this]() { return CurrentTrial; }, TotalTrials);
}

TSharedRef<SWidget> FTrialProgramBase::BuildTrialFooter()
{
    return SNullWidget::NullWidget;
}

TSharedRef<SWidget> FTrialProgramBase::BuildProgramWidget()
{
    const FScreenProgramStyle& Style = GetProgramStyle();

    TSharedRef<SWidget> MainContent = SNew(SVerticalBox)
        // Header
        + SVerticalBox::Slot().AutoHeight()
        [
            BuildHeader(GetTrialTitle())
        ]
        // Progress Bar
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(Style.GetSmallPadding())
        [
            BuildProgressBar()
        ]
        // Body
        + SVerticalBox::Slot().FillHeight(1.0f)
        [
            BuildTrialBody()
        ]
        // Footer
        + SVerticalBox::Slot().AutoHeight()
        [
            BuildTrialFooter()
        ];

    return MainContent;
}

void FTrialProgramBase::OnTick(float DeltaTime)
{
    if (bTimerActive && !IsTaskComplete())
    {
        if (GetTimerProgress() >= 1.0f)
        {
            OnTrialTimerExpired();
        }
    }
}

#undef LOCTEXT_NAMESPACE

