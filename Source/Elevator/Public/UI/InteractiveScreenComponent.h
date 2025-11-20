#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Slate/WidgetRenderer.h"
#include "UI/IScreenProgram.h"
#include "InteractiveScreenComponent.generated.h"

class UTextureRenderTarget2D;

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
    virtual ~UInteractiveScreenComponent();

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

    /** Process a click at the current cursor position. */
    void ProcessClick();

    /** Check if the active program requests to exit. */
    bool ShouldExit() const;

    /** Set the active program to display on this screen. */
    void SetProgram(TSharedPtr<IScreenProgram> InProgram);

    bool IsReady() const;
    FVector2D GetWidgetSize() const { return WidgetSize; }
    FVector2D GetCursor() const { return VirtualCursorPosition; }
    bool IsProgramTaskComplete() const;

private:
    void EnsureRenderer();
    void CreateWidgetIfNeeded();
    void UpdateWidgetSizeFromRenderTarget();
    void UpdateCursorInternal(const FVector2D& NormalizedPosition);

    UPROPERTY(EditAnywhere, Category = "Interactive Screen")
    TObjectPtr<UTextureRenderTarget2D> ScreenRenderTarget;

    TSharedPtr<IScreenProgram> CurrentProgram;
    TSharedPtr<SWidget> ProgramWidget;
    TUniquePtr<FWidgetRenderer> SlateWidgetRenderer;
    FVector2D WidgetSize = FVector2D::ZeroVector;
    FVector2D VirtualCursorPosition = FVector2D::ZeroVector;
    bool bWidgetInitialized = false;
};
