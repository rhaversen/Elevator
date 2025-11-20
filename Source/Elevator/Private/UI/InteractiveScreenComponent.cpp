#include "UI/InteractiveScreenComponent.h"

#include "UI/IScreenProgram.h"
#include "UI/SlateWidgetHelpers.h"

#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/DateTime.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    constexpr float ProgramPaddingX = 0.0f;
    constexpr float ProgramPaddingTop = 0.0f;
    constexpr float ProgramPaddingBottom = 24.0f;
    constexpr float ProgramContentPadding = 24.0f;
    constexpr float FooterBarHeight = 96.0f;
    constexpr float FooterTextSpacing = 24.0f;
    constexpr float FooterHorizontalPadding = 48.0f;
    constexpr float FooterExitButtonSpacing = 12.0f;
    const FVector2D ExitButtonSize(180.0f, 60.0f);
}

UInteractiveScreenComponent::UInteractiveScreenComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
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
    CreateInterfaceIfNeeded();
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

    CurrentProgram = InProgram;
    ProgramWidget.Reset();
    bWidgetInitialized = false;
    bExitRequested = false;
    bExitButtonHovered = false;
    ClearPointerState();
    CachedCursorType = EMouseCursor::Default;

    CreateInterfaceIfNeeded();

    if (CurrentProgram.IsValid())
    {
        CurrentProgram->OnActivated();
    }

    UpdateWidgetSizeFromRenderTarget();
    UpdateCursorInternal(VirtualCursorPosition);
    RefreshRender();
    UpdateHardwareCursor();
}

void UInteractiveScreenComponent::ProcessPointerPressed(const FKey& PointerKey)
{
    const FVector2D PixelPos(
        VirtualCursorPosition.X * WidgetSize.X,
        VirtualCursorPosition.Y * WidgetSize.Y);

    ActivePointerButtons.Add(PointerKey);

    if (ExitButtonRect.bIsValid && ExitButtonRect.IsInside(PixelPos))
    {
        bExitButtonPressed = true;
        RefreshRender();
        UpdateHardwareCursor();
        return;
    }

    if (!CurrentProgram.IsValid())
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

    const bool bWasPressedOnExit = bExitButtonPressed;
    const bool bIsReleasingOverExit = ExitButtonRect.bIsValid && ExitButtonRect.IsInside(PixelPos);

    ActivePointerButtons.Remove(PointerKey);
    bExitButtonPressed = false;

    if (bWasPressedOnExit)
    {
        if (bIsReleasingOverExit)
        {
            bExitRequested = true;
        }
        RefreshRender();
        UpdateHardwareCursor();
        return;
    }

    if (!CurrentProgram.IsValid())
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
    if (!ScreenRenderTarget || !RootWidget.IsValid())
    {
        return;
    }

    UpdateWidgetSizeFromRenderTarget();
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

    const FSlateBrush* NoBrush = FCoreStyle::Get().GetBrush("NoBrush");

    TSharedRef<SOverlay> Root = SNew(SOverlay)
        // Background
        + SOverlay::Slot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SNew(SImage)
            .ColorAndOpacity(FLinearColor::Black)
        ]
        // Outer chrome and footer
        + SOverlay::Slot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SNew(SVerticalBox)
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
                .HeightOverride(2.0f)
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
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                            .ColorAndOpacity(FSlateColor(FLinearColor::Green))
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(FMargin(FooterTextSpacing, 0.0f, 0.0f, 0.0f))
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this]() { return GetFooterDateText(); })
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                            .ColorAndOpacity(FSlateColor(FLinearColor::Green))
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(FMargin(FooterTextSpacing, 0.0f, 0.0f, 0.0f))
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this]() { return GetFooterTimeText(); })
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                            .ColorAndOpacity(FSlateColor(FLinearColor::Green))
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(FMargin(FooterTextSpacing, 0.0f, 0.0f, 0.0f))
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this]() { return GetFooterTaskStatusText(); })
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
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
                                ExitButtonSize,
                                FText::FromString(TEXT("Exit")),
                                14,
                                TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UInteractiveScreenComponent::GetExitButtonBorderColor)),
                                TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UInteractiveScreenComponent::GetExitButtonFillColor)),
                                TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &UInteractiveScreenComponent::GetExitButtonTextColor)))
                        ]
                    ]
                ]
            ]
        ];

    RootWidget = Root;
}

void UInteractiveScreenComponent::UpdateWidgetSizeFromRenderTarget()
{
    CreateInterfaceIfNeeded();

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
    if (!bWidgetInitialized || bSizeChanged)
    {
        UpdateLayout(Size);
        bWidgetInitialized = true;
        UpdateCursorInternal(VirtualCursorPosition);
    }
    else if (CurrentProgram.IsValid() && !ProgramWidget.IsValid())
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

    if (!CurrentProgram.IsValid())
    {
        ProgramWidget.Reset();
        ProgramContainer->SetContent(SNullWidget::NullWidget);
        return;
    }

    CurrentProgram->OnScreenResized(ProgramAreaSize);
    ProgramWidget = CurrentProgram->CreateWidget(ProgramAreaSize);

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

    bExitButtonHovered = ExitButtonRect.bIsValid && ExitButtonRect.IsInside(PixelPos);

    if (CurrentProgram.IsValid())
    {
        const FVector2D ProgramCursor = ConvertPixelToProgramNormalized(PixelPos);
        if (!bExitButtonPressed)
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
    const float ExitButtonTop = FooterTop + (FooterBarHeight - ExitButtonSize.Y) * 0.5f;
    ExitButtonRect = FBox2D(
        FVector2D(Size.X - FooterHorizontalPadding - ExitButtonSize.X, ExitButtonTop),
        FVector2D(Size.X - FooterHorizontalPadding, ExitButtonTop + ExitButtonSize.Y));
    ExitButtonRect.bIsValid = true;

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
    bExitRequested = false;
}

void UInteractiveScreenComponent::ClearPointerState()
{
    ActivePointerButtons.Empty();
    bExitButtonPressed = false;
    bExitButtonHovered = false;
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

    if (bExitButtonPressed)
    {
        DesiredCursor = EMouseCursor::GrabHand;
    }
    else if (bExitButtonHovered)
    {
        DesiredCursor = EMouseCursor::Hand;
    }
    else if (CurrentProgram.IsValid())
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

FSlateColor UInteractiveScreenComponent::GetExitButtonBorderColor() const
{
    return FSlateColor(FLinearColor::Green);
}

FSlateColor UInteractiveScreenComponent::GetExitButtonTextColor() const
{
    return bExitButtonHovered ? FSlateColor(FLinearColor::Black) : FSlateColor(FLinearColor::Green);
}

FSlateColor UInteractiveScreenComponent::GetExitButtonFillColor() const
{
    return bExitButtonHovered ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Transparent);
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
