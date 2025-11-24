#pragma once

#include "CoreMinimal.h"
#include "Math/Box2D.h"
#include "Components/ActorComponent.h"
#include "GenericPlatform/ICursor.h"
#include "Slate/WidgetRenderer.h"
#include "UI/IScreenProgram.h"
#include "InteractiveScreenComponent.generated.h"

struct FTimerHandle;
class UTextureRenderTarget2D;
class SBox;
class SBootAnimationWidget;
class SBorder;

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

    /** Activate the pending program, optionally playing the boot animation first. */
    void ActivatePendingProgram(bool bShouldBoot);

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
    enum class EDisplayState : uint8
    {
        Idle,
        Booting,
        ShowingProgram
    };

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
    void SetDisplayState(EDisplayState NewState);
    void BeginBootSequence();
    void AdvanceBootSequence();
    void HandleBootSequenceFinished();
    void StopBootSequence();

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

    /** Thickness of lines (borders, dividers) in pixels */
    UPROPERTY(EditAnywhere, Category = "Interactive Screen", meta = (ClampMin = "1.0", ClampMax = "10.0"))
    float LineThickness = 2.0f;

    /** Base font size for UI text */
    UPROPERTY(EditAnywhere, Category = "Interactive Screen", meta = (ClampMin = "8", ClampMax = "48"))
    int32 TextSize = 16;

    /** Boot log lines to display during startup. */
    UPROPERTY(EditAnywhere, Category = "Interactive Screen|Boot")
    TArray<FString> BootMessages;

    /** Minimum initial timestamp for the first boot log line (seconds). */
    UPROPERTY(EditAnywhere, Category = "Interactive Screen|Boot", meta = (ClampMin = "0.0"))
    float BootInitialTimestampMin = 0.004f;

    /** Maximum initial timestamp for the first boot log line (seconds). */
    UPROPERTY(EditAnywhere, Category = "Interactive Screen|Boot", meta = (ClampMin = "0.0"))
    float BootInitialTimestampMax = 0.030f;

    /** Base delay between boot log lines (seconds). */
    UPROPERTY(EditAnywhere, Category = "Interactive Screen|Boot", meta = (ClampMin = "0.01"))
    float BootLineBaseDelay = 0.20f;

    /** Random jitter added to the base delay between boot log lines (seconds). */
    UPROPERTY(EditAnywhere, Category = "Interactive Screen|Boot", meta = (ClampMin = "0.0"))
    float BootLineDelayJitter = 0.35f;

    /** Hold time after the final boot log before switching to the program (seconds). */
    UPROPERTY(EditAnywhere, Category = "Interactive Screen|Boot", meta = (ClampMin = "0.0"))
    float BootCompletionHoldDelay = 0.85f;

    TSharedPtr<IScreenProgram> CurrentProgram;
    TSharedPtr<IScreenProgram> PendingProgram;
    TSharedPtr<SWidget> RootWidget;
    TSharedPtr<SWidget> ProgramRootWidget;
    TSharedPtr<SWidget> ProgramWidget;
    TSharedPtr<SBox> ProgramContainer;
    TSharedPtr<SBorder> BootContainer;
    TSharedPtr<SBootAnimationWidget> BootWidget;
    TUniquePtr<FWidgetRenderer> SlateWidgetRenderer;
    FVector2D WidgetSize = FVector2D::ZeroVector;
    FVector2D ProgramAreaSize = FVector2D::ZeroVector;
    FVector2D VirtualCursorPosition = FVector2D::ZeroVector;
    bool bWidgetInitialized = false;
    EDisplayState DisplayState = EDisplayState::Idle;
    int32 NextBootLineIndex = 0;
    FTimerHandle BootTimerHandle;
    float BootElapsedSeconds = 0.0f;

    FBox2D ProgramAreaRect;
    FBox2D ExitButtonRect;
    bool bExitButtonHovered = false;
    bool bExitButtonPressed = false;
    bool bExitRequested = false;
    TSet<FKey> ActivePointerButtons;
    EMouseCursor::Type CachedCursorType = EMouseCursor::Default;
};
