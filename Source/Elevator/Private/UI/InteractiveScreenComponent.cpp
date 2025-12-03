#include "UI/InteractiveScreenComponent.h"

#include "UI/IScreenProgram.h"
#include "UI/SlateWidgetHelpers.h"
#include "System/ElevatorGameManagerSubsystem.h"

#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/DateTime.h"
#include "Logging/LogMacros.h"
#include "TimerManager.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

class SBootAnimationWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SBootAnimationWidget)
        : _TextSize(16)
    {}
        SLATE_ARGUMENT(int32, TextSize)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        const int32 ResolvedTextSize = FMath::Max(8, InArgs._TextSize);
        FontInfo = FCoreStyle::GetDefaultFontStyle(TEXT("Mono"), ResolvedTextSize);

        ChildSlot
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush(TEXT("NoBrush")))
            .Padding(FMargin(0.0f))
            [
                SAssignNew(ScrollBox, SScrollBox)
                .ScrollBarVisibility(EVisibility::Collapsed)
                + SScrollBox::Slot()
                [
                    SAssignNew(LineContainer, SVerticalBox)
                ]
            ]
        ];
    }

    void Reset()
    {
        if (LineContainer.IsValid())
        {
            LineContainer->ClearChildren();
        }

        if (ScrollBox.IsValid())
        {
            ScrollBox->ScrollToStart();
        }
    }

    void AppendLine(const FString& Line)
    {
        if (!LineContainer.IsValid())
        {
            return;
        }

        LineContainer->AddSlot()
        .AutoHeight()
        .Padding(FMargin(0.0f, 4.0f))
        [
            SNew(STextBlock)
            .Text(FText::FromString(Line))
            .Font(FontInfo)
            .ColorAndOpacity(FSlateColor(FLinearColor::Green))
        ];

        if (ScrollBox.IsValid())
        {
            ScrollBox->ScrollToEnd();
        }
    }

private:
    TSharedPtr<SScrollBox> ScrollBox;
    TSharedPtr<SVerticalBox> LineContainer;
    FSlateFontInfo FontInfo;
};

namespace
{
    constexpr float ProgramPaddingX = 0.0f;
    constexpr float ProgramPaddingTop = 0.0f;
    constexpr float ProgramPaddingBottom = 24.0f;
    constexpr float ProgramContentPadding = 24.0f;
    constexpr float FooterBarHeight = 96.0f;
    constexpr float FooterTextSpacing = 24.0f;
    constexpr float FooterHorizontalPadding = 48.0f;
    constexpr float FooterButtonSpacing = 12.0f;
    const FVector2D ExitButtonSize(180.0f, 60.0f);
    const FVector2D NavigationButtonSize(160.0f, 60.0f);
}

UInteractiveScreenComponent::UInteractiveScreenComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    BootMessages = {
        TEXT("init: shadow kernel remap OK"),
        TEXT("pci: probing ghost bridge @ 0x00d4"),
        TEXT("mmu: remapping stale office segment"),
        TEXT("cryptd: seeded entropy pool from /dev/random"),
        TEXT("systemd[1]: mounting /var/log/mirror"),
        TEXT("audit: service ghost@tunnel.service queued"),
        TEXT("daemon.try: handshake with //terminal/29 accepted"),
        TEXT("rtc: calibrating office-cycle oscillator"),
        TEXT("mapper: phantom-volume mapped at 0x7ffe1200"),
        TEXT("kernel: dram ECC scrub pass 1 complete"),
        TEXT("net.ifup: waiting on uplink (eth0) ..."),
        TEXT("boot-notify: stale session traces recovered"),
        TEXT("watch: stray process 'mirror-ghost' acknowledged"),
        TEXT("audit: integrity of /etc/tasks.d verified"),
        TEXT("displayd: binding RT_ScreenInterface to surface"),
        TEXT("init: handing off control to workstation session")
    };
}

UInteractiveScreenComponent::~UInteractiveScreenComponent()
{
}

void UInteractiveScreenComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeScreen();
}

void UInteractiveScreenComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    if (CurrentProgram.IsValid())
    {
        DispatchSyntheticPointerReleases();
        CurrentProgram->OnDeactivated();
    }

    StopBootSequence();
    PendingProgram.Reset();
    BootWidget.Reset();

    ClearPointerState();
    SlateWidgetRenderer.Reset();
    RootWidget.Reset();
    ProgramWidget.Reset();
    ProgramContainer.Reset();
    CurrentProgram.Reset();
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UInteractiveScreenComponent::SetRenderTarget(UTextureRenderTarget2D* InRenderTarget)
{
    ScreenRenderTarget = InRenderTarget;
    bWidgetInitialized = false;
}

void UInteractiveScreenComponent::InitializeScreen()
{
    UpdateWidgetSizeFromRenderTarget();
    UpdateCursorInternal(VirtualCursorPosition);
    RefreshRender();
}

void UInteractiveScreenComponent::ResetCursor(const FVector2D& NormalizedPosition)
{
    DispatchSyntheticPointerReleases();
    ClearPointerState();
    UpdateCursorInternal(NormalizedPosition);
    RefreshRender();
    UpdateHardwareCursor();
}

void UInteractiveScreenComponent::UpdateCursor(const FVector2D& NormalizedPosition)
{
    UpdateCursorInternal(NormalizedPosition);
    RefreshRender();
}

void UInteractiveScreenComponent::SetProgram(TSharedPtr<IScreenProgram> InProgram)
{
    if (CurrentProgram.IsValid())
    {
        DispatchSyntheticPointerReleases();
        CurrentProgram->OnDeactivated();
    }

    StopBootSequence();

    CurrentProgram.Reset();
    PendingProgram = InProgram;
    ProgramWidget.Reset();
    BootWidget.Reset();
    bWidgetInitialized = false;
    bExitRequested = false;
    bExitButtonHovered = false;
    NextBootLineIndex = 0;
    ClearPointerState();
    CachedCursorType = EMouseCursor::Default;

    SetDisplayState(EDisplayState::Idle);

    UpdateWidgetSizeFromRenderTarget();
    UpdateCursorInternal(VirtualCursorPosition);
    RefreshRender();
    UpdateHardwareCursor();
}

void UInteractiveScreenComponent::ActivatePendingProgram(bool bShouldBoot)
{
    if (DisplayState == EDisplayState::Booting || DisplayState == EDisplayState::ShowingProgram)
    {
        RefreshRender();
        return;
    }

    if (!PendingProgram.IsValid())
    {
        return;
    }

    CurrentProgram = PendingProgram;
    PendingProgram.Reset();

    bExitRequested = false;
    ClearPointerState();

    const bool bPlayBoot = bShouldBoot && BootMessages.Num() > 0;
    SetDisplayState(bPlayBoot ? EDisplayState::Booting : EDisplayState::ShowingProgram);

    ProgramWidget.Reset();

    CreateInterfaceIfNeeded();
    UpdateWidgetSizeFromRenderTarget();

    if (CurrentProgram.IsValid())
    {
        if (DisplayState == EDisplayState::ShowingProgram)
        {
            CurrentProgram->OnScreenResized(ProgramAreaSize);
        }
        CurrentProgram->OnActivated();
    }

    if (DisplayState == EDisplayState::Booting)
    {
        BeginBootSequence();
    }
    else
    {
        UpdateProgramContent();
        RefreshRender();
    }

    UpdateHardwareCursor();
}

void UInteractiveScreenComponent::ProcessPointerPressed(const FKey& PointerKey)
{
    const FVector2D PixelPos(
        VirtualCursorPosition.X * WidgetSize.X,
        VirtualCursorPosition.Y * WidgetSize.Y);

    ActivePointerButtons.Add(PointerKey);

    if (PrevTaskButtonRect.bIsValid && PrevTaskButtonRect.IsInside(PixelPos))
    {
        bPrevTaskButtonPressed = true;
        RefreshRender();
        UpdateHardwareCursor();
        return;
    }

    if (NextTaskButtonRect.bIsValid && NextTaskButtonRect.IsInside(PixelPos))
    {
        bNextTaskButtonPressed = true;
        RefreshRender();
        UpdateHardwareCursor();
        return;
    }

    if (ExitButtonRect.bIsValid && ExitButtonRect.IsInside(PixelPos))
    {
        bExitButtonPressed = true;
        RefreshRender();
        UpdateHardwareCursor();
        return;
    }

    if (DisplayState != EDisplayState::ShowingProgram || !CurrentProgram.IsValid() || !ProgramWidget.IsValid())
    {
        RefreshRender();
        UpdateHardwareCursor();
        return;
    }

    const FVector2D ProgramCursor = ConvertPixelToProgramNormalized(PixelPos);
    const FScreenPointerEvent Event = BuildPointerEvent(ProgramCursor, PixelPos, PointerKey);
    CurrentProgram->OnPointerPressed(Event);
    RefreshRender();
    UpdateHardwareCursor();
}

void UInteractiveScreenComponent::ProcessPointerReleased(const FKey& PointerKey)
{
    const FVector2D PixelPos(
        VirtualCursorPosition.X * WidgetSize.X,
        VirtualCursorPosition.Y * WidgetSize.Y);

    const bool bWasPressedOnPrev = bPrevTaskButtonPressed;
    const bool bWasPressedOnNext = bNextTaskButtonPressed;
    const bool bWasPressedOnExit = bExitButtonPressed;
    const bool bIsReleasingOverPrev = PrevTaskButtonRect.bIsValid && PrevTaskButtonRect.IsInside(PixelPos);
    const bool bIsReleasingOverNext = NextTaskButtonRect.bIsValid && NextTaskButtonRect.IsInside(PixelPos);
    const bool bIsReleasingOverExit = ExitButtonRect.bIsValid && ExitButtonRect.IsInside(PixelPos);

    ActivePointerButtons.Remove(PointerKey);
    bPrevTaskButtonPressed = false;
    bNextTaskButtonPressed = false;
    bExitButtonPressed = false;

    if (bWasPressedOnPrev || bWasPressedOnNext || bWasPressedOnExit)
    {
        if (bWasPressedOnPrev && bIsReleasingOverPrev)
        {
            RequestTaskNavigation(-1);
        }
        else if (bWasPressedOnNext && bIsReleasingOverNext)
        {
            RequestTaskNavigation(1);
        }
        else if (bWasPressedOnExit && bIsReleasingOverExit)
        {
            bExitRequested = true;
        }
        RefreshRender();
        UpdateHardwareCursor();
        return;
    }

    if (DisplayState != EDisplayState::ShowingProgram || !CurrentProgram.IsValid() || !ProgramWidget.IsValid())
    {
        RefreshRender();
        UpdateHardwareCursor();
        return;
    }

    const FVector2D ProgramCursor = ConvertPixelToProgramNormalized(PixelPos);
    const FScreenPointerEvent Event = BuildPointerEvent(ProgramCursor, PixelPos, PointerKey);
    CurrentProgram->OnPointerReleased(Event);
    RefreshRender();
    UpdateHardwareCursor();
}

bool UInteractiveScreenComponent::ShouldExit()
{
    if (bExitRequested)
    {
        bExitRequested = false;
        ClearPointerState();
        return true;
    }

    return false;
}

void UInteractiveScreenComponent::RefreshRender()
{
    if (!ScreenRenderTarget)
    {
        return;
    }

    UpdateWidgetSizeFromRenderTarget();

    if (DisplayState == EDisplayState::Idle || !RootWidget.IsValid())
    {
        UKismetRenderingLibrary::ClearRenderTarget2D(this, ScreenRenderTarget, FLinearColor::Black);
        return;
    }

    if (!IsReady())
    {
        return;
    }

    EnsureRenderer();

    ScreenRenderTarget->AddressX = TA_Clamp;
    ScreenRenderTarget->AddressY = TA_Clamp;

    UKismetRenderingLibrary::ClearRenderTarget2D(this, ScreenRenderTarget, FLinearColor::Black);
    SlateWidgetRenderer->DrawWidget(
        ScreenRenderTarget,
        RootWidget.ToSharedRef(),
        WidgetSize,
        0.0f,
        false);
}

bool UInteractiveScreenComponent::IsProgramTaskComplete() const
{
    return CurrentProgram.IsValid() && CurrentProgram->IsTaskComplete();
}

bool UInteractiveScreenComponent::IsReady() const
{
    return ScreenRenderTarget != nullptr && RootWidget.IsValid() && WidgetSize.X > 0.0f && WidgetSize.Y > 0.0f;
}

void UInteractiveScreenComponent::EnsureRenderer()
{
    if (!SlateWidgetRenderer.IsValid())
    {
        SlateWidgetRenderer = MakeUnique<FWidgetRenderer>(true, false);
    }
}

void UInteractiveScreenComponent::CreateInterfaceIfNeeded()
{
    if (RootWidget.IsValid())
    {
        return;
    }

    ProgramContainer.Reset();
    ProgramRootWidget.Reset();
    BootContainer.Reset();

    const FSlateBrush* NoBrush = FCoreStyle::Get().GetBrush("NoBrush");

    TSharedRef<SOverlay> Root = SNew(SOverlay)
        // Boot overlay layer
        + SOverlay::Slot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SAssignNew(BootContainer, SBorder)
            .BorderImage(NoBrush)
            .BorderBackgroundColor(FLinearColor::Black)
            .Padding(FMargin(ProgramContentPadding))
            [
                SNullWidget::NullWidget
            ]
        ]
        // Background + program chrome and footer
        + SOverlay::Slot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SAssignNew(ProgramRootWidget, SVerticalBox)
            + SVerticalBox::Slot()
            .Padding(FMargin(ProgramPaddingX, ProgramPaddingTop, ProgramPaddingX, ProgramPaddingBottom))
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            [
                SNew(SBorder)
                .BorderImage(NoBrush)
                .BorderBackgroundColor(FLinearColor::Black)
                .Padding(FMargin(ProgramContentPadding))
                [
                    SAssignNew(ProgramContainer, SBox)
                    .WidthOverride(ProgramAreaSize.X)
                    .HeightOverride(ProgramAreaSize.Y)
                    [
                        SNullWidget::NullWidget
                    ]
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBox)
                .HeightOverride(LineThickness)
                [
                    SNew(SImage)
                    .Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .ColorAndOpacity(FLinearColor::Green)
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBox)
                .HeightOverride(FooterBarHeight)
                [
                    SNew(SBorder)
                    .BorderImage(NoBrush)
                    .BorderBackgroundColor(FLinearColor::Black)
                    .Padding(FMargin(FooterHorizontalPadding, 16.0f, FooterHorizontalPadding, 16.0f))
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this]() { return GetFooterWeekdayText(); })
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
                            .ColorAndOpacity(FSlateColor(FLinearColor::Green))
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(FMargin(FooterTextSpacing, 0.0f, 0.0f, 0.0f))
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this]() { return GetFooterDateText(); })
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
                            .ColorAndOpacity(FSlateColor(FLinearColor::Green))
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(FMargin(FooterTextSpacing, 0.0f, 0.0f, 0.0f))
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this]() { return GetFooterTimeText(); })
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", TextSize))
                            .ColorAndOpacity(FSlateColor(FLinearColor::Green))
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(FMargin(FooterTextSpacing, 0.0f, 0.0f, 0.0f))
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this]() { return GetFooterTaskStatusText(); })
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", TextSize))
                            .ColorAndOpacity_Lambda([this]() { return GetFooterTaskStatusColor(); })
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(SSpacer)
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SlateWidgetHelpers::CreateOutlinedButton(
                                NavigationButtonSize,
                                FText::FromString(TEXT("Prev Task")),
                                TextSize,
                                LineThickness,
                                TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UInteractiveScreenComponent::GetPrevTaskButtonBorderColor)),
                                TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UInteractiveScreenComponent::GetPrevTaskButtonFillColor)),
                                TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UInteractiveScreenComponent::GetPrevTaskButtonTextColor)))
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(FMargin(FooterButtonSpacing, 0.0f, 0.0f, 0.0f))
                        [
                            SlateWidgetHelpers::CreateOutlinedButton(
                                NavigationButtonSize,
                                FText::FromString(TEXT("Next Task")),
                                TextSize,
                                LineThickness,
                                TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UInteractiveScreenComponent::GetNextTaskButtonBorderColor)),
                                TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UInteractiveScreenComponent::GetNextTaskButtonFillColor)),
                                TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UInteractiveScreenComponent::GetNextTaskButtonTextColor)))
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(FMargin(FooterButtonSpacing, 0.0f, 0.0f, 0.0f))
                        [
                            SlateWidgetHelpers::CreateOutlinedButton(
                                ExitButtonSize,
                                FText::FromString(TEXT("Exit")),
                                TextSize,
                                LineThickness,
                                TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UInteractiveScreenComponent::GetExitButtonBorderColor)),
                                TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UInteractiveScreenComponent::GetExitButtonFillColor)),
                                TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UInteractiveScreenComponent::GetExitButtonTextColor)))
                        ]
                    ]
                ]
            ]
        ];

    RootWidget = Root;
    SetDisplayState(DisplayState);
}

void UInteractiveScreenComponent::UpdateWidgetSizeFromRenderTarget()
{
    if (!ScreenRenderTarget)
    {
        return;
    }

    const FVector2D Size(ScreenRenderTarget->SizeX, ScreenRenderTarget->SizeY);
    if (Size.X <= 0.0f || Size.Y <= 0.0f)
    {
        return;
    }

    const bool bSizeChanged = !Size.Equals(WidgetSize);
    WidgetSize = Size;

    CreateInterfaceIfNeeded();

    if (!RootWidget.IsValid())
    {
        return;
    }

    if (!bWidgetInitialized || bSizeChanged)
    {
        UpdateLayout(Size);
        bWidgetInitialized = true;
        UpdateCursorInternal(VirtualCursorPosition);
    }
    else if (DisplayState == EDisplayState::ShowingProgram && CurrentProgram.IsValid() && !ProgramWidget.IsValid())
    {
        UpdateProgramContent();
    }
}

void UInteractiveScreenComponent::UpdateProgramContent()
{
    if (!ProgramContainer.IsValid())
    {
        return;
    }

    if (DisplayState == EDisplayState::Booting)
    {
        if (BootWidget.IsValid())
        {
            ProgramContainer->SetContent(BootWidget.ToSharedRef());
        }
        else
        {
            ProgramContainer->SetContent(SNullWidget::NullWidget);
        }
        return;
    }

    if (DisplayState != EDisplayState::ShowingProgram)
    {
        ProgramWidget.Reset();
        ProgramContainer->SetContent(SNullWidget::NullWidget);
        return;
    }

    if (!CurrentProgram.IsValid())
    {
        ProgramWidget.Reset();
        ProgramContainer->SetContent(SNullWidget::NullWidget);
        return;
    }

    CurrentProgram->OnScreenResized(ProgramAreaSize);
    
    FScreenProgramStyle Style(LineThickness, TextSize);
    ProgramWidget = CurrentProgram->CreateWidget(ProgramAreaSize, Style);

    if (ProgramWidget.IsValid())
    {
        ProgramContainer->SetContent(ProgramWidget.ToSharedRef());
    }
    else
    {
        ProgramContainer->SetContent(SNullWidget::NullWidget);
    }
}

void UInteractiveScreenComponent::UpdateCursorInternal(const FVector2D& NormalizedPosition)
{
    const FVector2D Clamped(
        FMath::Clamp(NormalizedPosition.X, 0.0f, 1.0f),
        FMath::Clamp(NormalizedPosition.Y, 0.0f, 1.0f));

    VirtualCursorPosition = Clamped;

    const FVector2D PixelPos(
        Clamped.X * WidgetSize.X,
        Clamped.Y * WidgetSize.Y);

    bPrevTaskButtonHovered = PrevTaskButtonRect.bIsValid && PrevTaskButtonRect.IsInside(PixelPos);
    bNextTaskButtonHovered = NextTaskButtonRect.bIsValid && NextTaskButtonRect.IsInside(PixelPos);
    bExitButtonHovered = ExitButtonRect.bIsValid && ExitButtonRect.IsInside(PixelPos);

    if (DisplayState == EDisplayState::ShowingProgram && CurrentProgram.IsValid() && ProgramWidget.IsValid())
    {
        const FVector2D ProgramCursor = ConvertPixelToProgramNormalized(PixelPos);
        const bool bChromePressed = bExitButtonPressed || bPrevTaskButtonPressed || bNextTaskButtonPressed;
        if (!bChromePressed)
        {
            const FScreenPointerEvent Event = BuildPointerEvent(ProgramCursor, PixelPos, FKey());
            CurrentProgram->OnPointerMoved(Event);
        }
    }

    UpdateHardwareCursor();
}

void UInteractiveScreenComponent::UpdateLayout(const FVector2D& Size)
{
    WidgetSize = Size;
    ProgramAreaSize = CalculateProgramAreaSize(Size);

    const FVector2D ProgramOrigin(
        ProgramPaddingX + ProgramContentPadding,
        ProgramPaddingTop + ProgramContentPadding);

    ProgramAreaRect = FBox2D(ProgramOrigin, ProgramOrigin + ProgramAreaSize);
    ProgramAreaRect.bIsValid = ProgramAreaSize.X > KINDA_SMALL_NUMBER && ProgramAreaSize.Y > KINDA_SMALL_NUMBER;

    const float FooterTop = Size.Y - FooterBarHeight;
    const float ButtonTop = FooterTop + (FooterBarHeight - ExitButtonSize.Y) * 0.5f;
    const float ExitButtonRight = Size.X - FooterHorizontalPadding;
    const FVector2D ExitButtonMin(ExitButtonRight - ExitButtonSize.X, ButtonTop);
    ExitButtonRect = FBox2D(ExitButtonMin, ExitButtonMin + ExitButtonSize);
    ExitButtonRect.bIsValid = true;

    const float NextButtonRight = ExitButtonMin.X - FooterButtonSpacing;
    const FVector2D NextButtonMin(NextButtonRight - NavigationButtonSize.X, ButtonTop);
    NextTaskButtonRect = FBox2D(NextButtonMin, NextButtonMin + NavigationButtonSize);
    NextTaskButtonRect.bIsValid = true;

    const float PrevButtonRight = NextButtonMin.X - FooterButtonSpacing;
    const FVector2D PrevButtonMin(PrevButtonRight - NavigationButtonSize.X, ButtonTop);
    PrevTaskButtonRect = FBox2D(PrevButtonMin, PrevButtonMin + NavigationButtonSize);
    PrevTaskButtonRect.bIsValid = true;

    if (ProgramContainer.IsValid())
    {
        ProgramContainer->SetWidthOverride(ProgramAreaSize.X);
        ProgramContainer->SetHeightOverride(ProgramAreaSize.Y);
    }

    UpdateProgramContent();
}

FVector2D UInteractiveScreenComponent::CalculateProgramAreaSize(const FVector2D& Size) const
{
    const float AvailableWidth = FMath::Max(Size.X - ((ProgramPaddingX + ProgramContentPadding) * 2.0f), 0.0f);
    const float AvailableHeight = FMath::Max(Size.Y - (ProgramPaddingTop + ProgramPaddingBottom + FooterBarHeight + (ProgramContentPadding * 2.0f)), 0.0f);

    return FVector2D(AvailableWidth, AvailableHeight);
}

FVector2D UInteractiveScreenComponent::ConvertPixelToProgramNormalized(const FVector2D& PixelPosition) const
{
    if (!ProgramAreaRect.bIsValid)
    {
        return FVector2D(0.5f, 0.5f);
    }

    const FVector2D Local = PixelPosition - ProgramAreaRect.Min;
    const FVector2D Size = ProgramAreaRect.GetSize();

    const float X = Size.X > KINDA_SMALL_NUMBER ? Local.X / Size.X : 0.5f;
    const float Y = Size.Y > KINDA_SMALL_NUMBER ? Local.Y / Size.Y : 0.5f;

    return FVector2D(
        FMath::Clamp(X, 0.0f, 1.0f),
        FMath::Clamp(Y, 0.0f, 1.0f));
}

void UInteractiveScreenComponent::DispatchSyntheticPointerReleases()
{
    if (ActivePointerButtons.Num() == 0)
    {
        bExitButtonPressed = false;
        bExitButtonHovered = false;
        bPrevTaskButtonPressed = false;
        bPrevTaskButtonHovered = false;
        bNextTaskButtonPressed = false;
        bNextTaskButtonHovered = false;
        bExitRequested = false;
        return;
    }

    const FVector2D PixelPos(
        VirtualCursorPosition.X * WidgetSize.X,
        VirtualCursorPosition.Y * WidgetSize.Y);

    const FVector2D ProgramCursor = ConvertPixelToProgramNormalized(PixelPos);

    if (CurrentProgram.IsValid())
    {
        const TArray<FKey> PointerKeys = ActivePointerButtons.Array();
        for (const FKey& PointerKey : PointerKeys)
        {
            const FScreenPointerEvent Event = BuildPointerEvent(ProgramCursor, PixelPos, PointerKey);
            CurrentProgram->OnPointerReleased(Event);
            ActivePointerButtons.Remove(PointerKey);
        }
        RefreshRender();
    }
    else
    {
        ActivePointerButtons.Empty();
    }

    bExitButtonPressed = false;
    bExitButtonHovered = false;
    bPrevTaskButtonPressed = false;
    bPrevTaskButtonHovered = false;
    bNextTaskButtonPressed = false;
    bNextTaskButtonHovered = false;
    bExitRequested = false;
}

void UInteractiveScreenComponent::ClearPointerState()
{
    ActivePointerButtons.Empty();
    bExitButtonPressed = false;
    bExitButtonHovered = false;
    bPrevTaskButtonPressed = false;
    bPrevTaskButtonHovered = false;
    bNextTaskButtonPressed = false;
    bNextTaskButtonHovered = false;
    UpdateHardwareCursor();
}

FScreenPointerEvent UInteractiveScreenComponent::BuildPointerEvent(const FVector2D& ProgramNormalized, const FVector2D& PixelPosition, const FKey& TriggerKey) const
{
    FScreenPointerEvent Event;
    Event.TriggerKey = TriggerKey;
    Event.ProgramNormalizedPosition = ProgramNormalized;
    Event.ProgramPixelPosition = FVector2D(
        ProgramNormalized.X * ProgramAreaSize.X,
        ProgramNormalized.Y * ProgramAreaSize.Y);
    Event.ScreenPixelPosition = PixelPosition;
    Event.PointerIndex = 0;
    Event.PressedKeys = ActivePointerButtons.Array();
    return Event;
}

void UInteractiveScreenComponent::UpdateHardwareCursor()
{
    EMouseCursor::Type DesiredCursor = EMouseCursor::Default;

    if (bExitButtonPressed || bPrevTaskButtonPressed || bNextTaskButtonPressed)
    {
        DesiredCursor = EMouseCursor::GrabHand;
    }
    else if (bExitButtonHovered || bPrevTaskButtonHovered || bNextTaskButtonHovered)
    {
        DesiredCursor = EMouseCursor::Hand;
    }
    else if (DisplayState == EDisplayState::ShowingProgram && CurrentProgram.IsValid())
    {
        DesiredCursor = CurrentProgram->GetCursorType();
    }

    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PC->CurrentMouseCursor = DesiredCursor;
        }
    }

    CachedCursorType = DesiredCursor;
}

void UInteractiveScreenComponent::SetDisplayState(UInteractiveScreenComponent::EDisplayState NewState)
{
    DisplayState = NewState;

    if (BootContainer.IsValid())
    {
        BootContainer->SetVisibility(DisplayState == EDisplayState::Booting ? EVisibility::Visible : EVisibility::Collapsed);
    }

    if (ProgramRootWidget.IsValid())
    {
        ProgramRootWidget->SetVisibility(DisplayState == EDisplayState::ShowingProgram ? EVisibility::Visible : EVisibility::Collapsed);
    }
}

void UInteractiveScreenComponent::BeginBootSequence()
{
    StopBootSequence();

    if (!ProgramContainer.IsValid())
    {
        return;
    }

    if (!BootWidget.IsValid())
    {
        BootWidget = SNew(SBootAnimationWidget).TextSize(TextSize);
    }
    else
    {
        BootWidget->Reset();
    }

    BootContainer->SetContent(BootWidget.ToSharedRef());
    NextBootLineIndex = 0;

    const float RangeMin = FMath::Max(0.0f, FMath::Min(BootInitialTimestampMin, BootInitialTimestampMax));
    const float RangeMax = FMath::Max(RangeMin, FMath::Max(BootInitialTimestampMin, BootInitialTimestampMax));
    BootElapsedSeconds = BootMessages.Num() > 0 ? FMath::FRandRange(RangeMin, RangeMax) : 0.0f;

    RefreshRender();

    if (BootMessages.Num() == 0)
    {
        HandleBootSequenceFinished();
        return;
    }

    AdvanceBootSequence();
}

void UInteractiveScreenComponent::AdvanceBootSequence()
{
    if (DisplayState != EDisplayState::Booting)
    {
        return;
    }

    if (!BootWidget.IsValid())
    {
        HandleBootSequenceFinished();
        return;
    }

    if (!BootMessages.IsValidIndex(NextBootLineIndex))
    {
        HandleBootSequenceFinished();
        return;
    }

    const FString& Message = BootMessages[NextBootLineIndex];
    const FString FormattedLine = FString::Printf(TEXT("[ %05.3f] %s"), BootElapsedSeconds, *Message);
    BootWidget->AppendLine(FormattedLine);
    ++NextBootLineIndex;

    RefreshRender();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float BaseDelay = FMath::Max(0.01f, BootLineBaseDelay);
    const float RandomDelay = BootLineDelayJitter > 0.0f ? FMath::FRandRange(0.0f, BootLineDelayJitter) : 0.0f;
    const float Delay = BaseDelay + RandomDelay;

    BootElapsedSeconds += Delay;

    if (NextBootLineIndex >= BootMessages.Num())
    {
        const float HoldDelay = FMath::Max(0.01f, BootCompletionHoldDelay);
        World->GetTimerManager().SetTimer(BootTimerHandle, this, &UInteractiveScreenComponent::HandleBootSequenceFinished, HoldDelay, false);
        return;
    }

    World->GetTimerManager().SetTimer(BootTimerHandle, this, &UInteractiveScreenComponent::AdvanceBootSequence, Delay, false);
}

void UInteractiveScreenComponent::HandleBootSequenceFinished()
{
    if (DisplayState != EDisplayState::Booting)
    {
        return;
    }

    StopBootSequence();

    SetDisplayState(EDisplayState::ShowingProgram);

    if (CurrentProgram.IsValid())
    {
        CurrentProgram->OnScreenResized(ProgramAreaSize);
    }

    UpdateProgramContent();
    RefreshRender();
    UpdateHardwareCursor();
}

void UInteractiveScreenComponent::StopBootSequence()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BootTimerHandle);
    }

    NextBootLineIndex = 0;
    BootElapsedSeconds = 0.0f;

    if (BootContainer.IsValid())
    {
        BootContainer->SetContent(SNullWidget::NullWidget);
    }

    if (ProgramContainer.IsValid())
    {
        ProgramContainer->SetContent(SNullWidget::NullWidget);
    }
}

FSlateColor UInteractiveScreenComponent::GetExitButtonBorderColor() const
{
    return FSlateColor(FLinearColor::Green);
}

FSlateColor UInteractiveScreenComponent::GetExitButtonTextColor() const
{
    if (bExitButtonPressed || bExitButtonHovered)
    {
        return FSlateColor(FLinearColor::Black);
    }
    return FSlateColor(FLinearColor::Green);
}

FSlateColor UInteractiveScreenComponent::GetExitButtonFillColor() const
{
    if (bExitButtonPressed)
    {
        return FSlateColor(FLinearColor::Green * 0.8f);
    }
    return bExitButtonHovered ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Transparent);
}

FSlateColor UInteractiveScreenComponent::GetPrevTaskButtonBorderColor() const
{
    return FSlateColor(FLinearColor::Green);
}

FSlateColor UInteractiveScreenComponent::GetPrevTaskButtonTextColor() const
{
    if (bPrevTaskButtonPressed || bPrevTaskButtonHovered)
    {
        return FSlateColor(FLinearColor::Black);
    }
    return FSlateColor(FLinearColor::Green);
}

FSlateColor UInteractiveScreenComponent::GetPrevTaskButtonFillColor() const
{
    if (bPrevTaskButtonPressed)
    {
        return FSlateColor(FLinearColor::Green * 0.8f);
    }
    return bPrevTaskButtonHovered ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Transparent);
}

FSlateColor UInteractiveScreenComponent::GetNextTaskButtonBorderColor() const
{
    return FSlateColor(FLinearColor::Green);
}

FSlateColor UInteractiveScreenComponent::GetNextTaskButtonTextColor() const
{
    if (bNextTaskButtonPressed || bNextTaskButtonHovered)
    {
        return FSlateColor(FLinearColor::Black);
    }
    return FSlateColor(FLinearColor::Green);
}

FSlateColor UInteractiveScreenComponent::GetNextTaskButtonFillColor() const
{
    if (bNextTaskButtonPressed)
    {
        return FSlateColor(FLinearColor::Green * 0.8f);
    }
    return bNextTaskButtonHovered ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Transparent);
}

FText UInteractiveScreenComponent::GetFooterDateText() const
{
    const FDateTime Now = FDateTime::Now();
    return FText::FromString(Now.ToString(TEXT("%b %d, %Y")));
}

FText UInteractiveScreenComponent::GetFooterTimeText() const
{
    const FDateTime Now = FDateTime::Now();
    return FText::FromString(Now.ToString(TEXT("%H:%M")));
}

FText UInteractiveScreenComponent::GetFooterWeekdayText() const
{
    const EDayOfWeek Day = FDateTime::Now().GetDayOfWeek();
    static const TCHAR* DayNames[] = {
        TEXT("Sunday"),
        TEXT("Monday"),
        TEXT("Tuesday"),
        TEXT("Wednesday"),
        TEXT("Thursday"),
        TEXT("Friday"),
        TEXT("Saturday")
    };

    const int32 Index = static_cast<int32>(Day);
    return (Index >= 0 && Index < UE_ARRAY_COUNT(DayNames))
        ? FText::FromString(DayNames[Index])
        : FText::FromString(TEXT(""));
}

FText UInteractiveScreenComponent::GetFooterTaskStatusText() const
{
    if (!CurrentProgram.IsValid())
    {
        return FText::FromString(TEXT("No Task"));
    }
    return CurrentProgram->IsTaskComplete() ? FText::FromString(TEXT("Task Complete")) : FText::FromString(TEXT("Task Pending"));
}

FSlateColor UInteractiveScreenComponent::GetFooterTaskStatusColor() const
{
    if (!CurrentProgram.IsValid())
    {
        return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f));
    }
    return CurrentProgram->IsTaskComplete() ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor(1.0f, 0.5f, 0.0f));
}

void UInteractiveScreenComponent::RequestTaskNavigation(int32 Direction)
{
    if (Direction == 0)
    {
        return;
    }

    bool bHandled = false;

    if (UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
    {
        bHandled = Manager->TrySelectProgramByOffset(Direction);
    }

    if (bHandled)
    {
        ActivatePendingProgram(false);
        RefreshRender();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[InteractiveScreen] Unable to navigate tasks; manager unavailable or selection failed (Direction=%d)."), Direction);
    }
}
