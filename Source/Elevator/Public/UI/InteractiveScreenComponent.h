#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Slate/WidgetRenderer.h"
#include "InteractiveScreenComponent.generated.h"

class UTextureRenderTarget2D;
class SInteractiveMonitorWidget;

/**
 * Component that manages rendering of a Slate monitor widget to a texture render target
 * and tracks a virtual cursor used for interaction logic.
 */
UCLASS(ClassGroup = (UI), Blueprintable, meta = (BlueprintSpawnableComponent))
class ELEVATOR_API UInteractiveScreenComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInteractiveScreenComponent();

    virtual void BeginPlay() override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

    /** Assign the render target used for drawing the monitor UI. */
    void SetRenderTarget(UTextureRenderTarget2D* InRenderTarget);

    /** Ensure the widget and render target are in sync and draw the current frame. */
    void InitializeScreen();

    /** Reset the virtual cursor to the provided normalized position and refresh the render. */
    void ResetCursor(const FVector2D& NormalizedPosition = FVector2D::ZeroVector);

    /** Update the virtual cursor and redraw the widget. */
    void UpdateCursor(const FVector2D& NormalizedPosition);

    /** Redraw the widget using the current cursor state. */
    void RefreshRender();

    bool IsReady() const;
    FVector2D GetWidgetSize() const { return WidgetSize; }
    FVector2D GetCursor() const { return VirtualCursorPosition; }

    /** Direct access to the underlying monitor widget if specialized configuration is required. */
    TSharedPtr<SInteractiveMonitorWidget> GetMonitorWidget() const { return MonitorWidget; }

private:
    void EnsureRenderer();
    void CreateWidgetIfNeeded();
    void UpdateWidgetSizeFromRenderTarget();
    void UpdateCursorInternal(const FVector2D& NormalizedPosition);

    UPROPERTY(EditAnywhere, Category = "Interactive Screen")
    TObjectPtr<UTextureRenderTarget2D> ScreenRenderTarget;

    TSharedPtr<SInteractiveMonitorWidget> MonitorWidget;
    TUniquePtr<FWidgetRenderer> SlateWidgetRenderer;
    FVector2D WidgetSize = FVector2D::ZeroVector;
    FVector2D VirtualCursorPosition = FVector2D::ZeroVector;
    bool bWidgetInitialized = false;
};
