#include "UI/InteractiveScreenComponent.h"

#include "Procedural/InteractiveMonitorWidget.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Slate/WidgetRenderer.h"

UInteractiveScreenComponent::UInteractiveScreenComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInteractiveScreenComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeScreen();
}

void UInteractiveScreenComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    SlateWidgetRenderer.Reset();
    MonitorWidget.Reset();
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UInteractiveScreenComponent::SetRenderTarget(UTextureRenderTarget2D* InRenderTarget)
{
    ScreenRenderTarget = InRenderTarget;
    bWidgetInitialized = false;
}

void UInteractiveScreenComponent::InitializeScreen()
{
    CreateWidgetIfNeeded();
    UpdateWidgetSizeFromRenderTarget();
    if (MonitorWidget.IsValid())
    {
        MonitorWidget->UpdateCursorPosition(VirtualCursorPosition);
    }
    RefreshRender();
}

void UInteractiveScreenComponent::ResetCursor(const FVector2D& NormalizedPosition)
{
    UpdateCursorInternal(NormalizedPosition);
    RefreshRender();
}

void UInteractiveScreenComponent::UpdateCursor(const FVector2D& NormalizedPosition)
{
    UpdateCursorInternal(NormalizedPosition);
    RefreshRender();
}

void UInteractiveScreenComponent::RefreshRender()
{
    if (!ScreenRenderTarget || !MonitorWidget.IsValid())
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
        MonitorWidget.ToSharedRef(),
        WidgetSize,
        0.0f,
        false);
}

bool UInteractiveScreenComponent::IsReady() const
{
    return ScreenRenderTarget != nullptr && MonitorWidget.IsValid() && WidgetSize.X > 0.0f && WidgetSize.Y > 0.0f;
}

void UInteractiveScreenComponent::EnsureRenderer()
{
    if (!SlateWidgetRenderer.IsValid())
    {
        SlateWidgetRenderer = MakeUnique<FWidgetRenderer>(true, false);
    }
}

void UInteractiveScreenComponent::CreateWidgetIfNeeded()
{
    if (!MonitorWidget.IsValid())
    {
        MonitorWidget = SNew(SInteractiveMonitorWidget);
        bWidgetInitialized = false;
    }
}

void UInteractiveScreenComponent::UpdateWidgetSizeFromRenderTarget()
{
    if (!ScreenRenderTarget || !MonitorWidget.IsValid())
    {
        return;
    }

    const FVector2D Size(ScreenRenderTarget->SizeX, ScreenRenderTarget->SizeY);
    if (Size.X <= 0.0f || Size.Y <= 0.0f)
    {
        return;
    }

    if (!bWidgetInitialized || !Size.Equals(WidgetSize))
    {
        WidgetSize = Size;
        MonitorWidget->SetWidgetSize(Size);
        bWidgetInitialized = true;
    }
}

void UInteractiveScreenComponent::UpdateCursorInternal(const FVector2D& NormalizedPosition)
{
    const FVector2D Clamped(
        FMath::Clamp(NormalizedPosition.X, 0.0f, 1.0f),
        FMath::Clamp(NormalizedPosition.Y, 0.0f, 1.0f));

    VirtualCursorPosition = Clamped;
    if (MonitorWidget.IsValid())
    {
        MonitorWidget->UpdateCursorPosition(VirtualCursorPosition);
    }
}
