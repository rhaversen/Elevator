#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Interface for screen programs that can be displayed on interactive screens.
 * Programs should be lightweight and report back when their task is completed.
 */
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
     * Called when the cursor position changes.
     * @param NormalizedPosition Cursor position in normalized 0-1 coordinates
     */
    virtual void UpdateCursor(const FVector2D& NormalizedPosition) = 0;

    /**
     * Called when a click occurs at the current cursor position.
     * @return True if the click was handled and requires a render update
     */
    virtual bool HandleClick() = 0;

    /**
     * Check if the program's task is complete.
     * @return True if the task is complete
     */
    virtual bool IsTaskComplete() const = 0;

    /**
     * Check if the program requests to exit the workstation.
     * @return True if the user should exit the workstation
     */
    virtual bool ShouldExit() const { return false; }

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
