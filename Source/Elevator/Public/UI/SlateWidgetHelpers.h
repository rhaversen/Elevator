#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Styling/SlateColor.h"

/**
 * Helper library for creating reusable Slate widget components
 * Provides parameterized factory functions for common UI patterns
 */
namespace SlateWidgetHelpers
{
	/**
	 * Creates a button with customizable outlined border and centered text
	 * @param Size - The fixed size of the button
	 * @param ButtonText - Text to display in the center of the button
	 * @param FontSize - Size of the font for the button text
	 * @param BorderThickness - Thickness of the border in pixels
	 * @param BorderColorAttr - Attribute for the border color (can be dynamic)
	 * @param FillColorAttr - Attribute for the background fill color (can be dynamic)
	 * @param TextColorAttr - Attribute for the text color (can be dynamic)
	 * @return A fully constructed outlined button widget
	 */
	ELEVATOR_API TSharedRef<SWidget> CreateOutlinedButton(
		const FVector2D& Size,
		const FText& ButtonText,
		int32 FontSize,
		float BorderThickness,
		TAttribute<FSlateColor> BorderColorAttr,
		TAttribute<FSlateColor> FillColorAttr,
		TAttribute<FSlateColor> TextColorAttr);

	/**
	 * Creates a simple colored border box
	 * @param BorderThickness - Thickness of the border in pixels
	 * @param BorderColor - Color of the border
	 * @param FillColor - Color of the interior fill
	 * @param Content - The widget to place inside the border
	 * @return A bordered widget
	 */
	ELEVATOR_API TSharedRef<SWidget> CreateColoredBorder(
		float BorderThickness,
		const FLinearColor& BorderColor,
		const FLinearColor& FillColor,
		TSharedRef<SWidget> Content);
}
