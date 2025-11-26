#include "UI/ScreenProgramBase.h"

#include "Widgets/SNullWidget.h"

FScreenProgramBase::FScreenProgramBase()
{
    ProgramSize = FVector2D::ZeroVector;
    ActiveStyle = FScreenProgramStyle();
    CursorOverride.Reset();
    bTaskComplete = false;
    BaseCursor = EMouseCursor::Default;
    LastPointerNormalized = FVector2D(0.5f, 0.5f);
    LastPointerPixel = FVector2D::ZeroVector;
}

TSharedRef<SWidget> FScreenProgramBase::CreateWidget(const FVector2D& Size, const FScreenProgramStyle& Style)
{
    ProgramSize = FVector2D(FMath::Max(Size.X, 1.0f), FMath::Max(Size.Y, 1.0f));
    ActiveStyle = Style;
    CursorOverride.Reset();
    BaseCursor = EMouseCursor::Default;
    LastPointerNormalized = FVector2D(0.5f, 0.5f);
    LastPointerPixel = FVector2D::ZeroVector;

    TSharedRef<SWidget> Widget = BuildProgramWidget();
    RootWidget = Widget;

    HandleScreenResized(ProgramSize);
    return Widget;
}

void FScreenProgramBase::OnPointerMoved(const FScreenPointerEvent& Event)
{
    LastPointerNormalized = Event.ProgramNormalizedPosition;
    LastPointerPixel = Event.ProgramPixelPosition;

    const bool bHandled = PreHandlePointerMoved(Event);
    HandlePointerMoved(Event, bHandled);
}

void FScreenProgramBase::OnPointerPressed(const FScreenPointerEvent& Event)
{
    LastPointerNormalized = Event.ProgramNormalizedPosition;
    LastPointerPixel = Event.ProgramPixelPosition;

    const bool bHandled = PreHandlePointerPressed(Event);
    HandlePointerPressed(Event, bHandled);
}

void FScreenProgramBase::OnPointerReleased(const FScreenPointerEvent& Event)
{
    LastPointerNormalized = Event.ProgramNormalizedPosition;
    LastPointerPixel = Event.ProgramPixelPosition;

    const bool bHandled = PreHandlePointerReleased(Event);
    HandlePointerReleased(Event, bHandled);
}

void FScreenProgramBase::OnScreenResized(const FVector2D& NewSize)
{
    ProgramSize = FVector2D(FMath::Max(NewSize.X, 1.0f), FMath::Max(NewSize.Y, 1.0f));
    HandleScreenResized(ProgramSize);
}

EMouseCursor::Type FScreenProgramBase::GetCursorType() const
{
    if (CursorOverride.IsSet())
    {
        return CursorOverride.GetValue();
    }
    return BaseCursor;
}

bool FScreenProgramBase::PreHandlePointerMoved(const FScreenPointerEvent&)
{
    return false;
}

bool FScreenProgramBase::PreHandlePointerPressed(const FScreenPointerEvent&)
{
    return false;
}

bool FScreenProgramBase::PreHandlePointerReleased(const FScreenPointerEvent&)
{
    return false;
}

void FScreenProgramBase::HandlePointerMoved(const FScreenPointerEvent&, bool)
{
}

void FScreenProgramBase::HandlePointerPressed(const FScreenPointerEvent&, bool)
{
}

void FScreenProgramBase::HandlePointerReleased(const FScreenPointerEvent&, bool)
{
}

void FScreenProgramBase::HandleScreenResized(const FVector2D&)
{
}

void FScreenProgramBase::HandleTaskCompletionChanged(bool)
{
}

void FScreenProgramBase::SetTaskComplete(bool bCompleted)
{
    if (bTaskComplete == bCompleted)
    {
        return;
    }

    bTaskComplete = bCompleted;
    HandleTaskCompletionChanged(bTaskComplete);
}

void FScreenProgramBase::SetCursorOverride(TOptional<EMouseCursor::Type> InCursorOverride)
{
    CursorOverride = InCursorOverride;
}
