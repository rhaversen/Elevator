#include "UI/SlateWidgetHelpers.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/CoreStyle.h"

namespace SlateWidgetHelpers
{
	TSharedRef<SWidget> CreateOutlinedButton(
		const FVector2D& Size,
		const FText& ButtonText,
		int32 FontSize,
		TAttribute<FSlateColor> BorderColorAttr,
		TAttribute<FSlateColor> FillColorAttr,
		TAttribute<FSlateColor> TextColorAttr)
	{
		const float BorderThickness = 2.0f;
		const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");

		return SNew(SBox)
			.WidthOverride(Size.X)
			.HeightOverride(Size.Y)
			[
				SNew(SOverlay)
				// Background fill
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SImage)
					.Image(Brush)
					.ColorAndOpacity(FillColorAttr)
				]
				// Top border
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.HeightOverride(BorderThickness)
					[
						SNew(SImage)
						.Image(Brush)
						.ColorAndOpacity(BorderColorAttr)
					]
				]
				// Bottom border
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Bottom)
				[
					SNew(SBox)
					.HeightOverride(BorderThickness)
					[
						SNew(SImage)
						.Image(Brush)
						.ColorAndOpacity(BorderColorAttr)
					]
				]
				// Left border
				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Fill)
				[
					SNew(SBox)
					.WidthOverride(BorderThickness)
					[
						SNew(SImage)
						.Image(Brush)
						.ColorAndOpacity(BorderColorAttr)
					]
				]
				// Right border
				+ SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Fill)
				[
					SNew(SBox)
					.WidthOverride(BorderThickness)
					[
						SNew(SImage)
						.Image(Brush)
						.ColorAndOpacity(BorderColorAttr)
					]
				]
				// Centered text
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(ButtonText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", FontSize))
					.ColorAndOpacity(TextColorAttr)
				]
			];
	}

	TSharedRef<SWidget> CreateColoredBorder(
		float BorderThickness,
		const FLinearColor& BorderColor,
		const FLinearColor& FillColor,
		TSharedRef<SWidget> Content)
	{
		const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");

		return SNew(SBorder)
			.BorderImage(Brush)
			.BorderBackgroundColor(BorderColor)
			.Padding(BorderThickness)
			[
				SNew(SBorder)
				.BorderImage(Brush)
				.BorderBackgroundColor(FillColor)
				.Padding(0.0f)
				[
					Content
				]
			];
	}
}
