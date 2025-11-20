#include "UI/InteractiveScreenComponent.h"

#include "UI/IScreenProgram.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"

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
        CurrentProgram->OnDeactivated();
    }
    SlateWidgetRenderer.Reset();
    ProgramWidget.Reset();
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
    CreateWidgetIfNeeded();
    UpdateWidgetSizeFromRenderTarget();
    if (CurrentProgram.IsValid())
    {
        CurrentProgram->UpdateCursor(VirtualCursorPosition);
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

void UInteractiveScreenComponent::SetProgram(TSharedPtr<IScreenProgram> InProgram)
{
    if (CurrentProgram.IsValid())
    {
        CurrentProgram->OnDeactivated();
    }

    CurrentProgram = InProgram;
    ProgramWidget.Reset();
    bWidgetInitialized = false;

    if (CurrentProgram.IsValid())
    {
        CurrentProgram->OnActivated();
        CreateWidgetIfNeeded();
        RefreshRender();
    }
}

void UInteractiveScreenComponent::ProcessClick()
{
    if (!CurrentProgram.IsValid())
    {
        return;
    }

    const bool bHandled = CurrentProgram->HandleClick();
    if (bHandled)
    {
        RefreshRender();
    }
}

bool UInteractiveScreenComponent::ShouldExit() const
{
    return CurrentProgram.IsValid() && CurrentProgram->ShouldExit();
}

void UInteractiveScreenComponent::RefreshRender()
{
    if (!ScreenRenderTarget || !ProgramWidget.IsValid())
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
        ProgramWidget.ToSharedRef(),
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
    return ScreenRenderTarget != nullptr && ProgramWidget.IsValid() && WidgetSize.X > 0.0f && WidgetSize.Y > 0.0f;
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
    if (!ProgramWidget.IsValid() && CurrentProgram.IsValid())
    {
        ProgramWidget = CurrentProgram->CreateWidget(WidgetSize);
        bWidgetInitialized = false;
    }
}

void UInteractiveScreenComponent::UpdateWidgetSizeFromRenderTarget()
{
    if (!ScreenRenderTarget || !CurrentProgram.IsValid())
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
        CurrentProgram->OnScreenResized(Size);
        
        // Recreate widget with new size
        ProgramWidget = CurrentProgram->CreateWidget(Size);
        bWidgetInitialized = true;
    }
}

void UInteractiveScreenComponent::UpdateCursorInternal(const FVector2D& NormalizedPosition)
{
    const FVector2D Clamped(
        FMath::Clamp(NormalizedPosition.X, 0.0f, 1.0f),
        FMath::Clamp(NormalizedPosition.Y, 0.0f, 1.0f));

    VirtualCursorPosition = Clamped;
    if (CurrentProgram.IsValid())
    {
        CurrentProgram->UpdateCursor(VirtualCursorPosition);
    }
}
