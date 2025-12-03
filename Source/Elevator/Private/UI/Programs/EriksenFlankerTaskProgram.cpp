#include "UI/Programs/EriksenFlankerTaskProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

REGISTER_SCREEN_PROGRAM(FEriksenFlankerTaskProgram, "eriksen_flanker_task")

FEriksenFlankerTaskProgram::FEriksenFlankerTaskProgram()
    : FTrialProgramBase(12)
{
    SetTrialTitle(FText::FromString(TEXT("Eriksen Flanker Task")));
    GenerateNewTrial();
}

void FEriksenFlankerTaskProgram::GenerateNewTrial()
{
    // Store previous state
    bool bPrevTargetLeft = bTargetLeft;
    bool bPrevCongruent = bCongruent;
    int32 PrevTargetPosition = TargetPosition;
    
    // Generate new trial - ensure at least one thing changes
    do {
        bTargetLeft = FMath::RandBool();
        bCongruent = FMath::RandRange(0, 100) < 50;
        TargetPosition = FMath::RandRange(0, 4);
    } while (bTargetLeft == bPrevTargetLeft && bCongruent == bPrevCongruent && TargetPosition == PrevTargetPosition && GetCurrentTrial() > 0);
}

void FEriksenFlankerTaskProgram::SelectResponse(bool bLeft)
{
    bool bCorrect = (bLeft == bTargetLeft);
    if (bCorrect)
    {
        AdvanceTrial();
    }
    else
    {
        FailTrial();
    }
}

TSharedRef<SWidget> FEriksenFlankerTaskProgram::BuildTrialBody()
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const int32 LargeTextSize = Style.TextSize * 2;

    return SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(SVerticalBox)
            // Signal bus display
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [SNew(STextBlock).Text(FText::FromString(TEXT("SIGNAL BUS")))
                .Font(FCoreStyle::GetDefaultFontStyle("Mono", Style.TextSize)).ColorAndOpacity(Style.GetDimColor())]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, Style.GetSmallPadding())
            [SNew(STextBlock).Text_Lambda([this]() {
                TCHAR TargetArrow = bTargetLeft ? TEXT('<') : TEXT('>');
                TCHAR FlankerArrow = bCongruent ? TargetArrow : (bTargetLeft ? TEXT('>') : TEXT('<'));
                FString Display;
                for (int32 i = 0; i < 5; ++i)
                {
                    const bool bIsTarget = (i == TargetPosition);
                    const TCHAR Arrow = bIsTarget ? TargetArrow : FlankerArrow;
                    Display += FString::Printf(TEXT(" %c "), Arrow);
                }
                return FText::FromString(Display);
            }).Font(FCoreStyle::GetDefaultFontStyle("Mono", LargeTextSize)).ColorAndOpacity(Style.GetPrimaryColor())]
        ];
}

TSharedRef<SWidget> FEriksenFlankerTaskProgram::BuildTrialFooter()
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const int32 LargeTextSize = Style.TextSize * 2;

    return SNew(SHorizontalBox)
        // Left port
        + SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [SNew(STextBlock).Text(FText::FromString(TEXT("PORT A")))
                .Font(FCoreStyle::GetDefaultFontStyle("Mono", Style.TextSize)).ColorAndOpacity(Style.GetDimColor())]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [SNew(STextBlock).Text(FText::FromString(TEXT("<<<")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", LargeTextSize * 2))
                .ColorAndOpacity_Lambda([this]() { 
                    return HoveredSocket == 0 ? GetProgramStyle().GetPrimaryColor() : GetProgramStyle().GetDimColor(); 
                })]]
        // Right port
        + SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [SNew(STextBlock).Text(FText::FromString(TEXT("PORT B")))
                .Font(FCoreStyle::GetDefaultFontStyle("Mono", Style.TextSize)).ColorAndOpacity(Style.GetDimColor())]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [SNew(STextBlock).Text(FText::FromString(TEXT(">>>")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", LargeTextSize * 2))
                .ColorAndOpacity_Lambda([this]() { 
                    return HoveredSocket == 1 ? GetProgramStyle().GetPrimaryColor() : GetProgramStyle().GetDimColor(); 
                })]];
}

void FEriksenFlankerTaskProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor) return;
    
    const FVector2D Size = GetProgramSize();
    const FVector2D& Pos = Event.ProgramPixelPosition;
    
    // Buttons are in the bottom half, split left/right
    HoveredSocket = -1;
    if (Pos.Y > Size.Y * 0.5f)
    {
        if (Pos.X < Size.X * 0.5f)
            HoveredSocket = 0;
        else
            HoveredSocket = 1;
    }
    
    UpdateCursorForState(false, HoveredSocket >= 0);
}

void FEriksenFlankerTaskProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor) return;
    ActivePointerKey = Event.TriggerKey;
    
    if (HoveredSocket >= 0)
    {
        SelectResponse(HoveredSocket == 0);
    }
}

void FEriksenFlankerTaskProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    ActivePointerKey = EKeys::Invalid;
}
