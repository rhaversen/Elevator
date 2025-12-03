#include "UI/Programs/DebugTaskProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

REGISTER_SCREEN_PROGRAM(FDebugTaskProgram, "debug")

FDebugTaskProgram::FDebugTaskProgram()
{
}

TSharedRef<SWidget> FDebugTaskProgram::BuildProgramWidget()
{
    const FScreenProgramStyle& Style = GetProgramStyle();

    return SNew(SVerticalBox)
        // Header
        + SVerticalBox::Slot().AutoHeight()
        [
            BuildHeader(FText::FromString(TEXT("Debug Task")))
        ]
        // Body with button
        + SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(Style.GetLargePadding())
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Click the button below to complete the task")))
                .Font(MakeScaledFont("Regular", 1.0f))
                .ColorAndOpacity(Style.GetDefaultTextColor())
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(Style.GetLargePadding())
            [
                BuildButton(
                    FText::FromString(TEXT("COMPLETE TASK")),
                    TAttribute<bool>::CreateLambda([this]() { return bButtonHovered; }),
                    TAttribute<bool>::CreateLambda([this]() { return bButtonPressed; })
                )
            ]
        ];
}

void FDebugTaskProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    // Simple hover detection - in a real implementation you'd hit-test the button
    // For now, just always show as hovered when pointer is in center area
    bButtonHovered = !GetTaskComplete();
    UpdateCursorForState(bButtonPressed, bButtonHovered);
}

void FDebugTaskProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (GetTaskComplete()) return;
    
    ActivePointerKey = Event.TriggerKey;
    bButtonPressed = true;
}

void FDebugTaskProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (Event.TriggerKey != ActivePointerKey) return;
    
    if (bButtonPressed)
    {
        bButtonPressed = false;
        ActivePointerKey = EKeys::Invalid;
        SetTaskComplete(true);
    }
}
