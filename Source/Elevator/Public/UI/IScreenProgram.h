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
 * Style parameters for screen programs
 */
struct ELEVATOR_API FScreenProgramStyle
{
    /** Thickness of lines (borders, dividers) in pixels */
    float LineThickness = 2.0f;
    
    /** Base font size for UI text */
    int32 TextSize = 16;

    FScreenProgramStyle() = default;
    FScreenProgramStyle(float InLineThickness, int32 InTextSize)
        : LineThickness(InLineThickness), TextSize(InTextSize) {}

    /** Get standard padding for general content areas */
    float GetPadding() const { return static_cast<float>(TextSize) * 0.75f + LineThickness; }

    /** Get small padding for tighter spacing */
    float GetSmallPadding() const { return static_cast<float>(TextSize) * 0.5f; }

    /** Get large padding for bigger gaps */
    float GetLargePadding() const { return static_cast<float>(TextSize) * 1.0f + LineThickness * 2.0f; }

    /** Get title bar height that accommodates text plus padding */
    float GetTitleBarHeight() const { return static_cast<float>(TextSize) + GetPadding() * 2.0f; }

    /** Get standard button height */
    float GetButtonHeight() const { return static_cast<float>(TextSize) + GetPadding() * 2.0f; }

    /** Get standard row height for lists/tables */
    float GetRowHeight() const { return static_cast<float>(TextSize) + GetSmallPadding() * 2.0f; }

    /** Get resize handle size */
    float GetResizeHandleSize() const { return GetPadding() * 2.0f; }

    /** Get content padding as margin */
    FMargin GetContentPadding() const { return FMargin(GetPadding()); }

    /** Get title bar padding */
    FMargin GetTitleBarPadding() const 
    { 
        const float HPad = GetPadding();
        const float VPad = GetSmallPadding();
        return FMargin(HPad, VPad, HPad, VPad); 
    }

    // -------------------------------------------------------------------------
    // Standard UI Colors - Use these for consistent styling across programs
    // -------------------------------------------------------------------------

    /** Primary color for borders, text, and active elements */
    FLinearColor GetPrimaryColor() const { return FLinearColor::Green; }

    /** Background color for content areas */
    FLinearColor GetBackgroundColor() const { return FLinearColor::Black; }

    /** Border color for selected elements */
    FSlateColor GetSelectedBorderColor() const { return FSlateColor(GetPrimaryColor()); }

    /** Border color for hovered elements */
    FSlateColor GetHoveredBorderColor() const { return FSlateColor(GetPrimaryColor() * 0.8f); }

    /** Border color for default/idle elements */
    FSlateColor GetDefaultBorderColor() const { return FSlateColor(GetPrimaryColor() * 0.5f); }

    /** Fill color for selected elements */
    FSlateColor GetSelectedFillColor() const { return FSlateColor(GetPrimaryColor()); }

    /** Fill color for hovered elements */
    FSlateColor GetHoveredFillColor() const { return FSlateColor(GetPrimaryColor()); }

    /** Fill color for default/idle elements (transparent) */
    FSlateColor GetDefaultFillColor() const { return FSlateColor(FLinearColor::Transparent); }

    /** Text color when element is selected/hovered (inverted for contrast) */
    FSlateColor GetActiveTextColor() const { return FSlateColor(GetBackgroundColor()); }

    /** Text color for default/idle state */
    FSlateColor GetDefaultTextColor() const { return FSlateColor(GetPrimaryColor()); }

    /** Dimmed text color for disabled or secondary elements */
    FSlateColor GetDimmedTextColor() const { return FSlateColor(GetPrimaryColor() * 0.5f); }

    // -------------------------------------------------------------------------
    // Color Convenience Methods - Return appropriate color based on state
    // -------------------------------------------------------------------------

    /** Get border color based on selection/hover state */
    FSlateColor GetBorderColorForState(bool bSelected, bool bHovered) const
    {
        if (bSelected) return GetSelectedBorderColor();
        if (bHovered) return GetHoveredBorderColor();
        return GetDefaultBorderColor();
    }

    /** Get fill color based on selection/hover state */
    FSlateColor GetFillColorForState(bool bSelected, bool bHovered) const
    {
        if (bSelected) return GetSelectedFillColor();
        if (bHovered) return GetHoveredFillColor();
        return GetDefaultFillColor();
    }

    /** Get text color based on selection/hover state */
    FSlateColor GetTextColorForState(bool bSelected, bool bHovered) const
    {
        if (bSelected || bHovered) return GetActiveTextColor();
        return GetDefaultTextColor();
    }
};

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
     * @param Style Style parameters including line thickness and text size
     * @return The root widget for this program
     */
    virtual TSharedRef<SWidget> CreateWidget(const FVector2D& Size, const FScreenProgramStyle& Style) = 0;

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
