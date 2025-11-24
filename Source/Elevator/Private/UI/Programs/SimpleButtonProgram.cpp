#include "UI/Programs/SimpleButtonProgram.h"

#include "UI/ScreenProgramIds.h"
#include "UI/ScreenProgramRegistry.h"
#include "UI/SlateWidgetHelpers.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	struct FSimpleButtonProgramRegistrar
	{
		FSimpleButtonProgramRegistrar()
		{
			FScreenProgramRegistry::Get().RegisterProgram(
				ScreenProgramIds::SimpleButton,
				[]() { return MakeShared<FSimpleButtonProgram>(); });
		}
	};

	static FSimpleButtonProgramRegistrar GSimpleButtonProgramRegistrar;
}

const FName FSimpleButtonProgram::PrimaryWindowId(TEXT("PrimaryWindow"));

FSimpleButtonProgram::FSimpleButtonProgram()
	: ActiveButtonPointerKey(EKeys::Invalid)
{
}

void FSimpleButtonProgram::BuildWindowLayout(FScreenProgramWindowBuilder& Builder)
{
	FScreenProgramWindowConfig Config;
	Config.WindowId = PrimaryWindowId;
	Config.Title = FText::FromString(TEXT("My Computer"));
	Config.InitialPosition = FVector2D(400.0f, 300.0f);
	Config.InitialSize = FVector2D(600.0f, 400.0f);
	Config.MinimumSize = FVector2D(MinWindowSize, MinWindowSize);
	Config.TitleBarHeight = TitleBarHeight;
	Config.ResizeHandleSize = ResizeHandleSize;

	FScreenProgramWindowChrome Chrome;
	Chrome.BorderColor = FLinearColor::Green;
	Chrome.BackgroundColor = FLinearColor::Black;
	Chrome.TitleBarColor = FLinearColor::Black;
	Chrome.TitleTextColor = FLinearColor::Green;
	Chrome.ContentPadding = FMargin(10.0f);

	Builder.AddWindow(
		Config,
		Chrome,
		[this](const FScreenProgramWindowConfig& WindowConfig)
		{
			return BuildPrimaryWindowContent(WindowConfig);
		});
}

void FSimpleButtonProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByChrome)
{
	if (GetTaskComplete())
	{
		bCompleteButtonHovered = false;
	}
	else
	{
		bCompleteButtonHovered = !bHandledByChrome && IsCursorOverCompleteButton(Event.ProgramPixelPosition);
	}

	UpdateCursorVisual();
}

void FSimpleButtonProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByChrome)
{
	if (bHandledByChrome)
	{
		bCompleteButtonPressed = false;
		UpdateCursorVisual();
		return;
	}

	if (GetTaskComplete())
	{
		bCompleteButtonPressed = false;
		UpdateCursorVisual();
		return;
	}

	if (IsCursorOverCompleteButton(Event.ProgramPixelPosition))
	{
		bCompleteButtonPressed = true;
		ActiveButtonPointerKey = Event.TriggerKey;
	}
	else
	{
		bCompleteButtonPressed = false;
		ActiveButtonPointerKey = EKeys::Invalid;
	}

	UpdateCursorVisual();
}

void FSimpleButtonProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByChrome)
{
	const bool bMatchesPointer = !ActiveButtonPointerKey.IsValid() || !Event.TriggerKey.IsValid() || ActiveButtonPointerKey == Event.TriggerKey;
	const bool bWasPressed = bCompleteButtonPressed;

	bCompleteButtonPressed = false;
	ActiveButtonPointerKey = EKeys::Invalid;

	if (!GetTaskComplete() && !bHandledByChrome && bMatchesPointer && bWasPressed && IsCursorOverCompleteButton(Event.ProgramPixelPosition))
	{
		SetTaskComplete(true);
	}

	if (GetTaskComplete())
	{
		bCompleteButtonHovered = false;
	}
	else
	{
		bCompleteButtonHovered = !bHandledByChrome && IsCursorOverCompleteButton(Event.ProgramPixelPosition);
	}

	UpdateCursorVisual();
}

void FSimpleButtonProgram::HandleScreenResized(const FVector2D& NewSize)
{
}

void FSimpleButtonProgram::HandleTaskCompletionChanged(bool bCompleted)
{
	bCompleteButtonHovered = false;
	bCompleteButtonPressed = false;
	UpdateCursorVisual();
}

TSharedRef<SWidget> FSimpleButtonProgram::BuildPrimaryWindowContent(const FScreenProgramWindowConfig& WindowConfig)
{
	const FScreenProgramStyle& Style = GetProgramStyle();
	const FLinearColor Green = FLinearColor::Green;

	return SNew(SOverlay)
		// Instructions text
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Welcome to Mock OS v0.1\n\n- Drag title bar to move\n- Drag bottom-right to resize\n- Click Complete to finish task")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", Style.TextSize))
			.ColorAndOpacity(Green)
		]
		// Complete button in center
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SlateWidgetHelpers::CreateOutlinedButton(
				FVector2D(CompleteButtonWidth, CompleteButtonHeight),
				FText::FromString(TEXT("Complete")),
				Style.TextSize,
				Style.LineThickness,
				TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateSP(this, &FSimpleButtonProgram::GetCompleteButtonBorderColor)),
				TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateSP(this, &FSimpleButtonProgram::GetCompleteButtonFillColor)),
				TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateSP(this, &FSimpleButtonProgram::GetCompleteButtonTextColor)))
		];
}

bool FSimpleButtonProgram::IsCursorOverCompleteButton(const FVector2D& ProgramPixel) const
{
	FScreenProgramWindowMetrics Metrics;
	if (!TryGetWindowMetrics(PrimaryWindowId, Metrics))
	{
		return false;
	}

	const FVector2D WindowPos = Metrics.Position;
	const FVector2D WindowSize = Metrics.Size;
	const FScreenProgramStyle& Style = GetProgramStyle();

	const FVector2D ContentPos = WindowPos + FVector2D(Style.LineThickness, Style.LineThickness + Metrics.TitleBarHeight + Style.LineThickness);
	const FVector2D ContentSize = FVector2D(
		FMath::Max(WindowSize.X - Style.LineThickness * 2.0f, 0.0f),
		FMath::Max(WindowSize.Y - (Metrics.TitleBarHeight + Style.LineThickness * 2.0f), 0.0f));

	const FVector2D ButtonPos(
		ContentPos.X + (ContentSize.X - CompleteButtonWidth) * 0.5f,
		ContentPos.Y + (ContentSize.Y - CompleteButtonHeight) * 0.5f);

	const FBox2D ButtonRect(ButtonPos, ButtonPos + FVector2D(CompleteButtonWidth, CompleteButtonHeight));
	return ButtonRect.IsInside(ProgramPixel);
}

void FSimpleButtonProgram::UpdateCursorVisual()
{
	if (GetTaskComplete())
	{
		SetCursorOverride(TOptional<EMouseCursor::Type>());
		return;
	}

	if (bCompleteButtonPressed)
	{
		SetCursorOverride(EMouseCursor::GrabHand);
	}
	else if (bCompleteButtonHovered)
	{
		SetCursorOverride(EMouseCursor::Hand);
	}
	else
	{
		SetCursorOverride(TOptional<EMouseCursor::Type>());
	}
}

FSlateColor FSimpleButtonProgram::GetCompleteButtonBorderColor() const
{
	return FSlateColor(FLinearColor::Green);
}

FSlateColor FSimpleButtonProgram::GetCompleteButtonFillColor() const
{
	if (GetTaskComplete())
	{
		return FSlateColor(FLinearColor::Transparent);
	}
	return bCompleteButtonHovered ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Transparent);
}

FSlateColor FSimpleButtonProgram::GetCompleteButtonTextColor() const
{
	if (GetTaskComplete())
	{
		return FSlateColor(FLinearColor::Green);
	}
	return bCompleteButtonHovered ? FSlateColor(FLinearColor::Black) : FSlateColor(FLinearColor::Green);
}
