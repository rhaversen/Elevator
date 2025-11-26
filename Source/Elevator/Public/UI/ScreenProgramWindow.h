#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"
#include "InputCoreTypes.h"
#include "Math/Box2D.h"
#include "Templates/Function.h"
#include "UI/ScreenProgramBase.h"
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

class ELEVATOR_API FWindowedScreenProgramBase : public FScreenProgramBase
{
public:
    virtual ~FWindowedScreenProgramBase() override = default;

protected:
    FWindowedScreenProgramBase();

    virtual TSharedRef<SWidget> BuildProgramWidget() override;
    virtual void BuildWindowLayout(FScreenProgramWindowBuilder& Builder) = 0;
    virtual TSharedRef<SWidget> BuildProgramContent();

    virtual bool PreHandlePointerMoved(const FScreenPointerEvent& Event) override;
    virtual bool PreHandlePointerPressed(const FScreenPointerEvent& Event) override;
    virtual bool PreHandlePointerReleased(const FScreenPointerEvent& Event) override;

    virtual void HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByChrome) override;
    virtual void HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByChrome) override;
    virtual void HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByChrome) override;
    virtual void HandleScreenResized(const FVector2D& NewSize) override;

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
    FName HoveredWindowId = NAME_None;
    int32 NextZOrder = 0;
};
