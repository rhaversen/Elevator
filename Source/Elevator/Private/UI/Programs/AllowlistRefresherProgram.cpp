#include "UI/Programs/AllowlistRefresherProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "HAL/PlatformTime.h"

#define LOCTEXT_NAMESPACE "AllowlistRefresherProgram"

REGISTER_SCREEN_PROGRAM(FAllowlistRefresherProgram, "AllowlistRefresher")

FAllowlistRefresherProgram::FAllowlistRefresherProgram()
    : FTrialProgramBase(20)  // 20 trials
{
    SetTrialTitle(LOCTEXT("TrialTitle", "ALLOWLIST"));
    GenerateNewTrial();
}

void FAllowlistRefresherProgram::GenerateNewTrial()
{
    // 60% Go trials (benign), 40% No-Go trials (hazard)
    bCurrentTrialIsBenign = FMath::RandRange(0, 100) < 60;
    bTrialApproved = false;
    StartTrialTimer(1.0f);
}

TSharedRef<SWidget> FAllowlistRefresherProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const int32 IconSize = Style.TextSize * 6;
    
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(GetCurrentIcon()); })
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", IconSize))
                .ColorAndOpacity_Lambda([this]() -> FSlateColor {
                    if (AreAllTrialsComplete()) return GetProgramStyle().GetDimColor();
                    // Dim if already approved (waiting for timer)
                    if (bTrialApproved) return GetProgramStyle().GetDimColor();
                    // Green for benign, red for hazard
                    return bCurrentTrialIsBenign ? GetProgramStyle().GetPrimaryColor() : GetProgramStyle().GetErrorColor();
                })
            ]
            // Timer bar
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, Style.GetLargePadding())
            [
                BuildTimerBarWidget(
                    [this]() { return GetTimerProgress(); },
                    [this]() -> FSlateColor {
                        // Dim if already approved
                        if (bTrialApproved) return GetProgramStyle().GetDimColor();
                        return bCurrentTrialIsBenign ? GetProgramStyle().GetPrimaryColor() : GetProgramStyle().GetErrorColor();
                    })
            ]
        ];
}

TSharedRef<SWidget> FAllowlistRefresherProgram::BuildTrialFooter()
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, GetProgramStyle().GetSmallPadding())
        [
            BuildButton(
                LOCTEXT("ApproveButtonLabel", "APPROVE"),
                TAttribute<bool>::CreateLambda([this]() { return bApproveHovered; }),
                TAttribute<bool>::CreateLambda([this]() { return bApprovePressed; })
            )
        ];
}

void FAllowlistRefresherProgram::OnTrialTimerExpired()
{
    if (!bCurrentTrialIsBenign)
    {
        // Correct: Did NOT click on hazard - advance
        AdvanceTrial();
    }
    else if (bTrialApproved)
    {
        // Correct: Approved benign and timer ran out - advance
        AdvanceTrial();
    }
    else
    {
        // Wrong: Did NOT click on benign in time - reset
        FailTrial();
    }
}

FString FAllowlistRefresherProgram::GetCurrentIcon() const
{
    if (AreAllTrialsComplete()) return TEXT("-");
    
    if (bCurrentTrialIsBenign)
    {
        // Benign items - safe symbols
        TArray<FString> Icons = {
            TEXT("O"),      // Circle
            TEXT("*"),      // Star
            TEXT("+"),      // Plus
            TEXT("#"),      // Hash
        };
        return Icons[GetCurrentTrial() % Icons.Num()];
    }
    else
    {
        // Hazard - X mark
        return TEXT("X");
    }
}

#undef LOCTEXT_NAMESPACE

void FAllowlistRefresherProgram::Approve()
{
    if (AreAllTrialsComplete()) return;
    if (bTrialApproved) return;  // Already approved this trial
    
    if (bCurrentTrialIsBenign)
    {
        // Correct: approved a benign item - mark as approved, wait for timer
        bTrialApproved = true;
    }
    else
    {
        // Wrong: approved a hazard - reset progress
        FailTrial();
    }
}

void FAllowlistRefresherProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor) return;
    
    // APPROVE button hitbox - bottom 25% of screen, center 50%
    const FVector2D Size = GetProgramSize();
    bool bInButtonY = Event.ProgramPixelPosition.Y > Size.Y * 0.75f;
    bool bInButtonX = Event.ProgramPixelPosition.X > Size.X * 0.25f && Event.ProgramPixelPosition.X < Size.X * 0.75f;
    bApproveHovered = bInButtonY && bInButtonX;
    
    UpdateCursorForState(bApprovePressed, bApproveHovered);
}

void FAllowlistRefresherProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor) return;
    ActivePointerKey = Event.TriggerKey;
    
    if (bApproveHovered)
    {
        bApprovePressed = true;
        Approve();
    }
}

void FAllowlistRefresherProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    bApprovePressed = false;
    ActivePointerKey = EKeys::Invalid;
}
