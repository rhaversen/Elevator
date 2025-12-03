#include "UI/Programs/DailyPacketProgram.h"
#include "UI/ScreenProgramIds.h"
#include "UI/ScreenProgramMacros.h"
#include "UI/SlateWidgetHelpers.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SCanvas.h"

REGISTER_SCREEN_PROGRAM(FDailyPacketProgram, "DailyPacket")

namespace
{
    // Utility to check whether a widget is under the absolute screen position reported by the pointer event
    bool IsWidgetUnderPointer(const TWeakPtr<SWidget>& Widget, const FVector2D& ScreenPosition)
    {
        if (const TSharedPtr<SWidget> Pinned = Widget.Pin())
        {
            return Pinned->GetCachedGeometry().IsUnderLocation(ScreenPosition);
        }
        return false;
    }

    TSharedRef<SWidget> BuildPanelHeaderWidget(const FScreenProgramStyle& Style, FText Label)
    {
        return SNew(SBorder)
            .BorderBackgroundColor(Style.GetPrimaryColor())
            .Padding(Style.GetSmallPadding())
            [
                SNew(STextBlock)
                .Text(Label)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", Style.TextSize))
                .ColorAndOpacity(FSlateColor(Style.GetPrimaryColor()))
            ];
    }

    TSharedRef<SWidget> BuildTemplateSectionLabel(const FScreenProgramStyle& Style, FText Label)
    {
        return SNew(STextBlock)
            .Text(Label)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", Style.TextSize - 2))
            .ColorAndOpacity(Style.GetPrimaryColor());
    }
}

FDailyPacketProgram::FDailyPacketProgram()
    : CurrentDay(1)
{
    InitializePoolItems();
    InitializeTemplateSlots();
    InitializeActionButtons();
    LastInteractionTime = FPlatformTime::Seconds();
}

FDailyPacketProgram::FDailyPacketProgram(int32 InDay)
    : CurrentDay(FMath::Clamp(InDay, 1, 5))
{
    InitializePoolItems();
    InitializeTemplateSlots();
    InitializeActionButtons();
    LastInteractionTime = FPlatformTime::Seconds();
}

void FDailyPacketProgram::InitializePoolItems()
{
    // Day determines how many items to show (1-5, capped)
    const int32 ItemCount = FMath::Clamp(CurrentDay, 1, 5);
    
    // Headlines
    Categories.Add({TEXT("Headlines"), FText::FromString(TEXT("Headline")), true, FBox2D()});
    for (int32 i = 1; i <= ItemCount; ++i)
    {
        FPoolItem Item;
        Item.ItemId = *FString::Printf(TEXT("Headline-%d"), i);
        Item.DisplayText = FText::FromString(FString::Printf(TEXT("Headline %d"), i));
        Item.Category = TEXT("Headlines");
        PoolItems.Add(Item);
    }
    
    // Content
    Categories.Add({TEXT("Content"), FText::FromString(TEXT("Content")), true, FBox2D()});
    for (int32 i = 1; i <= ItemCount; ++i)
    {
        FPoolItem Item;
        Item.ItemId = *FString::Printf(TEXT("Content-%d"), i);
        Item.DisplayText = FText::FromString(FString::Printf(TEXT("Content %d"), i));
        Item.Category = TEXT("Content");
        PoolItems.Add(Item);
    }
    
    // Charts
    Categories.Add({TEXT("Charts"), FText::FromString(TEXT("Chart")), true, FBox2D()});
    for (int32 i = 1; i <= ItemCount; ++i)
    {
        FPoolItem Item;
        Item.ItemId = *FString::Printf(TEXT("Chart-%d"), i);
        Item.DisplayText = FText::FromString(FString::Printf(TEXT("Chart %d"), i));
        Item.Category = TEXT("Charts");
        PoolItems.Add(Item);
    }
    
    // Notes
    Categories.Add({TEXT("Notes"), FText::FromString(TEXT("Note")), true, FBox2D()});
    for (int32 i = 1; i <= ItemCount; ++i)
    {
        FPoolItem Item;
        Item.ItemId = *FString::Printf(TEXT("Note-%d"), i);
        Item.DisplayText = FText::FromString(FString::Printf(TEXT("Note %d"), i));
        Item.Category = TEXT("Notes");
        PoolItems.Add(Item);
    }
}

void FDailyPacketProgram::InitializeTemplateSlots()
{
    TemplateSlots.Add({TEXT("Headline"), FText::FromString(TEXT("Headline")), TEXT("Headlines"), NAME_None, FBox2D(), false});
    TemplateSlots.Add({TEXT("Content"), FText::FromString(TEXT("Content")), TEXT("Content"), NAME_None, FBox2D(), false});
    TemplateSlots.Add({TEXT("Chart"), FText::FromString(TEXT("Chart")), TEXT("Charts"), NAME_None, FBox2D(), false});
    TemplateSlots.Add({TEXT("Note"), FText::FromString(TEXT("Note")), TEXT("Notes"), NAME_None, FBox2D(), false});
}

void FDailyPacketProgram::InitializeActionButtons()
{
    ActionButtons.Add({TEXT("Preview"), FText::FromString(TEXT("Preview")), false, false, false, false, 0.4f, 0.0f, FBox2D()});
    ActionButtons.Add({TEXT("Export"), FText::FromString(TEXT("Export")), false, false, false, false, 0.6f, 0.0f, FBox2D()});
    ActionButtons.Add({TEXT("Send"), FText::FromString(TEXT("Send")), false, false, false, false, 0.8f, 0.0f, FBox2D()});
}

TSharedRef<SWidget> FDailyPacketProgram::BuildProgramWidget()
{
    const FScreenProgramStyle& Style = GetProgramStyle();

    // Reset cached widgets so we capture fresh geometry on rebuild
    for (FPoolItem& Item : PoolItems)
    {
        Item.Widget.Reset();
    }
    for (FCategoryState& Category : Categories)
    {
        Category.HeaderWidget.Reset();
    }
    for (FTemplateSlot& Slot : TemplateSlots)
    {
        Slot.Widget.Reset();
    }
    for (FActionButton& Button : ActionButtons)
    {
        Button.Widget.Reset();
    }
    RootOverlayWidget.Reset();
    
    // Main layout: pool on left, template+actions column on right
    TSharedRef<SHorizontalBox> MainContent = SNew(SHorizontalBox)
        // Left Panel: Item Pool
        + SHorizontalBox::Slot()
        .FillWidth(0.3f)
        .Padding(0, 0, Style.GetSmallPadding(), 0)
        [
            BuildItemPoolPanel()
        ]
        // Right Column: Template above, Actions below
        + SHorizontalBox::Slot()
        .FillWidth(0.7f)
        .Padding(Style.GetSmallPadding(), 0, 0, 0)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                BuildTemplatePanel()
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, Style.GetSmallPadding(), 0, 0)
            [
                BuildActionsPanel()
            ]
        ];

    TSharedPtr<SOverlay> ProgramOverlay;

    TSharedRef<SWidget> RootWidgetRef = SAssignNew(ProgramOverlay, SOverlay)
        + SOverlay::Slot()
        [
            SNew(SBox)
            .Padding(Style.GetSmallPadding())
            [
                MainContent
            ]
        ]
        + SOverlay::Slot()
        .ZOrder(10)
        [
            BuildDragGhostLayer()
        ];

    RootOverlayWidget = ProgramOverlay;
    return RootWidgetRef;
}

TSharedRef<SWidget> FDailyPacketProgram::BuildItemPoolPanel()
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    
    TSharedRef<SVerticalBox> PoolBox = SNew(SVerticalBox);
    
    // Panel header
    PoolBox->AddSlot()
        .AutoHeight()
        .Padding(0, 0, 0, Style.GetSmallPadding())
        [
            BuildPanelHeaderWidget(Style, FText::FromString(TEXT("POOL")))
        ];
    
    auto AddCategorySection = [&](FCategoryState& Category)
    {
        TSharedPtr<SBorder> CategoryBorder;
        PoolBox->AddSlot()
            .AutoHeight()
            .Padding(0, Style.GetSmallPadding() * 0.5f)
            [
                SAssignNew(CategoryBorder, SBorder)
                .BorderBackgroundColor(GetCategoryHeaderColor(Category))
                .Padding(FMargin(Style.GetSmallPadding(), 2.0f))
                [
                    SNew(STextBlock)
                    .Text(Category.DisplayName)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", Style.TextSize - 2))
                    .ColorAndOpacity(FSlateColor(Style.GetPrimaryColor()))
                ]
            ];
        Category.HeaderWidget = CategoryBorder;

        for (FPoolItem& Item : PoolItems)
        {
            if (Item.Category != Category.Category)
            {
                continue;
            }

            TSharedPtr<SBorder> ItemBorder;
            PoolBox->AddSlot()
                .AutoHeight()
                .Padding(Style.GetSmallPadding(), 4.0f, 0, 4.0f)
                [
                    SAssignNew(ItemBorder, SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("Border"))
                    .BorderBackgroundColor_Lambda([this, ItemId = Item.ItemId]() { return GetPoolItemColorById(ItemId); })
                    .Padding(FMargin(Style.LineThickness))
                    [
                        SNew(SBorder)
                        .BorderBackgroundColor_Lambda([this, ItemId = Item.ItemId]() { return GetPoolItemFillColorById(ItemId); })
                        .Padding(FMargin(Style.GetSmallPadding(), 2.0f))
                        [
                            SNew(STextBlock)
                            .Text(Item.DisplayText)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", Style.TextSize - 2))
                            .ColorAndOpacity_Lambda([this, ItemId = Item.ItemId]() { return GetPoolItemTextColorById(ItemId); })
                        ]
                    ]
                ];
            Item.Widget = ItemBorder;
        }
    };

    // Categories with static, always-expanded items
    for (FCategoryState& Category : Categories)
    {
        AddCategorySection(Category);
    }
    
    return PoolBox;
}

TSharedRef<SWidget> FDailyPacketProgram::BuildTemplatePanel()
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const float SlotHeight = Style.TextSize * 2.5f;
    
    TSharedRef<SVerticalBox> TemplateBox = SNew(SVerticalBox);
    
    // Panel header
    TemplateBox->AddSlot()
        .AutoHeight()
        .Padding(0, 0, 0, Style.GetSmallPadding())
        [
            BuildPanelHeaderWidget(Style, FText::FromString(TEXT("TEMPLATE")))
        ];

    auto AddSectionLabel = [&](const FText& Label)
    {
        TemplateBox->AddSlot()
            .AutoHeight()
            .Padding(0, Style.GetSmallPadding())
            [
                BuildTemplateSectionLabel(Style, Label)
            ];
    };

    auto AddSlotRow = [&](FTemplateSlot& Slot, float Height)
    {
        TSharedPtr<SBorder> SlotBorder;
        TemplateBox->AddSlot()
            .AutoHeight()
            .Padding(0, 2.0f)
            [
                SNew(SBox)
                .HeightOverride(Height)
                [
                    SAssignNew(SlotBorder, SBorder)
                    .BorderBackgroundColor_Lambda([this, SlotId = Slot.SlotId]() { return GetSlotBorderColorById(SlotId); })
                    .Padding(Style.LineThickness)
                    [
                        SNew(SBorder)
                        .BorderBackgroundColor_Lambda([this, SlotId = Slot.SlotId]() { return GetSlotFillColorById(SlotId); })
                        .Padding(Style.GetSmallPadding())
                        .HAlign(HAlign_Left)
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this, SlotId = Slot.SlotId]() { return GetSlotTextById(SlotId); })
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", Style.TextSize - 2))
                            .ColorAndOpacity_Lambda([this, SlotId = Slot.SlotId]() { return GetSlotTextColorById(SlotId); })
                        ]
                    ]
                ]
            ];
        Slot.Widget = SlotBorder;
    };

    AddSectionLabel(FText::FromString(TEXT("Headline")));
    AddSlotRow(TemplateSlots[0], SlotHeight);

    AddSectionLabel(FText::FromString(TEXT("Content")));
    AddSlotRow(TemplateSlots[1], SlotHeight);

    AddSectionLabel(FText::FromString(TEXT("Chart")));
    AddSlotRow(TemplateSlots[2], SlotHeight);

    AddSectionLabel(FText::FromString(TEXT("Note")));
    AddSlotRow(TemplateSlots[3], SlotHeight);
    
    return TemplateBox;
}

TSharedRef<SWidget> FDailyPacketProgram::BuildActionsPanel()
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const float ButtonWidth = Style.TextSize * 6.0f;
    const float ButtonHeight = Style.GetButtonHeight();
    
    TSharedRef<SHorizontalBox> ActionsRow = SNew(SHorizontalBox);
    
    auto AddActionButton = [&](FActionButton& Button)
    {
        TSharedPtr<SBorder> ButtonBorder;
        ActionsRow->AddSlot()
            .AutoWidth()
            .Padding(Style.GetSmallPadding(), 0)
            [
                SNew(SBox)
                .WidthOverride(ButtonWidth)
                .HeightOverride(ButtonHeight)
                [
                    SAssignNew(ButtonBorder, SBorder)
                    .BorderBackgroundColor_Lambda([this, ButtonId = Button.ButtonId]() { return GetActionButtonBorderColorById(ButtonId); })
                    .Padding(Style.LineThickness)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()
                        [
                            SNew(SBorder)
                            .BorderBackgroundColor_Lambda([this, ButtonId = Button.ButtonId]() { return GetActionButtonFillColorById(ButtonId); })
                        ]
                        + SOverlay::Slot()
                        [
                            SNew(SBox)
                            .WidthOverride_Lambda([this, ButtonId = Button.ButtonId, ButtonWidth]() { return GetActionButtonProgressWidthById(ButtonId, ButtonWidth); })
                            .HAlign(HAlign_Left)
                            [
                                SNew(SBorder)
                                .BorderBackgroundColor_Lambda([this, ButtonId = Button.ButtonId]() { return GetActionButtonProgressColorById(ButtonId); })
                            ]
                        ]
                        + SOverlay::Slot()
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this, ButtonId = Button.ButtonId]() { return GetActionButtonTextById(ButtonId); })
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", Style.TextSize - 2))
                            .ColorAndOpacity_Lambda([this, ButtonId = Button.ButtonId]() { return GetActionButtonTextColorById(ButtonId); })
                        ]
                    ]
                ]
            ];
        Button.Widget = ButtonBorder;
    };

    for (FActionButton& Button : ActionButtons)
    {
        AddActionButton(Button);
    }
    
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        [
            SNullWidget::NullWidget
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            ActionsRow
        ];
}

TSharedRef<SWidget> FDailyPacketProgram::BuildDragGhostLayer()
{
    return SNew(SCanvas)
        .Visibility_Lambda([this]()
        {
            return DragGhost.bVisible ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
        })
        + SCanvas::Slot()
        .Position_Lambda([this]()
        {
            return DragGhost.Position;
        })
        .Size_Lambda([this]()
        {
            return DragGhost.Size;
        })
        [
            SNew(SBorder)
            .BorderBackgroundColor_Lambda([this]()
            {
                return FSlateColor(DragGhost.BorderColor);
            })
            .BorderImage(FCoreStyle::Get().GetBrush("Border"))
            .Padding(GetProgramStyle().LineThickness)
            [
                SNew(SBorder)
                .BorderBackgroundColor_Lambda([this]()
                {
                    return FSlateColor(DragGhost.FillColor);
                })
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .Padding(GetProgramStyle().GetSmallPadding())
                .HAlign(HAlign_Left)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]()
                    {
                        return DragGhost.Label;
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", GetProgramStyle().TextSize - 2))
                    .ColorAndOpacity_Lambda([this]()
                    {
                        return FSlateColor(DragGhost.TextColor);
                    })
                ]
            ]
        ];
}

void FDailyPacketProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    ResetIdleTimer();
    
    if (bHandledByPreProcessor || GetTaskComplete())
    {
        ClearSlotHighlights();
        HoverItemId = NAME_None;
        UpdateCursorVisual();
        return;
    }
    
    if (DragState.bIsDragging)
    {
        UpdateDrag(Event);
        UpdateSlotHighlights(Event);
        UpdateDragGhost(Event);
    }
    else
    {
        ClearSlotHighlights();
        HoverItemId = NAME_None;

        // Update hover states for buttons
        for (FActionButton& Button : ActionButtons)
        {
            Button.bHovered = Button.bEnabled && IsWidgetUnderPointer(Button.Widget, Event.ScreenPixelPosition);
        }

        if (FPoolItem* HoveredItem = GetPoolItemAtPosition(Event))
        {
            HoverItemId = HoveredItem->bUsed ? NAME_None : HoveredItem->ItemId;
        }
    }
    
    UpdateCursorVisual();
}

void FDailyPacketProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    ResetIdleTimer();
    
    if (bHandledByPreProcessor || GetTaskComplete())
    {
        return;
    }
    
    ActivePointerKey = Event.TriggerKey;
    
    // Check pool items for drag start
    FPoolItem* ClickedItem = GetPoolItemAtPosition(Event);
    if (ClickedItem && !ClickedItem->bUsed)
    {
        HoverItemId = ClickedItem->ItemId;
        BeginDrag(ClickedItem, Event.ProgramPixelPosition);
        OnPoolItemOpened.Broadcast(ClickedItem->Category);
        UpdateSlotHighlights(Event);
        UpdateDragGhost(Event);
        return;
    }
    
    // Check action buttons
    FActionButton* ClickedButton = GetActionButtonAtPosition(Event);
    if (ClickedButton && ClickedButton->bEnabled)
    {
        ClickedButton->bPressed = true;
    }
    
    UpdateCursorVisual();
}

void FDailyPacketProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    ResetIdleTimer();
    
    if (bHandledByPreProcessor || GetTaskComplete())
    {
        if (DragState.bIsDragging)
        {
            DragState.bIsDragging = false;
            ClearSlotHighlights();
        }
        ActivePointerKey = EKeys::Invalid;
        HideDragGhost();
        UpdateCursorVisual();
        return;
    }
    
    // End drag if active
    if (DragState.bIsDragging)
    {
        EndDrag(Event);
        ClearSlotHighlights();
    }
    
    // Check action buttons
    for (FActionButton& Button : ActionButtons)
    {
        if (Button.bPressed && Button.bEnabled && IsWidgetUnderPointer(Button.Widget, Event.ScreenPixelPosition))
        {
            OnActionButtonClicked(&Button);
        }
        Button.bPressed = false;
    }
    
    ActivePointerKey = EKeys::Invalid;
    HideDragGhost();
    UpdateCursorVisual();
}

void FDailyPacketProgram::HandleScreenResized(const FVector2D& NewSize)
{
    // Geometry is captured directly from Slate widgets, so nothing extra required here
}

void FDailyPacketProgram::HandleTaskCompletionChanged(bool bCompleted)
{
    if (bCompleted)
    {
        OnWorkspaceClosed.Broadcast();
    }
    UpdateCursorVisual();
}

void FDailyPacketProgram::BeginDrag(FPoolItem* Item, const FVector2D& Position)
{
    if (!Item) return;
    
    DragState.bIsDragging = true;
    DragState.DraggedItemId = Item->ItemId;
    DragState.DragOffset = FVector2D::ZeroVector;
    DragState.CurrentPosition = Position;
    HoverItemId = Item->ItemId;
    DragGhost.bVisible = true;
    DragGhost.Label = Item->DisplayText;
    DragGhost.Size = GetGhostDefaultSize();
    DragGhost.Position = Position - DragGhost.Size * 0.5f;
    DragGhost.BorderColor = GetProgramStyle().GetPrimaryColor();
    DragGhost.FillColor = GetProgramStyle().GetBackgroundColor();
    DragGhost.TextColor = GetProgramStyle().GetPrimaryColor();
    DragGhost.bSnapped = false;
    HoverSlotId = NAME_None;
    
    if (!bFirstDragOccurred)
    {
        bFirstDragOccurred = true;
        OnFirstDrag.Broadcast();
    }
}

void FDailyPacketProgram::UpdateDrag(const FScreenPointerEvent& Event)
{
    DragState.CurrentPosition = Event.ProgramPixelPosition;
    DragGhost.bVisible = true;
    HoverItemId = DragState.DraggedItemId;
}

void FDailyPacketProgram::EndDrag(const FScreenPointerEvent& Event)
{
    if (!DragState.bIsDragging) return;
    
    // Find target slot
    FTemplateSlot* TargetSlot = GetTemplateSlotAtPosition(Event);
    
    if (TargetSlot)
    {
        if (TryDropOnSlot(TargetSlot, DragState.DraggedItemId))
        {
            // Successful drop - mark item as used
            for (FPoolItem& Item : PoolItems)
            {
                if (Item.ItemId == DragState.DraggedItemId)
                {
                    Item.bUsed = true;
                    break;
                }
            }
            
            // Fire trigger for Chart
            if (TargetSlot->SlotId == TEXT("Chart"))
            {
                OnMiniChartDropped.Broadcast();
            }
            else if (TargetSlot->SlotId == TEXT("Note"))
            {
                OnFooterFocused.Broadcast();
            }
            
            UpdateActionButtonStates();
        }
        else
        {
            TriggerInvalidDropFeedback();
        }
    }
    else
    {
        TriggerInvalidDropFeedback();
    }
    
    DragState.bIsDragging = false;
    DragState.DraggedItemId = NAME_None;
    HideDragGhost();
    HoverItemId = NAME_None;
}

bool FDailyPacketProgram::TryDropOnSlot(FTemplateSlot* Slot, const FName& ItemId)
{
    if (!Slot || ItemId.IsNone()) return false;

    // Find item to check category
    const FPoolItem* Item = FindPoolItemById(ItemId);
    if (!Item) return false;

    // Check if category matches
    if (!Slot->AcceptedCategory.IsNone() && Slot->AcceptedCategory != Item->Category)
    {
        return false;
    }

    if (!Slot->AssignedItemId.IsNone() && Slot->AssignedItemId != ItemId)
    {
        // Release previously assigned item back to the pool
        for (FPoolItem& PoolIt : PoolItems)
        {
            if (PoolIt.ItemId == Slot->AssignedItemId)
            {
                PoolIt.bUsed = false;
                break;
            }
        }
    }

    Slot->AssignedItemId = ItemId;
    return true;
}

void FDailyPacketProgram::TriggerInvalidDropFeedback()
{
    // Start shake animation (300ms)
    DragState.InvalidDropShakeTime = 0.3f;
}

void FDailyPacketProgram::OnActionButtonClicked(FActionButton* Button)
{
    if (!Button || !Button->bEnabled || Button->bCompleted) return;
    
    // Fire appropriate trigger
    if (Button->ButtonId == TEXT("Preview"))
    {
        OnPreviewClicked.Broadcast();
        Button->bCompleted = true;
    }
    else if (Button->ButtonId == TEXT("Export"))
    {
        OnExportClicked.Broadcast();
        Button->bCompleted = true;
    }
    else if (Button->ButtonId == TEXT("Send"))
    {
        OnSendClicked.Broadcast();
        Button->bCompleted = true;
        SetTaskComplete(true);
    }
    
    UpdateActionButtonStates();
}

void FDailyPacketProgram::UpdateActionButtonStates()
{
    bool bTemplateComplete = IsTemplateComplete();
    
    // Preview: enabled when template is complete
    ActionButtons[0].bEnabled = bTemplateComplete && !ActionButtons[0].bCompleted;
    
    // Export: enabled after Preview
    ActionButtons[1].bEnabled = ActionButtons[0].bCompleted && !ActionButtons[1].bCompleted;
    
    // Send: enabled after Export
    ActionButtons[2].bEnabled = ActionButtons[1].bCompleted && !ActionButtons[2].bCompleted;
}

bool FDailyPacketProgram::IsTemplateComplete() const
{
    for (const FTemplateSlot& Slot : TemplateSlots)
    {
        if (Slot.AssignedItemId.IsNone())
        {
            return false;
        }
    }
    return true;
}

bool FDailyPacketProgram::CanPreview() const
{
    return IsTemplateComplete() && !ActionButtons[0].bCompleted;
}

bool FDailyPacketProgram::CanExport() const
{
    return ActionButtons[0].bCompleted && !ActionButtons[1].bCompleted;
}

bool FDailyPacketProgram::CanSend() const
{
    return ActionButtons[1].bCompleted && !ActionButtons[2].bCompleted;
}

void FDailyPacketProgram::UpdateSlotHighlights(const FScreenPointerEvent& Event)
{
    if (!DragState.bIsDragging)
    {
        ClearSlotHighlights();
        return;
    }
    
    const FPoolItem* DraggedItem = FindPoolItemById(DragState.DraggedItemId);
    const FName DragCategory = DraggedItem ? DraggedItem->Category : NAME_None;

    HoverSlotId = NAME_None;

    for (FTemplateSlot& Slot : TemplateSlots)
    {
        const bool bAcceptsCategory = Slot.AcceptedCategory.IsNone() || Slot.AcceptedCategory == DragCategory;
        const bool bAllowsDrop = bAcceptsCategory;
        Slot.bHighlighted = DragState.bIsDragging && bAllowsDrop;

        if (bAllowsDrop && IsWidgetUnderPointer(Slot.Widget, Event.ScreenPixelPosition))
        {
            HoverSlotId = Slot.SlotId;
        }
    }
}

void FDailyPacketProgram::ClearSlotHighlights()
{
    for (FTemplateSlot& Slot : TemplateSlots)
    {
        Slot.bHighlighted = false;
    }
    HoverSlotId = NAME_None;
}

void FDailyPacketProgram::UpdateDragGhost(const FScreenPointerEvent& Event)
{
    if (!DragState.bIsDragging)
    {
        HideDragGhost();
        return;
    }

    DragGhost.bVisible = true;

    const FScreenProgramStyle& Style = GetProgramStyle();
    if (const FPoolItem* DraggedItem = FindPoolItemById(DragState.DraggedItemId))
    {
        DragGhost.Label = DraggedItem->DisplayText;
    }
    else
    {
        DragGhost.Label = FText::FromName(DragState.DraggedItemId);
    }

    TSharedPtr<SOverlay> Root = RootOverlayWidget.Pin();
    FVector2D DefaultSize = GetGhostDefaultSize();
    FVector2D DesiredPosition = DragState.CurrentPosition - DefaultSize * 0.5f;
    DragGhost.Size = DefaultSize;
    DragGhost.bSnapped = false;

    if (Root.IsValid())
    {
        const FGeometry& RootGeometry = Root->GetCachedGeometry();
        DesiredPosition = RootGeometry.AbsoluteToLocal(Event.ScreenPixelPosition) - DefaultSize * 0.5f;

        if (!HoverSlotId.IsNone())
        {
            if (FTemplateSlot* Slot = FindTemplateSlotById(HoverSlotId))
            {
                if (TSharedPtr<SWidget> SlotWidget = Slot->Widget.Pin())
                {
                    const FGeometry& SlotGeometry = SlotWidget->GetCachedGeometry();
                    FVector2D SlotLocalPos = RootGeometry.AbsoluteToLocal(SlotGeometry.GetAbsolutePosition());
                    FVector2D SlotSize = SlotGeometry.GetLocalSize();
                    if (SlotSize.GetMin() > 0.0f)
                    {
                        DragGhost.Size = SlotSize;
                    }
                    DesiredPosition = SlotLocalPos;
                    DragGhost.bSnapped = true;
                }
            }
        }

        const FVector2D RootSize = RootGeometry.GetLocalSize();
        DesiredPosition.X = FMath::Clamp(DesiredPosition.X, 0.0f, RootSize.X - DragGhost.Size.X);
        DesiredPosition.Y = FMath::Clamp(DesiredPosition.Y, 0.0f, RootSize.Y - DragGhost.Size.Y);
    }

    DragGhost.Position = DesiredPosition;

    FLinearColor GhostFill = Style.GetBackgroundColor();
    GhostFill.A = 1.0f;

    DragGhost.FillColor = GhostFill;
    DragGhost.TextColor = Style.GetPrimaryColor();

    if (DragGhost.bSnapped)
    {
        DragGhost.BorderColor = Style.GetHighlightColor().GetSpecifiedColor();
    }
    else
    {
        DragGhost.BorderColor = Style.GetPrimaryColor();
    }
}

void FDailyPacketProgram::HideDragGhost()
{
    DragGhost.bVisible = false;
    DragGhost.bSnapped = false;
}

FVector2D FDailyPacketProgram::GetGhostDefaultSize() const
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    return FVector2D(Style.TextSize * 7.0f, Style.GetRowHeight() + Style.GetSmallPadding());
}

const FDailyPacketProgram::FPoolItem* FDailyPacketProgram::FindPoolItemById(FName ItemId) const
{
    if (ItemId.IsNone())
    {
        return nullptr;
    }

    for (const FPoolItem& Item : PoolItems)
    {
        if (Item.ItemId == ItemId)
        {
            return &Item;
        }
    }
    return nullptr;
}

FDailyPacketProgram::FTemplateSlot* FDailyPacketProgram::FindTemplateSlotById(FName SlotId)
{
    return const_cast<FTemplateSlot*>(static_cast<const FDailyPacketProgram*>(this)->FindTemplateSlotById(SlotId));
}

const FDailyPacketProgram::FTemplateSlot* FDailyPacketProgram::FindTemplateSlotById(FName SlotId) const
{
    if (SlotId.IsNone())
    {
        return nullptr;
    }

    for (const FTemplateSlot& Slot : TemplateSlots)
    {
        if (Slot.SlotId == SlotId)
        {
            return &Slot;
        }
    }
    return nullptr;
}

FDailyPacketProgram::FActionButton* FDailyPacketProgram::FindActionButtonById(FName ButtonId)
{
    return const_cast<FActionButton*>(static_cast<const FDailyPacketProgram*>(this)->FindActionButtonById(ButtonId));
}

const FDailyPacketProgram::FActionButton* FDailyPacketProgram::FindActionButtonById(FName ButtonId) const
{
    if (ButtonId.IsNone())
    {
        return nullptr;
    }

    for (const FActionButton& Button : ActionButtons)
    {
        if (Button.ButtonId == ButtonId)
        {
            return &Button;
        }
    }
    return nullptr;
}

FDailyPacketProgram::FPoolItem* FDailyPacketProgram::GetPoolItemAtPosition(const FScreenPointerEvent& Event)
{
    for (FPoolItem& Item : PoolItems)
    {
        if (IsWidgetUnderPointer(Item.Widget, Event.ScreenPixelPosition))
        {
            return &Item;
        }
    }
    return nullptr;
}

FDailyPacketProgram::FTemplateSlot* FDailyPacketProgram::GetTemplateSlotAtPosition(const FScreenPointerEvent& Event)
{
    for (FTemplateSlot& Slot : TemplateSlots)
    {
        if (IsWidgetUnderPointer(Slot.Widget, Event.ScreenPixelPosition))
        {
            return &Slot;
        }
    }
    return nullptr;
}

FDailyPacketProgram::FActionButton* FDailyPacketProgram::GetActionButtonAtPosition(const FScreenPointerEvent& Event)
{
    for (FActionButton& Button : ActionButtons)
    {
        if (IsWidgetUnderPointer(Button.Widget, Event.ScreenPixelPosition))
        {
            return &Button;
        }
    }
    return nullptr;
}

void FDailyPacketProgram::UpdateCursorVisual()
{
    bool bPressed = false;
    bool bHovered = false;
    
    if (DragState.bIsDragging)
    {
        bPressed = true;
        bHovered = true;
    }
    else
    {
        for (const FActionButton& Button : ActionButtons)
        {
            if (Button.bPressed) bPressed = true;
            if (Button.bHovered) bHovered = true;
        }

        if (!HoverItemId.IsNone())
        {
            bHovered = true;
        }
    }
    
    UpdateCursorForState(bPressed, bHovered);
}

void FDailyPacketProgram::ResetIdleTimer()
{
    LastInteractionTime = FPlatformTime::Seconds();
}

void FDailyPacketProgram::CheckIdleTimeout()
{
    const double CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime - LastInteractionTime > 8.0)
    {
        OnIdleTimeout.Broadcast();
        LastInteractionTime = CurrentTime; // Reset to avoid spam
    }
}

FSlateColor FDailyPacketProgram::GetCategoryHeaderColor(const FCategoryState& Category) const
{
    return GetProgramStyle().GetPrimaryColor();
}

FSlateColor FDailyPacketProgram::GetPoolItemColor(const FPoolItem& Item) const
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const bool bIsDraggingItem = DragState.bIsDragging && DragState.DraggedItemId == Item.ItemId;
    const bool bIsHoveringItem = !DragState.bIsDragging && HoverItemId == Item.ItemId && !Item.bUsed;

    if (bIsDraggingItem || bIsHoveringItem)
    {
        return Style.GetHighlightColor();
    }
    if (Item.bUsed)
    {
        return Style.GetDimColor();
    }
    return Style.GetDefaultBorderColor();
}

FSlateColor FDailyPacketProgram::GetPoolItemFillColor(const FPoolItem& Item) const
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const bool bIsDraggingItem = DragState.bIsDragging && DragState.DraggedItemId == Item.ItemId;
    const bool bIsHoveringItem = !DragState.bIsDragging && HoverItemId == Item.ItemId && !Item.bUsed;
    const FLinearColor Primary = Style.GetPrimaryColor();

    if (bIsDraggingItem || bIsHoveringItem)
    {
        return FSlateColor(FLinearColor(Primary.R, Primary.G, Primary.B, 0.25f));
    }
    if (Item.bUsed)
    {
        return FSlateColor(FLinearColor(Primary.R, Primary.G, Primary.B, 0.08f));
    }
    return FSlateColor(Style.GetBackgroundColor());
}

FSlateColor FDailyPacketProgram::GetSlotBorderColor(const FTemplateSlot& Slot) const
{
    if (DragState.bIsDragging && HoverSlotId == Slot.SlotId)
    {
        return GetProgramStyle().GetHighlightColor();
    }
    if (Slot.bHighlighted)
    {
        return GetProgramStyle().GetDefaultBorderColor();
    }
    return GetProgramStyle().GetPrimaryColor();
}

FSlateColor FDailyPacketProgram::GetSlotFillColor(const FTemplateSlot& Slot) const
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    if (Slot.bHighlighted)
    {
        const FLinearColor Primary = Style.GetPrimaryColor();
        const float Opacity = (DragState.bIsDragging && HoverSlotId == Slot.SlotId) ? 0.25f : 0.12f;
        return FSlateColor(FLinearColor(Primary.R, Primary.G, Primary.B, Opacity));
    }
    return FSlateColor(Style.GetBackgroundColor());
}

FSlateColor FDailyPacketProgram::GetActionButtonBorderColor(const FActionButton& Button) const
{
    if (!Button.bEnabled && !Button.bCompleted)
    {
        return GetProgramStyle().GetDimColor();
    }
    return GetProgramStyle().GetPrimaryColor();
}

FSlateColor FDailyPacketProgram::GetActionButtonFillColor(const FActionButton& Button) const
{
    if (Button.bCompleted)
    {
        return GetProgramStyle().GetDefaultFillColor();
    }
    if (Button.bPressed)
    {
        return GetProgramStyle().GetPressedFillColor();
    }
    if (Button.bHovered)
    {
        return GetProgramStyle().GetHoveredFillColor();
    }
    return FSlateColor(GetProgramStyle().GetBackgroundColor());
}

FSlateColor FDailyPacketProgram::GetActionButtonTextColor(const FActionButton& Button) const
{
    if (!Button.bEnabled && !Button.bCompleted)
    {
        return GetProgramStyle().GetDimColor();
    }
    return GetProgramStyle().GetDefaultBorderColor();
}

// ID-based lookup helpers for use in lambdas
FSlateColor FDailyPacketProgram::GetPoolItemColorById(FName ItemId) const
{
    if (const FPoolItem* PoolIt = FindPoolItemById(ItemId))
    {
        return GetPoolItemColor(*PoolIt);
    }
    return FSlateColor(GetProgramStyle().GetPrimaryColor());
}

FSlateColor FDailyPacketProgram::GetPoolItemFillColorById(FName ItemId) const
{
    if (const FPoolItem* PoolIt = FindPoolItemById(ItemId))
    {
        return GetPoolItemFillColor(*PoolIt);
    }
    return FSlateColor(GetProgramStyle().GetBackgroundColor());
}

FSlateColor FDailyPacketProgram::GetPoolItemTextColorById(FName ItemId) const
{
    if (const FPoolItem* PoolIt = FindPoolItemById(ItemId))
    {
        const bool bIsActive = (DragState.bIsDragging && DragState.DraggedItemId == ItemId) ||
            (!DragState.bIsDragging && HoverItemId == ItemId && !PoolIt->bUsed);

        const FScreenProgramStyle& Style = GetProgramStyle();

        if (PoolIt->bUsed && !bIsActive)
        {
            return Style.GetDimColor();
        }

        return FSlateColor(Style.GetPrimaryColor());
    }
    return FSlateColor(GetProgramStyle().GetPrimaryColor());
}

FSlateColor FDailyPacketProgram::GetSlotBorderColorById(FName SlotId) const
{
    if (const FTemplateSlot* Slot = FindTemplateSlotById(SlotId))
    {
        return GetSlotBorderColor(*Slot);
    }
    return FSlateColor(GetProgramStyle().GetPrimaryColor());
}

FSlateColor FDailyPacketProgram::GetSlotFillColorById(FName SlotId) const
{
    if (const FTemplateSlot* Slot = FindTemplateSlotById(SlotId))
    {
        return GetSlotFillColor(*Slot);
    }
    return FSlateColor(GetProgramStyle().GetBackgroundColor());
}

FSlateColor FDailyPacketProgram::GetActionButtonBorderColorById(FName ButtonId) const
{
    if (const FActionButton* Btn = FindActionButtonById(ButtonId))
    {
        return GetActionButtonBorderColor(*Btn);
    }
    return FSlateColor(GetProgramStyle().GetPrimaryColor());
}

FSlateColor FDailyPacketProgram::GetActionButtonFillColorById(FName ButtonId) const
{
    if (const FActionButton* Btn = FindActionButtonById(ButtonId))
    {
        return GetActionButtonFillColor(*Btn);
    }
    return FSlateColor(GetProgramStyle().GetBackgroundColor());
}

FSlateColor FDailyPacketProgram::GetActionButtonTextColorById(FName ButtonId) const
{
    if (const FActionButton* Btn = FindActionButtonById(ButtonId))
    {
        return GetActionButtonTextColor(*Btn);
    }
    return FSlateColor(GetProgramStyle().GetPrimaryColor());
}

float FDailyPacketProgram::GetActionButtonProgressWidthById(FName ButtonId, float BaseWidth) const
{
    if (const FActionButton* Btn = FindActionButtonById(ButtonId))
    {
        if (Btn->CurrentProgress > 0.0f)
        {
            return (BaseWidth - GetProgramStyle().LineThickness * 2.0f) * Btn->CurrentProgress;
        }
    }
    return 0.0f;
}

FSlateColor FDailyPacketProgram::GetSlotTextColorById(FName SlotId) const
{
    if (const FTemplateSlot* Slot = FindTemplateSlotById(SlotId))
    {
        const bool bIsActive = DragState.bIsDragging && HoverSlotId == SlotId;
        if (bIsActive)
        {
            return FSlateColor(GetProgramStyle().GetPrimaryColor());
        }
        return Slot->AssignedItemId.IsNone() ? GetProgramStyle().GetDimColor() : FSlateColor(GetProgramStyle().GetPrimaryColor());
    }
    return GetProgramStyle().GetDimColor();
}

FText FDailyPacketProgram::GetSlotTextById(FName SlotId) const
{
    if (const FTemplateSlot* Slot = FindTemplateSlotById(SlotId))
    {
        if (!Slot->AssignedItemId.IsNone())
        {
            if (const FPoolItem* AssignedItem = FindPoolItemById(Slot->AssignedItemId))
            {
                return AssignedItem->DisplayText;
            }
            return FText::FromName(Slot->AssignedItemId);
        }
        return Slot->Label;
    }
    return FText::FromString(TEXT("Slot"));
}

FText FDailyPacketProgram::GetActionButtonTextById(FName ButtonId) const
{
    if (const FActionButton* Btn = FindActionButtonById(ButtonId))
    {
        if (Btn->bCompleted)
        {
            return FText::FromString(FString::Printf(TEXT("[%s]"), *Btn->Label.ToString()));
        }
        return Btn->Label;
    }
    return FText::GetEmpty();
}

FSlateColor FDailyPacketProgram::GetActionButtonProgressColorById(FName ButtonId) const
{
    if (const FActionButton* Btn = FindActionButtonById(ButtonId))
    {
        if (Btn->CurrentProgress > 0.0f)
        {
            return FSlateColor(GetProgramStyle().GetPrimaryColor());
        }
    }
    return FSlateColor(FLinearColor::Transparent);
}
