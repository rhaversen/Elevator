#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"
#include "InputCoreTypes.h"
#include "Math/Box2D.h"
#include "Templates/Function.h"
#include "UI/IScreenProgram.h"
#include "Widgets/SCanvas.h"

enum class EScreenProgramWindowHitRegion : uint8
{
    None,
    TitleBar,
    ResizeHandle,
    Content
};

struct ELEVATOR_API FScreenProgramWindowConfig
{
    FName WindowId;
    FText Title;
    FVector2D InitialPosition = FVector2D::ZeroVector;
    FVector2D InitialSize = FVector2D(400.0f, 300.0f);
    FVector2D MinimumSize = FVector2D(100.0f, 100.0f);
    bool bCanResize = true;
    bool bCanMove = true;
    bool bClampToViewport = true;
    float TitleBarHeight = 30.0f;
    float ResizeHandleSize = 20.0f;
};

struct ELEVATOR_API FScreenProgramWindowChrome
{
    FLinearColor BorderColor = FLinearColor::Green;
    FLinearColor BackgroundColor = FLinearColor::Black;
    FLinearColor TitleBarColor = FLinearColor::Black;
    FLinearColor TitleTextColor = FLinearColor::Green;
    FMargin ContentPadding = FMargin(10.0f);
};

struct ELEVATOR_API FScreenProgramWindowMetrics
{
    FVector2D Position = FVector2D::ZeroVector;
    FVector2D Size = FVector2D::ZeroVector;
    float TitleBarHeight = 0.0f;
    float ResizeHandleSize = 0.0f;
    FMargin ContentPadding = FMargin(0.0f);
};

class FWindowedScreenProgramBase;

class ELEVATOR_API FScreenProgramWindowBuilder
{
public:
    using FContentBuilder = TFunction<TSharedRef<SWidget>(const FScreenProgramWindowConfig&)>;

    explicit FScreenProgramWindowBuilder(FWindowedScreenProgramBase& InOwner);

    void AddWindow(const FScreenProgramWindowConfig& Config, const FScreenProgramWindowChrome& Chrome, FContentBuilder ContentBuilder);

private:
    FWindowedScreenProgramBase& Owner;
};

class ELEVATOR_API FWindowedScreenProgramBase : public IScreenProgram, public TSharedFromThis<FWindowedScreenProgramBase>
{
public:
    virtual ~FWindowedScreenProgramBase() override = default;

    virtual TSharedRef<SWidget> CreateWidget(const FVector2D& Size, const FScreenProgramStyle& Style) override final;
    virtual void OnPointerMoved(const FScreenPointerEvent& Event) override final;
    virtual void OnPointerPressed(const FScreenPointerEvent& Event) override final;
    virtual void OnPointerReleased(const FScreenPointerEvent& Event) override final;
    virtual void OnScreenResized(const FVector2D& NewSize) override final;
    virtual EMouseCursor::Type GetCursorType() const override final;
    virtual bool IsTaskComplete() const override { return bTaskComplete; }

protected:
    FWindowedScreenProgramBase();

    virtual void BuildWindowLayout(FScreenProgramWindowBuilder& Builder) = 0;

    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByChrome);
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByChrome);
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByChrome);
    virtual void HandleScreenResized(const FVector2D& NewSize);
    virtual void HandleTaskCompletionChanged(bool bCompleted);

    void SetTaskComplete(bool bCompleted);
    bool GetTaskComplete() const { return bTaskComplete; }

    void SetCursorOverride(TOptional<EMouseCursor::Type> CursorOverride);

    const FVector2D& GetProgramSize() const { return ProgramSize; }
    const FScreenProgramStyle& GetProgramStyle() const { return ActiveStyle; }
    FName GetHoveredWindowId() const { return HoveredWindowId; }
    bool TryGetWindowMetrics(FName WindowId, FScreenProgramWindowMetrics& OutMetrics) const;

private:
    friend class FScreenProgramWindowBuilder;

    struct FWindowInstance
    {
        FScreenProgramWindowConfig Config;
        FScreenProgramWindowChrome Chrome;
        TSharedPtr<SWidget> ContentWidget;
        TSharedPtr<SWidget> RootWidget;
        SCanvas::FSlot* CanvasSlot = nullptr;
        FVector2D Position = FVector2D::ZeroVector;
        FVector2D Size = FVector2D::ZeroVector;
        int32 ZOrder = 0;
    };

    struct FInteractionState
    {
        FName WindowId = NAME_None;
        EScreenProgramWindowHitRegion Region = EScreenProgramWindowHitRegion::None;
        FVector2D GrabOffset = FVector2D::ZeroVector;
        FKey PointerKey = EKeys::Invalid;

        bool IsActive() const { return Region != EScreenProgramWindowHitRegion::None && PointerKey.IsValid(); }
        void Reset()
        {
            WindowId = NAME_None;
            Region = EScreenProgramWindowHitRegion::None;
            GrabOffset = FVector2D::ZeroVector;
            PointerKey = EKeys::Invalid;
        }
    };

    struct FWindowHitResult
    {
        FName WindowId = NAME_None;
        EScreenProgramWindowHitRegion Region = EScreenProgramWindowHitRegion::None;
        FVector2D LocalPosition = FVector2D::ZeroVector;
    };

    void AddWindowInternal(const FScreenProgramWindowConfig& Config, const FScreenProgramWindowChrome& Chrome, FScreenProgramWindowBuilder::FContentBuilder&& ContentBuilder);
    FWindowInstance* FindWindow(FName WindowId);
    const FWindowInstance* FindWindow(FName WindowId) const;
    void BringWindowToFront(FWindowInstance& Instance);
    void ClampWindowToViewport(FWindowInstance& Instance);
    FWindowHitResult HitTestWindows(const FVector2D& ProgramPixel) const;
    bool ProcessPointerPressed(const FScreenPointerEvent& Event);
    bool ProcessPointerMoved(const FScreenPointerEvent& Event);
    bool ProcessPointerReleased(const FScreenPointerEvent& Event);
    void UpdateHoverState(const FWindowHitResult& Hit);
    void UpdateBaseCursorForHit(const FWindowHitResult& Hit);
    void UpdateCursorFromInteraction();
    TSharedRef<SWidget> BuildWindowWidget(FWindowInstance& Instance, const FScreenProgramStyle& Style);
    FVector2D GetWindowPositionAttribute(FName WindowId) const;
    FVector2D GetWindowSizeAttribute(FName WindowId) const;

    TSharedPtr<SCanvas> RootCanvas;
    TArray<FWindowInstance> WindowInstances;
    FInteractionState Interaction;
    FVector2D ProgramSize = FVector2D::ZeroVector;
    FScreenProgramStyle ActiveStyle;
    FName HoveredWindowId = NAME_None;
    FVector2D LastPointerNormalized = FVector2D(0.5f, 0.5f);
    FVector2D LastPointerPixel = FVector2D::ZeroVector;
    int32 NextZOrder = 0;
    bool bTaskComplete = false;
    TOptional<EMouseCursor::Type> CursorOverride;
    EMouseCursor::Type BaseCursor = EMouseCursor::Default;
};
