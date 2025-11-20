#pragma once

#include "CoreMinimal.h"
#include "Math/Box2D.h"
#include "Components/ActorComponent.h"
#include "GenericPlatform/ICursor.h"
#include "Slate/WidgetRenderer.h"
#include "UI/IScreenProgram.h"
#include "InteractiveScreenComponent.generated.h"

class UTextureRenderTarget2D;
class SBox;

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

    /** Process a pointer press at the current cursor position. */
    void ProcessPointerPressed(const FKey& PointerKey);

    /** Process a pointer release at the current cursor position. */
    void ProcessPointerReleased(const FKey& PointerKey);

    /** Check if the interface requested to exit; returns true once per click. */
    bool ShouldExit();

    /** Set the active program to display on this screen. */
    void SetProgram(TSharedPtr<IScreenProgram> InProgram);

    bool IsReady() const;
    FVector2D GetWidgetSize() const { return WidgetSize; }
    FVector2D GetCursor() const { return VirtualCursorPosition; }
    bool IsProgramTaskComplete() const;

private:
    void EnsureRenderer();
    void CreateInterfaceIfNeeded();
    void UpdateWidgetSizeFromRenderTarget();
    void UpdateProgramContent();
    void UpdateCursorInternal(const FVector2D& NormalizedPosition);
    void UpdateLayout(const FVector2D& Size);
    FVector2D CalculateProgramAreaSize(const FVector2D& Size) const;
    FVector2D ConvertPixelToProgramNormalized(const FVector2D& PixelPosition) const;
    void DispatchSyntheticPointerReleases();
    void ClearPointerState();
    FScreenPointerEvent BuildPointerEvent(const FVector2D& ProgramNormalized, const FVector2D& PixelPosition, const FKey& TriggerKey) const;
    void UpdateHardwareCursor();

    FSlateColor GetExitButtonBorderColor() const;
    FSlateColor GetExitButtonTextColor() const;
    FSlateColor GetExitButtonFillColor() const;
    FText GetFooterDateText() const;
    FText GetFooterTimeText() const;
    FText GetFooterWeekdayText() const;
    FText GetFooterTaskStatusText() const;
    FSlateColor GetFooterTaskStatusColor() const;

    UPROPERTY(EditAnywhere, Category = "Interactive Screen")
    TObjectPtr<UTextureRenderTarget2D> ScreenRenderTarget;

    TSharedPtr<IScreenProgram> CurrentProgram;
    TSharedPtr<SWidget> RootWidget;
    TSharedPtr<SWidget> ProgramWidget;
    TSharedPtr<SBox> ProgramContainer;
    TUniquePtr<FWidgetRenderer> SlateWidgetRenderer;
    FVector2D WidgetSize = FVector2D::ZeroVector;
    FVector2D ProgramAreaSize = FVector2D::ZeroVector;
    FVector2D VirtualCursorPosition = FVector2D::ZeroVector;
    bool bWidgetInitialized = false;

    FBox2D ProgramAreaRect;
    FBox2D ExitButtonRect;
    bool bExitButtonHovered = false;
    bool bExitButtonPressed = false;
    bool bExitRequested = false;
    TSet<FKey> ActivePointerButtons;
    EMouseCursor::Type CachedCursorType = EMouseCursor::Default;
};
