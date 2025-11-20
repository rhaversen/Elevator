#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GenericPlatform/ICursor.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Interface for screen programs that can be displayed on interactive screens.
 * Programs should be lightweight and report back when their task is completed.
 */
/**
 * Pointer input payload passed to screen programs. Positions are already normalized to the
 * program content area; the host may provide multiple pressed keys for combo interactions.
 */
struct ELEVATOR_API FScreenPointerEvent
{
    FScreenPointerEvent() = default;

    FKey TriggerKey = EKeys::Invalid;
    TArray<FKey> PressedKeys;
    FVector2D ProgramNormalizedPosition = FVector2D::ZeroVector;
    FVector2D ProgramPixelPosition = FVector2D::ZeroVector;
    FVector2D ScreenPixelPosition = FVector2D::ZeroVector;
    int32 PointerIndex = 0;

    bool IsButtonPressed(const FKey& Key) const
    {
        return PressedKeys.Contains(Key);
    }

    bool HasAnyButtonPressed() const
    {
        return PressedKeys.Num() > 0;
    }
};

class ELEVATOR_API IScreenProgram
{
public:
    virtual ~IScreenProgram() = default;

    /**
     * Create the Slate widget for this program.
     * @param Size The size of the screen in pixels
     * @return The root widget for this program
     */
    virtual TSharedRef<SWidget> CreateWidget(const FVector2D& Size) = 0;

    /**
     * Called whenever the pointer moves inside the program content area.
     * @param Event Pointer description containing normalized and pixel positions plus pressed keys
     */
    virtual void OnPointerMoved(const FScreenPointerEvent& Event) = 0;

    /**
     * Called when a pointer button is pressed while hovering the program.
     * @param Event Pointer description for the press event
     */
    virtual void OnPointerPressed(const FScreenPointerEvent& Event) = 0;

    /**
     * Called when a pointer button is released.
     * @param Event Pointer description for the release event
     */
    virtual void OnPointerReleased(const FScreenPointerEvent& Event) = 0;

    /**
     * Check if the program's task is complete.
     * @return True if the task is complete
     */
    virtual bool IsTaskComplete() const = 0;

    /**
     * Provide the cursor style best representing the current hover/interaction state.
     */
    virtual EMouseCursor::Type GetCursorType() const { return EMouseCursor::Default; }

    /**
     * Called when the screen size changes (e.g., render target resize).
     * @param NewSize The new screen size in pixels
     */
    virtual void OnScreenResized(const FVector2D& NewSize) {}

    /**
     * Called when the program is activated/becomes visible.
     */
    virtual void OnActivated() {}

    /**
     * Called when the program is deactivated/hidden.
     */
    virtual void OnDeactivated() {}
};
