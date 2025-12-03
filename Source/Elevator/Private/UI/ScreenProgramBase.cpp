#include "UI/ScreenProgramBase.h"

#include "Styling/CoreStyle.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"

#define LOCTEXT_NAMESPACE "ScreenProgramBase"

FScreenProgramBase::FScreenProgramBase()
{
    ProgramSize = FVector2D::ZeroVector;
    ActiveStyle = FScreenProgramStyle();
    CursorOverride.Reset();
    bTaskComplete = false;
    BaseCursor = EMouseCursor::Default;
    LastPointerNormalized = FVector2D(0.5f, 0.5f);
    LastPointerPixel = FVector2D::ZeroVector;
}

TSharedRef<SWidget> FScreenProgramBase::CreateWidget(const FVector2D& Size, const FScreenProgramStyle& Style)
{
    ProgramSize = FVector2D(FMath::Max(Size.X, 1.0f), FMath::Max(Size.Y, 1.0f));
    ActiveStyle = Style;
    CursorOverride.Reset();
    BaseCursor = EMouseCursor::Default;
    LastPointerNormalized = FVector2D(0.5f, 0.5f);
    LastPointerPixel = FVector2D::ZeroVector;

    TSharedRef<SWidget> Content = BuildProgramWidget();
    TSharedRef<SWidget> Wrapped = WrapWithCompletionOverlay(Content);
    RootWidget = Wrapped;

    // Register tick handler
    Wrapped->RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &FScreenProgramBase::HandleTick));

    HandleScreenResized(ProgramSize);
    return Wrapped;
}

void FScreenProgramBase::OnTick(float DeltaTime)
{
}

EActiveTimerReturnType FScreenProgramBase::HandleTick(double InCurrentTime, float InDeltaTime)
{
    OnTick(InDeltaTime);
    return EActiveTimerReturnType::Continue;
}

void FScreenProgramBase::OnPointerMoved(const FScreenPointerEvent& Event)
{
    LastPointerNormalized = Event.ProgramNormalizedPosition;
    LastPointerPixel = Event.ProgramPixelPosition;

    const bool bHandled = PreHandlePointerMoved(Event);
    HandlePointerMoved(Event, bHandled);
}

void FScreenProgramBase::OnPointerPressed(const FScreenPointerEvent& Event)
{
    LastPointerNormalized = Event.ProgramNormalizedPosition;
    LastPointerPixel = Event.ProgramPixelPosition;

    const bool bHandled = PreHandlePointerPressed(Event);
    HandlePointerPressed(Event, bHandled);
}

void FScreenProgramBase::OnPointerReleased(const FScreenPointerEvent& Event)
{
    LastPointerNormalized = Event.ProgramNormalizedPosition;
    LastPointerPixel = Event.ProgramPixelPosition;

    const bool bHandled = PreHandlePointerReleased(Event);
    HandlePointerReleased(Event, bHandled);
}

void FScreenProgramBase::OnScreenResized(const FVector2D& NewSize)
{
    ProgramSize = FVector2D(FMath::Max(NewSize.X, 1.0f), FMath::Max(NewSize.Y, 1.0f));
    HandleScreenResized(ProgramSize);
}

EMouseCursor::Type FScreenProgramBase::GetCursorType() const
{
    if (CursorOverride.IsSet())
    {
        return CursorOverride.GetValue();
    }
    return BaseCursor;
}

bool FScreenProgramBase::PreHandlePointerMoved(const FScreenPointerEvent&)
{
    return false;
}

bool FScreenProgramBase::PreHandlePointerPressed(const FScreenPointerEvent&)
{
    return false;
}

bool FScreenProgramBase::PreHandlePointerReleased(const FScreenPointerEvent&)
{
    return false;
}

void FScreenProgramBase::HandlePointerMoved(const FScreenPointerEvent&, bool)
{
}

void FScreenProgramBase::HandlePointerPressed(const FScreenPointerEvent&, bool)
{
}

void FScreenProgramBase::HandlePointerReleased(const FScreenPointerEvent&, bool)
{
}

void FScreenProgramBase::HandleScreenResized(const FVector2D&)
{
}

void FScreenProgramBase::HandleTaskCompletionChanged(bool)
{
}

void FScreenProgramBase::SetTaskComplete(bool bCompleted)
{
    if (bTaskComplete == bCompleted)
    {
        return;
    }

    bTaskComplete = bCompleted;
    HandleTaskCompletionChanged(bTaskComplete);
}

void FScreenProgramBase::SetCursorOverride(TOptional<EMouseCursor::Type> InCursorOverride)
{
    CursorOverride = InCursorOverride;
}

FString FScreenProgramBase::BuildProgressBarString(int32 Current, int32 Total)
{
    FString Progress;
    for (int32 i = 0; i < Total; ++i)
    {
        Progress += (i < Current) ? TEXT("\u25A0") : TEXT("\u25A1");
    }
    return Progress;
}

TSharedRef<SWidget> FScreenProgramBase::BuildProgressBarWidget(TFunction<int32()> CurrentGetter, int32 Total) const
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    return SNew(STextBlock)
        .Text_Lambda([CurrentGetter, Total]() { return FText::FromString(BuildProgressBarString(CurrentGetter(), Total)); })
    .Font(MakeFont(TEXT("Regular"), Style.TextSize))
        .ColorAndOpacity(Style.GetDimColor());
}

FString FScreenProgramBase::BuildTimerBarString(float Progress, int32 Width)
{
    const int32 FilledBars = FMath::RoundToInt(Progress * Width);
    FString Bar;
    for (int32 i = 0; i < Width; ++i)
    {
        Bar += (i < FilledBars) ? TEXT("|") : TEXT(".");
    }
    return Bar;
}

TSharedRef<SWidget> FScreenProgramBase::BuildTimerBarWidget(TFunction<float()> ProgressGetter, TFunction<FSlateColor()> ColorGetter, int32 Width) const
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    
    TSharedRef<STextBlock> TimerBar = SNew(STextBlock)
        .Text_Lambda([ProgressGetter, Width]() { return FText::FromString(BuildTimerBarString(ProgressGetter(), Width)); })
        .Font(MakeFont(TEXT("Mono"), Style.TextSize + 2));
    
    if (ColorGetter)
    {
        TimerBar->SetColorAndOpacity(TAttribute<FSlateColor>::CreateLambda(ColorGetter));
    }
    else
    {
        TimerBar->SetColorAndOpacity(Style.GetPrimaryColor());
    }
    
    return TimerBar;
}

TSharedRef<SWidget> FScreenProgramBase::BuildHeader(const FText& Title) const
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(Style.GetSmallPadding())
        [
            BuildTitleWidget(Title, 2.0f)
        ];
}

TSharedRef<SWidget> FScreenProgramBase::BuildButton(const FText& Label, TAttribute<bool> IsHovered, TAttribute<bool> IsPressed) const
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor_Lambda([this, IsHovered, IsPressed]() {
            if (IsPressed.Get(false)) return GetProgramStyle().GetPressedFillColor();
            if (IsHovered.Get(false)) return GetProgramStyle().GetHoveredFillColor();
            return FSlateColor(FLinearColor::Transparent);
        })
        .Padding(Style.GetPadding())
        [
            SNew(STextBlock)
            .Text(Label)
            .Font(MakeScaledFont(TEXT("Bold"), 1.5f))
            .ColorAndOpacity_Lambda([this, IsHovered, IsPressed]() {
                if (IsPressed.Get(false) || IsHovered.Get(false)) return GetProgramStyle().GetActiveTextColor();
                return GetProgramStyle().GetDimColor();
            })
            .Justification(ETextJustify::Center)
        ];
}

TSharedRef<SWidget> FScreenProgramBase::BuildCard(TSharedRef<SWidget> Content) const
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    return SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor::Transparent)
        .Padding(Style.GetPadding())
        [
            Content
        ];
}

void FScreenProgramBase::SetCompletionText(const FText& InText)
{
    CompletionTextOverride = InText;
    bHasCompletionTextOverride = true;
}

FText FScreenProgramBase::GetCompletionText() const
{
    if (bHasCompletionTextOverride)
    {
        return CompletionTextOverride;
    }

    return LOCTEXT("DefaultCompletionText", "TASK COMPLETE");
}

bool FScreenProgramBase::ShouldShowCompletionOverlay() const
{
    return IsTaskComplete();
}

TSharedRef<SWidget> FScreenProgramBase::BuildCompletionOverlay() const
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const FLinearColor CompletionGreen(0.0f, 0.82f, 0.0f, 1.0f);

    return SNew(SBorder)
        .Visibility_Lambda([this]() { return ShouldShowCompletionOverlay() ? EVisibility::Visible : EVisibility::Collapsed; })
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor::Black)
        .Padding(0.0f)
        [
            SNew(SOverlay)
            + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(CompletionGreen)
                .Padding(FMargin(Style.GetLargePadding()))
                [
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(FLinearColor::Black)
                    .Padding(FMargin(Style.GetLargePadding()))
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this]() { return GetCompletionText(); })
                        .Font(MakeScaledFont(TEXT("Bold"), 2.2f))
                        .Justification(ETextJustify::Center)
                        .ColorAndOpacity(CompletionGreen)
                        .WrapTextAt(Style.GetTitleBarHeight() * 6.0f)
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> FScreenProgramBase::WrapWithCompletionOverlay(TSharedRef<SWidget> InContent) const
{
    return SNew(SOverlay)
        + SOverlay::Slot()
        [
            InContent
        ]
        + SOverlay::Slot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            BuildCompletionOverlay()
        ];
}
FSlateFontInfo FScreenProgramBase::MakeFont(const FString& Typeface, int32 Size) const
{
    const int32 ClampedSize = FMath::Max(Size, 1);
    return FCoreStyle::GetDefaultFontStyle(FName(*Typeface), ClampedSize);
}

FSlateFontInfo FScreenProgramBase::MakeScaledFont(const FString& Typeface, float SizeMultiplier) const
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const float BaseSize = static_cast<float>(FMath::Max(Style.TextSize, 1));
    const int32 ComputedSize = FMath::Max(1, FMath::RoundToInt(BaseSize * SizeMultiplier));
    return MakeFont(Typeface, ComputedSize);
}

TSharedRef<STextBlock> FScreenProgramBase::BuildTitleWidget(const FText& TitleText, float SizeMultiplier) const
{
    return SNew(STextBlock)
        .Text(TitleText)
        .Font(MakeScaledFont(TEXT("Bold"), SizeMultiplier))
        .ColorAndOpacity(GetProgramStyle().GetPrimaryColor())
        .Justification(ETextJustify::Center);
}

TSharedRef<STextBlock> FScreenProgramBase::BuildSectionLabel(const FText& LabelText, float SizeMultiplier) const
{
    return SNew(STextBlock)
        .Text(LabelText)
        .Font(MakeScaledFont(TEXT("Bold"), SizeMultiplier))
        .ColorAndOpacity(GetProgramStyle().GetPrimaryColor());
}

TSharedRef<STextBlock> FScreenProgramBase::BuildStyledText(const FText& Text, const FString& Typeface, float SizeMultiplier, TFunction<FSlateColor()> ColorGetter, TOptional<ETextJustify::Type> Justification) const
{
    const FSlateFontInfo Font = MakeScaledFont(Typeface, SizeMultiplier);
    TSharedRef<STextBlock> Label = SNew(STextBlock)
        .Text(Text)
        .Font(Font);

    if (ColorGetter)
    {
        Label->SetColorAndOpacity(TAttribute<FSlateColor>::CreateLambda(ColorGetter));
    }
    else
    {
        Label->SetColorAndOpacity(GetProgramStyle().GetPrimaryColor());
    }

    if (Justification.IsSet())
    {
        Label->SetJustification(Justification.GetValue());
    }

    return Label;
}

#undef LOCTEXT_NAMESPACE
