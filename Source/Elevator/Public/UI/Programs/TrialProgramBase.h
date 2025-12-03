#pragma once

#include "CoreMinimal.h"
#include "UI/ScreenProgramBase.h"

/**
 * Base class for trial-based programs that have multiple rounds/trials.
 * Provides shared trial counting, progress display, and reset-on-failure logic.
 */
class ELEVATOR_API FTrialProgramBase : public FScreenProgramBase
{
public:
    virtual ~FTrialProgramBase() override = default;

protected:
    FTrialProgramBase(int32 InTotalTrials = 10);

    // -------------------------------------------------------------------------
    // Trial Management - Subclasses use these
    // -------------------------------------------------------------------------

    /** Call when player completes a trial successfully. Returns true if all trials complete. */
    bool AdvanceTrial();

    /** Call when player fails a trial. Resets progress and generates new trial. */
    void FailTrial();

    /** Get current trial (0-indexed) */
    int32 GetCurrentTrial() const { return CurrentTrial; }

    /** Set the title rendered by the shared trial header. */
    void SetTrialTitle(const FText& InTitle);

    /** Get total number of trials */
    int32 GetTotalTrials() const { return TotalTrials; }

    /** Check if all trials are complete */
    bool AreAllTrialsComplete() const { return CurrentTrial >= TotalTrials; }

    // -------------------------------------------------------------------------
    // Timer Management
    // -------------------------------------------------------------------------

    /** Start the trial timer with a specific duration. */
    void StartTrialTimer(float Duration);

    /** Stop the trial timer. */
    void StopTrialTimer();

    /** Get normalized timer progress (0.0 to 1.0). Returns 0 if timer not active. */
    float GetTimerProgress() const;

    /** Called when the trial timer expires. Default implementation fails the trial. */
    virtual void OnTrialTimerExpired();

    // -------------------------------------------------------------------------
    // UI Construction - Standardized Layout
    // -------------------------------------------------------------------------

    /** Override this to provide the program title. Default returns empty. */
    virtual FText GetTrialTitle() const;

    /** Override this to provide the main content of the trial. Default returns an empty widget. */
    virtual TSharedRef<SWidget> BuildTrialBody();

    /** Override this to provide footer content (e.g. buttons). Optional. */
    virtual TSharedRef<SWidget> BuildTrialFooter();

    /** Standard implementation of BuildProgramWidget that assembles the trial layout. */
    virtual TSharedRef<SWidget> BuildProgramWidget() override;
    
    /** Handle timer updates. */
    virtual void OnTick(float DeltaTime) override;

    // -------------------------------------------------------------------------
    // UI Helpers - Use these in BuildProgramWidget
    // -------------------------------------------------------------------------

    /** Build a progress bar widget showing completed trials */
    TSharedRef<SWidget> BuildProgressBar() const;

    /** Get progress bar string for custom rendering */
    FString GetProgressBarString() const;

    // -------------------------------------------------------------------------
    // Override Points - Subclasses must implement
    // -------------------------------------------------------------------------

    /** Called when a new trial should be generated (after advance or fail) */
    virtual void GenerateNewTrial() = 0;

private:
    int32 CurrentTrial = 0;
    int32 TotalTrials = 10;

    // Timer state
    bool bTimerActive = false;
    double TrialStartTime = 0.0;
    float TrialDuration = 0.0f;

    FText TrialTitle;
    bool bHasTrialTitle = false;
};
