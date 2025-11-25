#include "System/ElevatorGameManagerSubsystem.h"

#include "Engine/GameInstance.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Templates/SharedPointer.h"
#include "UI/ScreenProgramIds.h"
#include "UI/ScreenProgramRegistry.h"

namespace
{
    FString JoinNames(const TArray<FName>& Names)
    {
        if (Names.Num() == 0)
        {
            return TEXT("[]");
        }

        FString Result(TEXT("["));
        for (int32 Index = 0; Index < Names.Num(); ++Index)
        {
            Result += Names[Index].ToString();
            if (Index < Names.Num() - 1)
            {
                Result += TEXT(", ");
            }
        }
        Result += TEXT("]");
        return Result;
    }

    FString JoinNames(const TSet<FName>& NamesSet)
    {
        if (NamesSet.Num() == 0)
        {
            return TEXT("[]");
        }

        TArray<FName> SortedNames = NamesSet.Array();
        SortedNames.Sort(FNameLexicalLess());
        return JoinNames(SortedNames);
    }
}

UElevatorGameManagerSubsystem* UElevatorGameManagerSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    const UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    if (UGameInstance* GameInstance = World->GetGameInstance())
    {
        return GameInstance->GetSubsystem<UElevatorGameManagerSubsystem>();
    }

    return nullptr;
}

void UElevatorGameManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    LoadSchedule();
    ApplyCurrentDayConfig(false);

    UE_LOG(LogTemp, Log, TEXT("[Manager] Initialized. Day=%d Program=%s LockedButtons=%s"),
        CurrentDayIndex,
        *ActiveProgramId.ToString(),
        *JoinNames(CurrentLockedButtons));

    DayChangedDelegate.Broadcast(CurrentDayIndex, ActiveDayConfig);
    ProgramChangedDelegate.Broadcast(ActiveProgramId);
    TaskStateChangedDelegate.Broadcast(bTaskComplete);
}

void UElevatorGameManagerSubsystem::SetTaskComplete(bool bCompleted)
{
    if (bTaskComplete == bCompleted)
    {
        return;
    }

    bTaskComplete = bCompleted;
    UE_LOG(LogTemp, Log, TEXT("[Manager] Task completion set to %s for Day %d."), bTaskComplete ? TEXT("true") : TEXT("false"), CurrentDayIndex);
    TaskStateChangedDelegate.Broadcast(bTaskComplete);
}

bool UElevatorGameManagerSubsystem::TryAdvanceDay()
{
    if (!bTaskComplete)
    {
        UE_LOG(LogTemp, Log, TEXT("[Manager] Cannot advance day %d: task not complete."), CurrentDayIndex);
        return false;
    }

    const int32 PreviousDay = CurrentDayIndex;
    ++CurrentDayIndex;
    bTaskComplete = false;
    TaskStateChangedDelegate.Broadcast(bTaskComplete);

    ApplyCurrentDayConfig(true);
    UE_LOG(LogTemp, Log, TEXT("[Manager] Advanced from Day %d to Day %d. Program=%s LockedButtons=%s"),
        PreviousDay,
        CurrentDayIndex,
        *ActiveProgramId.ToString(),
        *JoinNames(CurrentLockedButtons));
    return true;
}

bool UElevatorGameManagerSubsystem::IsElevatorButtonEnabled(FName ButtonId) const
{
    if (ButtonId.IsNone())
    {
        return false;
    }

    if (CurrentLockedButtons.Contains(ButtonId))
    {
        return false;
    }

    return true;
}

TSharedPtr<IScreenProgram> UElevatorGameManagerSubsystem::CreateActiveProgramInstance() const
{
    return CreateProgramInstanceForId(ActiveProgramId);
}

TSharedPtr<IScreenProgram> UElevatorGameManagerSubsystem::CreateProgramInstanceForId(FName ProgramId) const
{
    const FName RequestedId = ProgramId.IsNone() ? ActiveProgramId : ProgramId;
    const FName ResolvedId = RequestedId.IsNone() ? Schedule.DefaultProgramId : RequestedId;
    const FName FallbackId = ScreenProgramIds::SimpleButton;
    const FName FinalId = ResolvedId.IsNone() ? FallbackId : ResolvedId;

    UE_LOG(LogTemp, Log, TEXT("[Manager] Creating program instance. Requested=%s Resolved=%s Final=%s"),
        *RequestedId.ToString(),
        *ResolvedId.ToString(),
        *FinalId.ToString());

    TSharedPtr<IScreenProgram> Instance = FScreenProgramRegistry::Get().CreateProgram(FinalId);

    if (!Instance.IsValid() && FinalId != FallbackId)
    {
        UE_LOG(LogTemp, Warning, TEXT("Unknown workstation program '%s'. Attempting fallback '%s'."), *FinalId.ToString(), *FallbackId.ToString());
        Instance = FScreenProgramRegistry::Get().CreateProgram(FallbackId);
    }

    if (!Instance.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create workstation program for id '%s'. No fallback available."), *FinalId.ToString());
    }

    return Instance;
}

void UElevatorGameManagerSubsystem::HandleElevatorButtonPressed(FName ButtonId)
{
    if (ButtonId.IsNone())
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[Manager] Button %s pressed. Day=%d TaskComplete=%s"),
        *ButtonId.ToString(),
        CurrentDayIndex,
        bTaskComplete ? TEXT("true") : TEXT("false"));

    if (!IsElevatorButtonEnabled(ButtonId))
    {
        UE_LOG(LogTemp, Log, TEXT("[Manager] Ignoring button %s because it is disabled."), *ButtonId.ToString());
        return;
    }

    const FString ButtonIdString = ButtonId.ToString();
    if (ButtonIdString.StartsWith(TEXT("Floor")))
    {
        if (!TryAdvanceDay())
        {
            UE_LOG(LogTemp, Log, TEXT("[Manager] Day advancement blocked: task incomplete."));
        }
    }
}

void UElevatorGameManagerSubsystem::LoadSchedule()
{
    Schedule = FElevatorDaySchedule();
    Schedule.DefaultProgramId = ScreenProgramIds::SimpleButton;

    const FString SchedulePath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/DaySchedule.json"));
    FString FileContents;
    if (!FPaths::FileExists(SchedulePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Day schedule JSON not found at %s. Using defaults."), *SchedulePath);
        Schedule.Days.Add(MakeDefaultEntry(0));
        return;
    }

    if (!FFileHelper::LoadFileToString(FileContents, *SchedulePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load day schedule JSON from %s. Using defaults."), *SchedulePath);
        Schedule.Days.Add(MakeDefaultEntry(0));
        return;
    }

    TSharedPtr<FJsonObject> RootObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to parse day schedule JSON %s. Using defaults."), *SchedulePath);
        Schedule.Days.Add(MakeDefaultEntry(0));
        return;
    }

    FString DefaultProgramString;
    if (RootObject->TryGetStringField(TEXT("DefaultProgramId"), DefaultProgramString))
    {
        Schedule.DefaultProgramId = FName(*DefaultProgramString);
    }

    const TArray<TSharedPtr<FJsonValue>>* DefaultLockedButtonsArray = nullptr;
    if (RootObject->TryGetArrayField(TEXT("DefaultLockedButtons"), DefaultLockedButtonsArray))
    {
        Schedule.DefaultLockedButtons.Reset();
        for (const TSharedPtr<FJsonValue>& Value : *DefaultLockedButtonsArray)
        {
            FString ButtonIdString;
            if (Value.IsValid() && Value->TryGetString(ButtonIdString))
            {
                Schedule.DefaultLockedButtons.Add(FName(*ButtonIdString));
            }
        }
    }

    // Parse DefaultElementOverrideSets
    const TArray<TSharedPtr<FJsonValue>>* DefaultOverrideSetsArray = nullptr;
    if (RootObject->TryGetArrayField(TEXT("DefaultElementOverrideSets"), DefaultOverrideSetsArray))
    {
        Schedule.DefaultElementOverrideSets.Reset();
        for (const TSharedPtr<FJsonValue>& Value : *DefaultOverrideSetsArray)
        {
            FString SetIdString;
            if (Value.IsValid() && Value->TryGetString(SetIdString))
            {
                Schedule.DefaultElementOverrideSets.Add(FName(*SetIdString));
            }
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* DaysArray = nullptr;
    if (RootObject->TryGetArrayField(TEXT("Days"), DaysArray))
    {
        Schedule.Days.Reset();
        for (int32 Index = 0; Index < DaysArray->Num(); ++Index)
        {
            const TSharedPtr<FJsonValue>& Value = (*DaysArray)[Index];
            if (!Value.IsValid() || Value->Type != EJson::Object)
            {
                continue;
            }

            FElevatorDayProgramEntry Entry;
            if (FJsonObjectConverter::JsonObjectToUStruct(Value->AsObject().ToSharedRef(), &Entry, 0, 0))
            {
                if (Entry.DayNumber == INDEX_NONE)
                {
                    Entry.DayNumber = Index;
                }
                Schedule.Days.Add(Entry);
            }
        }
    }
    else if (RootObject->HasField(TEXT("ProgramId")) || RootObject->HasField(TEXT("LockedButtons")))
    {
        Schedule.Days.Reset();
        FElevatorDayProgramEntry Entry;
        if (FJsonObjectConverter::JsonObjectToUStruct(RootObject.ToSharedRef(), &Entry, 0, 0))
        {
            if (Entry.DayNumber == INDEX_NONE)
            {
                Entry.DayNumber = 0;
            }
            Schedule.Days.Add(Entry);
        }
    }

    if (Schedule.DefaultProgramId.IsNone())
    {
        Schedule.DefaultProgramId = ScreenProgramIds::SimpleButton;
    }

    if (Schedule.Days.Num() == 0)
    {
        Schedule.Days.Add(MakeDefaultEntry(0));
    }

    UE_LOG(LogTemp, Log, TEXT("[Manager] Loaded day schedule from %s. DayCount=%d DefaultProgram=%s DefaultLocked=%s DefaultOverrideSets=%s"),
        *SchedulePath,
        Schedule.Days.Num(),
        *Schedule.DefaultProgramId.ToString(),
        *JoinNames(Schedule.DefaultLockedButtons),
        *JoinNames(Schedule.DefaultElementOverrideSets));
}

void UElevatorGameManagerSubsystem::ApplyCurrentDayConfig(bool bBroadcast)
{
    const FElevatorDayProgramEntry* Entry = FindConfigForDay(CurrentDayIndex);
    ActiveDayConfig = Entry ? *Entry : MakeDefaultEntry(CurrentDayIndex);

    ActiveProgramId = ActiveDayConfig.ProgramId.IsNone() ? Schedule.DefaultProgramId : ActiveDayConfig.ProgramId;
    if (ActiveProgramId.IsNone())
    {
        ActiveProgramId = ScreenProgramIds::SimpleButton;
    }

    CurrentLockedButtons = BuildLockedButtonSet(ActiveDayConfig);
    CurrentElementOverrideSets = BuildElementOverrideSetList(ActiveDayConfig);

    UE_LOG(LogTemp, Log, TEXT("[Manager] Day %d config applied. Program=%s LockedButtons=%s OverrideSets=%s"),
        CurrentDayIndex,
        *ActiveProgramId.ToString(),
        *JoinNames(CurrentLockedButtons),
        *JoinNames(CurrentElementOverrideSets));

    if (bBroadcast)
    {
        DayChangedDelegate.Broadcast(CurrentDayIndex, ActiveDayConfig);
        ProgramChangedDelegate.Broadcast(ActiveProgramId);
    }
}

FElevatorDayProgramEntry UElevatorGameManagerSubsystem::MakeDefaultEntry(int32 DayIndex) const
{
    FElevatorDayProgramEntry Entry;
    Entry.DayNumber = DayIndex;
    Entry.ProgramId = Schedule.DefaultProgramId.IsNone() ? ScreenProgramIds::SimpleButton : Schedule.DefaultProgramId;
    Entry.LockedButtons = Schedule.DefaultLockedButtons;
    Entry.ElementOverrideSets = Schedule.DefaultElementOverrideSets;
    return Entry;
}

const FElevatorDayProgramEntry* UElevatorGameManagerSubsystem::FindConfigForDay(int32 DayIndex) const
{
    for (int32 Index = 0; Index < Schedule.Days.Num(); ++Index)
    {
        const FElevatorDayProgramEntry& Entry = Schedule.Days[Index];
        const int32 EntryDay = (Entry.DayNumber >= 0) ? Entry.DayNumber : Index;
        if (EntryDay == DayIndex)
        {
            return &Entry;
        }
    }

    if (Schedule.Days.IsValidIndex(DayIndex))
    {
        return &Schedule.Days[DayIndex];
    }

    return Schedule.Days.Num() > 0 ? &Schedule.Days.Last() : nullptr;
}

TSet<FName> UElevatorGameManagerSubsystem::BuildLockedButtonSet(const FElevatorDayProgramEntry& Entry) const
{
    TSet<FName> Result;

    for (const FName& ButtonId : Schedule.DefaultLockedButtons)
    {
        if (!ButtonId.IsNone())
        {
            Result.Add(ButtonId);
        }
    }

    for (const FName& ButtonId : Entry.LockedButtons)
    {
        if (!ButtonId.IsNone())
        {
            Result.Add(ButtonId);
        }
    }

    return Result;
}

TArray<FName> UElevatorGameManagerSubsystem::BuildElementOverrideSetList(const FElevatorDayProgramEntry& Entry) const
{
    TArray<FName> Result;

    // Add default override sets first
    for (const FName& SetId : Schedule.DefaultElementOverrideSets)
    {
        if (!SetId.IsNone())
        {
            Result.AddUnique(SetId);
        }
    }

    // Add day-specific override sets (these can override defaults)
    for (const FName& SetId : Entry.ElementOverrideSets)
    {
        if (!SetId.IsNone())
        {
            Result.AddUnique(SetId);
        }
    }

    return Result;
}
