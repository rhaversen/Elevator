#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ElevatorGameManagerSubsystem.generated.h"

class IScreenProgram;

/**
 * Configuration entry describing how a given day should behave.
 * Only a subset of the fields are consumed today, but the struct is designed
 * to carry extra data (sounds, layouts, monitor states) as the project grows.
 */
USTRUCT(BlueprintType)
struct FElevatorDayProgramEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day")
    int32 DayNumber = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day")
    FName ProgramId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day")
    TArray<FName> LockedButtons;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day")
    FString OfficeLayout;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day")
    TArray<FName> EnabledMonitors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day")
    TArray<FName> SoundsToPlay;

    /** Element override set IDs to apply for this day (from ElementOverrides.json) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day")
    TArray<FName> ElementOverrideSets;
};

USTRUCT()
struct FElevatorDaySchedule
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FElevatorDayProgramEntry> Days;

    UPROPERTY()
    FName DefaultProgramId = NAME_None;

    UPROPERTY()
    TArray<FName> DefaultLockedButtons;

    UPROPERTY()
    FString DefaultOfficeLayout;

    /** Default element override set IDs applied to all days unless overridden */
    UPROPERTY()
    TArray<FName> DefaultElementOverrideSets;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnElevatorDayChanged, int32 /*DayIndex*/, const FElevatorDayProgramEntry& /*Config*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnElevatorTaskStateChanged, bool /*bIsComplete*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnElevatorProgramChanged, FName /*ProgramId*/);

/**
 * Lightweight central game manager that tracks day progression, task completion
 * and exposes which elevator buttons are currently usable. Designed to be
 * extended incrementally as new systems come online.
 */
UCLASS()
class ELEVATOR_API UElevatorGameManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    static UElevatorGameManagerSubsystem* Get(const UObject* WorldContextObject);

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    int32 GetCurrentDay() const { return CurrentDayIndex; }
    FName GetActiveProgramId() const { return ActiveProgramId; }
    const FElevatorDayProgramEntry& GetActiveDayConfig() const { return ActiveDayConfig; }

    /** Get the currently active element override set IDs (merged from defaults and day-specific) */
    const TArray<FName>& GetActiveElementOverrideSets() const { return CurrentElementOverrideSets; }

    bool IsTaskComplete() const { return bTaskComplete; }
    void SetTaskComplete(bool bCompleted);

    bool CanStartNextDay() const { return bTaskComplete; }
    bool TryAdvanceDay();

    bool IsElevatorButtonEnabled(FName ButtonId) const;

    TSharedPtr<IScreenProgram> CreateActiveProgramInstance() const;
    TSharedPtr<IScreenProgram> CreateProgramInstanceForId(FName ProgramId) const;

    void HandleElevatorButtonPressed(FName ButtonId);

    FOnElevatorDayChanged& OnDayChanged() { return DayChangedDelegate; }
    FOnElevatorTaskStateChanged& OnTaskStateChanged() { return TaskStateChangedDelegate; }
    FOnElevatorProgramChanged& OnProgramChanged() { return ProgramChangedDelegate; }

private:
    void LoadSchedule();
    void ApplyCurrentDayConfig(bool bBroadcast = true);
    FElevatorDayProgramEntry MakeDefaultEntry(int32 DayIndex) const;
    const FElevatorDayProgramEntry* FindConfigForDay(int32 DayIndex) const;
    TSet<FName> BuildLockedButtonSet(const FElevatorDayProgramEntry& Entry) const;
    TArray<FName> BuildElementOverrideSetList(const FElevatorDayProgramEntry& Entry) const;

    FElevatorDaySchedule Schedule;
    int32 CurrentDayIndex = 0;
    bool bTaskComplete = false;
    FName ActiveProgramId = NAME_None;
    FElevatorDayProgramEntry ActiveDayConfig;
    TSet<FName> CurrentLockedButtons;
    TArray<FName> CurrentElementOverrideSets;

    FOnElevatorDayChanged DayChangedDelegate;
    FOnElevatorTaskStateChanged TaskStateChangedDelegate;
    FOnElevatorProgramChanged ProgramChangedDelegate;
};
