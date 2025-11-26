#include "UI/ScreenProgramWindow.h"

#include "Logging/LogMacros.h"
#include "Misc/AssertionMacros.h"
#include "Styling/CoreStyle.h"
#include "Layout/Children.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

FScreenProgramWindowBuilder::FScreenProgramWindowBuilder(FWindowedScreenProgramBase& InOwner)
    : Owner(InOwner)
{
}

void FScreenProgramWindowBuilder::AddWindow(const FScreenProgramWindowConfig& Config, const FScreenProgramWindowChrome& Chrome, FContentBuilder ContentBuilder)
{
    Owner.AddWindowInternal(Config, Chrome, MoveTemp(ContentBuilder));
}

FWindowedScreenProgramBase::FWindowedScreenProgramBase()
{
    Interaction.Reset();
}

TSharedRef<SWidget> FWindowedScreenProgramBase::BuildProgramWidget()
{
    RootCanvas = SNew(SCanvas);
    WindowInstances.Reset();
    Interaction.Reset();
    HoveredWindowId = NAME_None;
    NextZOrder = 0;

    FScreenProgramWindowBuilder Builder(*this);
    BuildWindowLayout(Builder);

    if (WindowInstances.Num() == 0)
    {
        return BuildProgramContent();
    }

    TSharedRef<FWindowedScreenProgramBase> SharedThis = StaticCastSharedRef<FWindowedScreenProgramBase>(AsShared());

    for (FWindowInstance& Instance : WindowInstances)
    {
        ClampWindowToViewport(Instance);

        const TSharedRef<SWidget> WindowWidget = BuildWindowWidget(Instance, GetProgramStyle());
        Instance.RootWidget = WindowWidget;

        RootCanvas->AddSlot()
            .Expose(Instance.CanvasSlot)
            .HAlign(HAlign_Left)
            .VAlign(VAlign_Top)
            .Position(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(SharedThis, &FWindowedScreenProgramBase::GetWindowPositionAttribute, Instance.Config.WindowId)))
            .Size(TAttribute<FVector2D>::Create(TAttribute<FVector2D>::FGetter::CreateSP(SharedThis, &FWindowedScreenProgramBase::GetWindowSizeAttribute, Instance.Config.WindowId)))
            [
                WindowWidget
            ];
    }

    return RootCanvas.ToSharedRef();
}

TSharedRef<SWidget> FWindowedScreenProgramBase::BuildProgramContent()
{
    return RootCanvas.IsValid() ? RootCanvas.ToSharedRef() : SNullWidget::NullWidget;
}

bool FWindowedScreenProgramBase::PreHandlePointerMoved(const FScreenPointerEvent& Event)
{
    return ProcessPointerMoved(Event);
}

bool FWindowedScreenProgramBase::PreHandlePointerPressed(const FScreenPointerEvent& Event)
{
    return ProcessPointerPressed(Event);
}

bool FWindowedScreenProgramBase::PreHandlePointerReleased(const FScreenPointerEvent& Event)
{
    return ProcessPointerReleased(Event);
}

void FWindowedScreenProgramBase::HandlePointerMoved(const FScreenPointerEvent&, bool)
{
}

void FWindowedScreenProgramBase::HandlePointerPressed(const FScreenPointerEvent&, bool)
{
}

void FWindowedScreenProgramBase::HandlePointerReleased(const FScreenPointerEvent&, bool)
{
}

void FWindowedScreenProgramBase::HandleScreenResized(const FVector2D& NewSize)
{
    for (FWindowInstance& Instance : WindowInstances)
    {
        ClampWindowToViewport(Instance);
    }
    SetBaseCursor(EMouseCursor::Default);
}

bool FWindowedScreenProgramBase::TryGetWindowMetrics(FName WindowId, FScreenProgramWindowMetrics& OutMetrics) const
{
    if (const FWindowInstance* Instance = FindWindow(WindowId))
    {
        OutMetrics.Position = Instance->Position;
        OutMetrics.Size = Instance->Size;
        OutMetrics.TitleBarHeight = Instance->Config.TitleBarHeight;
        OutMetrics.ResizeHandleSize = Instance->Config.ResizeHandleSize;
        OutMetrics.ContentPadding = Instance->Chrome.ContentPadding;
        return true;
    }

    return false;
}

void FWindowedScreenProgramBase::AddWindowInternal(const FScreenProgramWindowConfig& Config, const FScreenProgramWindowChrome& Chrome, FScreenProgramWindowBuilder::FContentBuilder&& ContentBuilder)
{
    if (Config.WindowId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("Ignoring workstation window with None identifier."));
        return;
    }

    if (FindWindow(Config.WindowId) != nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Duplicate workstation window id '%s' ignored."), *Config.WindowId.ToString());
        return;
    }

    FWindowInstance Instance;
    Instance.Config = Config;
    Instance.Chrome = Chrome;
    Instance.Position = Config.InitialPosition;
    Instance.Size.X = FMath::Max(Config.InitialSize.X, Config.MinimumSize.X);
    Instance.Size.Y = FMath::Max(Config.InitialSize.Y, Config.MinimumSize.Y);
    Instance.ZOrder = NextZOrder++;

    if (ContentBuilder)
    {
        Instance.ContentWidget = ContentBuilder(Config);
    }

    if (!Instance.ContentWidget.IsValid())
    {
        Instance.ContentWidget = SNullWidget::NullWidget;
    }

    WindowInstances.Add(MoveTemp(Instance));
}

FWindowedScreenProgramBase::FWindowInstance* FWindowedScreenProgramBase::FindWindow(FName WindowId)
{
    for (FWindowInstance& Instance : WindowInstances)
    {
        if (Instance.Config.WindowId == WindowId)
        {
            return &Instance;
        }
    }
    return nullptr;
}

const FWindowedScreenProgramBase::FWindowInstance* FWindowedScreenProgramBase::FindWindow(FName WindowId) const
{
    for (const FWindowInstance& Instance : WindowInstances)
    {
        if (Instance.Config.WindowId == WindowId)
        {
            return &Instance;
        }
    }
    return nullptr;
}

void FWindowedScreenProgramBase::BringWindowToFront(FWindowInstance& Instance)
{
    if (!Instance.CanvasSlot || !RootCanvas.IsValid())
    {
        return;
    }

    Instance.ZOrder = NextZOrder++;

    if (FChildren* Children = RootCanvas->GetChildren())
    {
        auto* CanvasChildren = static_cast<TPanelChildren<SCanvas::FSlot>*>(Children);
        const int32 NumChildren = CanvasChildren->Num();
        if (NumChildren <= 1)
        {
            return;
        }

        for (int32 Index = 0; Index < NumChildren; ++Index)
        {
            if (&(*CanvasChildren)[Index] == Instance.CanvasSlot)
            {
                CanvasChildren->Move(Index, NumChildren - 1);
                break;
            }
        }
    }
}

void FWindowedScreenProgramBase::ClampWindowToViewport(FWindowInstance& Instance)
{
    Instance.Size.X = FMath::Max(Instance.Size.X, Instance.Config.MinimumSize.X);
    Instance.Size.Y = FMath::Max(Instance.Size.Y, Instance.Config.MinimumSize.Y);

    if (!Instance.Config.bClampToViewport)
    {
        return;
    }

    const FVector2D AvailableSize = GetProgramSize();
    const float MaxWidth = FMath::Max(AvailableSize.X - Instance.Position.X, Instance.Config.MinimumSize.X);
    const float MaxHeight = FMath::Max(AvailableSize.Y - Instance.Position.Y, Instance.Config.MinimumSize.Y);

    Instance.Size.X = FMath::Clamp(Instance.Size.X, Instance.Config.MinimumSize.X, MaxWidth);
    Instance.Size.Y = FMath::Clamp(Instance.Size.Y, Instance.Config.MinimumSize.Y, MaxHeight);

    const float LimitX = FMath::Max(AvailableSize.X - Instance.Size.X, 0.0f);
    const float LimitY = FMath::Max(AvailableSize.Y - Instance.Size.Y, 0.0f);

    Instance.Position.X = FMath::Clamp(Instance.Position.X, 0.0f, LimitX);
    Instance.Position.Y = FMath::Clamp(Instance.Position.Y, 0.0f, LimitY);
}

FWindowedScreenProgramBase::FWindowHitResult FWindowedScreenProgramBase::HitTestWindows(const FVector2D& ProgramPixel) const
{
    FWindowHitResult Result;

    const FWindowInstance* BestInstance = nullptr;
    for (const FWindowInstance& Instance : WindowInstances)
    {
        const FVector2D Min = Instance.Position;
        const FVector2D Max = Instance.Position + Instance.Size;
        const bool bInside = ProgramPixel.X >= Min.X && ProgramPixel.X <= Max.X && ProgramPixel.Y >= Min.Y && ProgramPixel.Y <= Max.Y;
        if (!bInside)
        {
            continue;
        }

        if (!BestInstance || Instance.ZOrder >= BestInstance->ZOrder)
        {
            BestInstance = &Instance;
        }
    }

    if (!BestInstance)
    {
        return Result;
    }

    Result.WindowId = BestInstance->Config.WindowId;
    Result.LocalPosition = ProgramPixel - BestInstance->Position;

    const float TitleHeight = BestInstance->Config.TitleBarHeight;
    const float ResizeSize = BestInstance->Config.ResizeHandleSize;

    if (BestInstance->Config.bCanResize && Result.LocalPosition.X >= BestInstance->Size.X - ResizeSize && Result.LocalPosition.Y >= BestInstance->Size.Y - ResizeSize)
    {
        Result.Region = EScreenProgramWindowHitRegion::ResizeHandle;
    }
    else if (Result.LocalPosition.Y <= TitleHeight)
    {
        Result.Region = EScreenProgramWindowHitRegion::TitleBar;
    }
    else
    {
        Result.Region = EScreenProgramWindowHitRegion::Content;
    }

    return Result;
}

bool FWindowedScreenProgramBase::ProcessPointerPressed(const FScreenPointerEvent& Event)
{
    FWindowHitResult Hit = HitTestWindows(Event.ProgramPixelPosition);
    UpdateHoverState(Hit);

    if (Hit.WindowId == NAME_None)
    {
        SetBaseCursor(EMouseCursor::Default);
        return false;
    }

    FWindowInstance* Instance = FindWindow(Hit.WindowId);
    if (!Instance)
    {
        return false;
    }

    BringWindowToFront(*Instance);

    const FKey PointerKey = Event.TriggerKey.IsValid() ? Event.TriggerKey : EKeys::LeftMouseButton;

    if (Hit.Region == EScreenProgramWindowHitRegion::TitleBar && Instance->Config.bCanMove)
    {
        Interaction.WindowId = Hit.WindowId;
        Interaction.Region = Hit.Region;
        Interaction.PointerKey = PointerKey;
        Interaction.GrabOffset = Event.ProgramPixelPosition - Instance->Position;
        UpdateCursorFromInteraction();
        return true;
    }

    if (Hit.Region == EScreenProgramWindowHitRegion::ResizeHandle && Instance->Config.bCanResize)
    {
        Interaction.WindowId = Hit.WindowId;
        Interaction.Region = Hit.Region;
        Interaction.PointerKey = PointerKey;
        Interaction.GrabOffset = FVector2D::ZeroVector;
        UpdateCursorFromInteraction();
        return true;
    }

    UpdateBaseCursorForHit(Hit);
    return false;
}

bool FWindowedScreenProgramBase::ProcessPointerMoved(const FScreenPointerEvent& Event)
{
    if (Interaction.IsActive())
    {
        FWindowInstance* Instance = FindWindow(Interaction.WindowId);
        if (!Instance)
        {
            Interaction.Reset();
            SetBaseCursor(EMouseCursor::Default);
            return false;
        }

        if (Interaction.Region == EScreenProgramWindowHitRegion::TitleBar && Instance->Config.bCanMove)
        {
            Instance->Position = Event.ProgramPixelPosition - Interaction.GrabOffset;
            ClampWindowToViewport(*Instance);
        }
        else if (Interaction.Region == EScreenProgramWindowHitRegion::ResizeHandle && Instance->Config.bCanResize)
        {
            FVector2D DesiredSize = Event.ProgramPixelPosition - Instance->Position;
            DesiredSize.X = FMath::Max(DesiredSize.X, Instance->Config.MinimumSize.X);
            DesiredSize.Y = FMath::Max(DesiredSize.Y, Instance->Config.MinimumSize.Y);
            Instance->Size = DesiredSize;
            ClampWindowToViewport(*Instance);
        }

        UpdateCursorFromInteraction();
        return true;
    }

    FWindowHitResult Hit = HitTestWindows(Event.ProgramPixelPosition);
    UpdateHoverState(Hit);
    UpdateBaseCursorForHit(Hit);
    return false;
}

bool FWindowedScreenProgramBase::ProcessPointerReleased(const FScreenPointerEvent& Event)
{
    if (!Interaction.IsActive())
    {
        FWindowHitResult Hit = HitTestWindows(Event.ProgramPixelPosition);
        UpdateHoverState(Hit);
        UpdateBaseCursorForHit(Hit);
        return false;
    }

    if (Interaction.PointerKey.IsValid() && Event.TriggerKey.IsValid() && Interaction.PointerKey != Event.TriggerKey)
    {
        return false;
    }

    Interaction.Reset();

    FWindowHitResult Hit = HitTestWindows(Event.ProgramPixelPosition);
    UpdateHoverState(Hit);
    UpdateBaseCursorForHit(Hit);
    return true;
}

void FWindowedScreenProgramBase::UpdateHoverState(const FWindowHitResult& Hit)
{
    HoveredWindowId = Hit.WindowId;
}

void FWindowedScreenProgramBase::UpdateBaseCursorForHit(const FWindowHitResult& Hit)
{
    if (Interaction.IsActive())
    {
        UpdateCursorFromInteraction();
        return;
    }

    if (Hit.WindowId == NAME_None)
    {
        SetBaseCursor(EMouseCursor::Default);
        return;
    }

    const FWindowInstance* Instance = FindWindow(Hit.WindowId);
    if (!Instance)
    {
        SetBaseCursor(EMouseCursor::Default);
        return;
    }

    switch (Hit.Region)
    {
    case EScreenProgramWindowHitRegion::ResizeHandle:
        SetBaseCursor(Instance->Config.bCanResize ? EMouseCursor::ResizeSouthEast : EMouseCursor::Default);
        break;
    case EScreenProgramWindowHitRegion::TitleBar:
        SetBaseCursor(Instance->Config.bCanMove ? EMouseCursor::Hand : EMouseCursor::Default);
        break;
    default:
        SetBaseCursor(EMouseCursor::Default);
        break;
    }
}

void FWindowedScreenProgramBase::UpdateCursorFromInteraction()
{
    switch (Interaction.Region)
    {
    case EScreenProgramWindowHitRegion::TitleBar:
        SetBaseCursor(EMouseCursor::GrabHand);
        break;
    case EScreenProgramWindowHitRegion::ResizeHandle:
        SetBaseCursor(EMouseCursor::ResizeSouthEast);
        break;
    default:
        SetBaseCursor(EMouseCursor::Default);
        break;
    }
}

TSharedRef<SWidget> FWindowedScreenProgramBase::BuildWindowWidget(FWindowInstance& Instance, const FScreenProgramStyle& Style)
{
    const FSlateBrush* SolidBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
    const FSlateBrush* TransparentBrush = FCoreStyle::Get().GetBrush("NoBrush");

    // Use style-based title bar height if config uses default, otherwise use config value
    const float DefaultTitleBarHeight = 30.0f;
    const float TitleBarHeight = FMath::IsNearlyEqual(Instance.Config.TitleBarHeight, DefaultTitleBarHeight)
        ? Style.GetTitleBarHeight()
        : Instance.Config.TitleBarHeight;

    // Update instance config so metrics are accurate
    Instance.Config.TitleBarHeight = TitleBarHeight;

    return SNew(SBorder)
        .BorderImage(SolidBrush)
        .BorderBackgroundColor(Instance.Chrome.BorderColor)
        .Padding(Style.LineThickness)
        [
            SNew(SBorder)
            .BorderImage(SolidBrush)
            .BorderBackgroundColor(Instance.Chrome.BackgroundColor)
            .Padding(0.0f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBox)
                    .HeightOverride(TitleBarHeight)
                    [
                        SNew(SBorder)
                        .BorderImage(TransparentBrush)
                        .BorderBackgroundColor(Instance.Chrome.TitleBarColor)
                        .Padding(Style.GetTitleBarPadding())
                        [
                            SNew(STextBlock)
                            .Text(Instance.Config.Title)
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", Style.TextSize))
                            .ColorAndOpacity(Instance.Chrome.TitleTextColor)
                        ]
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBox)
                    .HeightOverride(Style.LineThickness)
                    [
                        SNew(SImage)
                        .Image(SolidBrush)
                        .ColorAndOpacity(Instance.Chrome.BorderColor)
                    ]
                ]
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    SNew(SBorder)
                    .BorderImage(TransparentBrush)
                    .BorderBackgroundColor(Instance.Chrome.BackgroundColor)
                    .Padding(Instance.Chrome.ContentPadding)
                    [
                        Instance.ContentWidget.ToSharedRef()
                    ]
                ]
            ]
        ];
}

FVector2D FWindowedScreenProgramBase::GetWindowPositionAttribute(FName WindowId) const
{
    if (const FWindowInstance* Instance = FindWindow(WindowId))
    {
        return Instance->Position;
    }
    return FVector2D::ZeroVector;
}

FVector2D FWindowedScreenProgramBase::GetWindowSizeAttribute(FName WindowId) const
{
    if (const FWindowInstance* Instance = FindWindow(WindowId))
    {
        return Instance->Size;
    }
    return FVector2D::ZeroVector;
}
