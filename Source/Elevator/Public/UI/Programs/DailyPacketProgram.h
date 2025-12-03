#pragma once

#include "CoreMinimal.h"
#include "Math/Box2D.h"
#include "UI/ScreenProgramBase.h"
#include "Widgets/SWidget.h"
#include "Widgets/SOverlay.h"

/**
 * Daily Packet - The main completable task
 * 
 * Layout:
 * - Left Panel: Item Pool with Headlines (HL-01 to HL-10), Metrics (Ops/Sales/Support), Notes (Memo-01 to Memo-06)
 * - Center: Packet Template with Bullets A/B/C, Mini-Chart slot, Footer Note slot
 * - Right Panel: Actions (Preview -> Export -> Send)
 * 
 * Players drag items from left pool to center template slots.
 * Complete workflow: Preview (400ms) -> Export (600ms) -> Send (800ms)
 */
class ELEVATOR_API FDailyPacketProgram : public FScreenProgramBase
{
public:
    FDailyPacketProgram();
    explicit FDailyPacketProgram(int32 InDay);
    virtual ~FDailyPacketProgram() override = default;

    /** Set the current day (1-5). Affects how many items appear in the pool. */
    void SetDay(int32 InDay) { CurrentDay = FMath::Clamp(InDay, 1, 5); }
    int32 GetDay() const { return CurrentDay; }

    // Trigger events for pop-up scheduler
    DECLARE_MULTICAST_DELEGATE(FOnDailyPacketTrigger);
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnPoolItemOpened, FName /*ItemCategory*/);
    
    FOnPoolItemOpened OnPoolItemOpened;
    FOnDailyPacketTrigger OnFirstDrag;
    FOnDailyPacketTrigger OnMiniChartDropped;
    FOnDailyPacketTrigger OnFooterFocused;
    FOnDailyPacketTrigger OnPreviewClicked;
    FOnDailyPacketTrigger OnExportClicked;
    FOnDailyPacketTrigger OnSendClicked;
    FOnDailyPacketTrigger OnRapidFocusSwap;
    FOnDailyPacketTrigger OnIdleTimeout;
    FOnDailyPacketTrigger OnWorkspaceClosed;

protected:
    virtual TSharedRef<SWidget> BuildProgramWidget() override;
    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor) override;
    virtual void HandleScreenResized(const FVector2D& NewSize) override;
    virtual void HandleTaskCompletionChanged(bool bCompleted) override;

private:
    // Item Pool Entry
    struct FPoolItem
    {
        FName ItemId;
        FText DisplayText;
        FName Category; // Headlines, Metrics, Notes
        bool bUsed = false;
        TWeakPtr<SWidget> Widget;
    };

    // Template Slot
    struct FTemplateSlot
    {
        FName SlotId;
        FText Label;
        FName AcceptedCategory; // Empty = any, or specific category
        FName AssignedItemId;
        FBox2D Bounds;
        bool bHighlighted = false;
        TWeakPtr<SWidget> Widget;
    };

    // Action Button
    struct FActionButton
    {
        FName ButtonId;
        FText Label;
        bool bEnabled = false;
        bool bHovered = false;
        bool bPressed = false;
        bool bCompleted = false;
        float ProgressDuration = 0.0f;
        float CurrentProgress = 0.0f;
        FBox2D Bounds;
        TWeakPtr<SWidget> Widget;
    };

    // Drag state
    struct FDragState
    {
        bool bIsDragging = false;
        FName DraggedItemId;
        FVector2D DragOffset;
        FVector2D CurrentPosition;
        float InvalidDropShakeTime = 0.0f;
    };

    // Category fold state
    struct FCategoryState
    {
        FName Category;
        FText DisplayName;
        bool bExpanded = false;
        FBox2D HeaderBounds;
        TWeakPtr<SWidget> HeaderWidget;
    };

    // Build UI
    TSharedRef<SWidget> BuildItemPoolPanel();
    TSharedRef<SWidget> BuildTemplatePanel();
    TSharedRef<SWidget> BuildActionsPanel();
    TSharedRef<SWidget> BuildDragGhostLayer();

    // Initialize data
    void InitializePoolItems();
    void InitializeTemplateSlots();
    void InitializeActionButtons();

    // Hit testing
    FPoolItem* GetPoolItemAtPosition(const FScreenPointerEvent& Event);
    FTemplateSlot* GetTemplateSlotAtPosition(const FScreenPointerEvent& Event);
    FActionButton* GetActionButtonAtPosition(const FScreenPointerEvent& Event);

    // Drag-drop logic
    void BeginDrag(FPoolItem* Item, const FVector2D& Position);
    void UpdateDrag(const FScreenPointerEvent& Event);
    void EndDrag(const FScreenPointerEvent& Event);
    bool TryDropOnSlot(FTemplateSlot* Slot, const FName& ItemId);
    void TriggerInvalidDropFeedback();

    // Action button logic
    void OnActionButtonClicked(FActionButton* Button);
    void UpdateActionButtonStates();
    void TickProgressBars(float DeltaTime);

    // Validation
    bool IsTemplateComplete() const;
    bool CanPreview() const;
    bool CanExport() const;
    bool CanSend() const;

    // Visual updates
    void UpdateCursorVisual();
    void UpdateSlotHighlights(const FScreenPointerEvent& Event);
    void ClearSlotHighlights();
    void UpdateDragGhost(const FScreenPointerEvent& Event);
    void HideDragGhost();
    
    // Colors - by object reference
    FSlateColor GetCategoryHeaderColor(const FCategoryState& Category) const;
    FSlateColor GetPoolItemColor(const FPoolItem& Item) const;
    FSlateColor GetPoolItemFillColor(const FPoolItem& Item) const;
    FSlateColor GetSlotBorderColor(const FTemplateSlot& Slot) const;
    FSlateColor GetSlotFillColor(const FTemplateSlot& Slot) const;
    FSlateColor GetActionButtonBorderColor(const FActionButton& Button) const;
    FSlateColor GetActionButtonFillColor(const FActionButton& Button) const;
    FSlateColor GetActionButtonTextColor(const FActionButton& Button) const;

    // Colors - by ID lookup (for use in lambdas)
    FSlateColor GetPoolItemColorById(FName ItemId) const;
    FSlateColor GetPoolItemFillColorById(FName ItemId) const;
    FSlateColor GetPoolItemTextColorById(FName ItemId) const;
    FSlateColor GetSlotBorderColorById(FName SlotId) const;
    FSlateColor GetSlotFillColorById(FName SlotId) const;
    FSlateColor GetSlotTextColorById(FName SlotId) const;
    FText GetSlotTextById(FName SlotId) const;
    FSlateColor GetActionButtonBorderColorById(FName ButtonId) const;
    FSlateColor GetActionButtonFillColorById(FName ButtonId) const;
    FSlateColor GetActionButtonTextColorById(FName ButtonId) const;
    FText GetActionButtonTextById(FName ButtonId) const;
    float GetActionButtonProgressWidthById(FName ButtonId, float BaseWidth) const;
    FSlateColor GetActionButtonProgressColorById(FName ButtonId) const;
    FVector2D GetGhostDefaultSize() const;
    const FPoolItem* FindPoolItemById(FName ItemId) const;
    FTemplateSlot* FindTemplateSlotById(FName SlotId);
    const FTemplateSlot* FindTemplateSlotById(FName SlotId) const;
    FActionButton* FindActionButtonById(FName ButtonId);
    const FActionButton* FindActionButtonById(FName ButtonId) const;

    // Idle tracking
    void ResetIdleTimer();
    void CheckIdleTimeout();

    // Data
    TArray<FPoolItem> PoolItems;
    TArray<FCategoryState> Categories;
    TArray<FTemplateSlot> TemplateSlots;
    TArray<FActionButton> ActionButtons;
    
    // State
    FDragState DragState;
    FKey ActivePointerKey = EKeys::Invalid;
    bool bFirstDragOccurred = false;
    double LastInteractionTime = 0.0;
    int32 RecentFocusSwapCount = 0;
    double LastFocusSwapTime = 0.0;
    int32 CurrentDay = 1; // Day 1-5 determines pool item count

    // Drag ghost state
    struct FDragGhostState
    {
        bool bVisible = false;
        bool bSnapped = false;
        FVector2D Position = FVector2D::ZeroVector;
        FVector2D Size = FVector2D::ZeroVector;
        FText Label = FText::GetEmpty();
        FLinearColor BorderColor = FLinearColor::Transparent;
        FLinearColor FillColor = FLinearColor::Transparent;
        FLinearColor TextColor = FLinearColor::White;
    };

    FDragGhostState DragGhost;
    FName HoverItemId = NAME_None;
    FName HoverSlotId = NAME_None;
    TWeakPtr<SOverlay> RootOverlayWidget;
};
