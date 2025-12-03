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
#include "Algo/Sort.h"

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
    const FName FallbackId = ScreenProgramIds::DailyPacket;
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

bool UElevatorGameManagerSubsystem::TrySelectProgramById(FName ProgramId)
{
    if (ProgramId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Manager] Cannot select workstation program: provided id is None."));
        return false;
    }

    if (!FScreenProgramRegistry::Get().IsProgramRegistered(ProgramId))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Manager] Cannot select workstation program '%s': id is not registered."), *ProgramId.ToString());
        return false;
    }

    if (ProgramId == ActiveProgramId)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Manager] Workstation program '%s' is already active."), *ProgramId.ToString());
        return false;
    }

    ActiveProgramId = ProgramId;
    ActiveDayConfig.ProgramId = ProgramId;
    SetTaskComplete(false);

    UE_LOG(LogTemp, Log, TEXT("[Manager] Active workstation program changed to '%s'."), *ActiveProgramId.ToString());
    ProgramChangedDelegate.Broadcast(ActiveProgramId);
    return true;
}

bool UElevatorGameManagerSubsystem::TrySelectProgramByOffset(int32 Offset)
{
    if (Offset == 0)
    {
        return false;
    }

    TArray<FName> ProgramIds = FScreenProgramRegistry::Get().GetRegisteredProgramIds();
    if (ProgramIds.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Manager] Cannot cycle workstation programs: registry is empty."));
        return false;
    }

    ProgramIds.Sort(FNameLexicalLess());

    const int32 Count = ProgramIds.Num();
    int32 CurrentIndex = ProgramIds.IndexOfByKey(ActiveProgramId);
    if (CurrentIndex == INDEX_NONE)
    {
        CurrentIndex = 0;
    }

    int32 TargetIndex = CurrentIndex + Offset;
    TargetIndex %= Count;
    if (TargetIndex < 0)
    {
        TargetIndex += Count;
    }

    if (TargetIndex == CurrentIndex && ProgramIds[TargetIndex] == ActiveProgramId)
    {
        return false;
    }

    return TrySelectProgramById(ProgramIds[TargetIndex]);
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
    Schedule.DefaultProgramId = ScreenProgramIds::DailyPacket;

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
    FString DefaultOfficeLayoutString;
    if (RootObject->TryGetStringField(TEXT("DefaultOfficeLayout"), DefaultOfficeLayoutString))
    {
        Schedule.DefaultOfficeLayout = DefaultOfficeLayoutString;
    }

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
    TSet<int32> RegisteredDayNumbers;

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
                    UE_LOG(LogTemp, Warning, TEXT("Day schedule entry at index %d is missing required DayNumber. Entry will be ignored."), Index);
                    continue;
                }

                if (Entry.DayNumber < 0)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Day schedule entry at index %d has invalid DayNumber %d. Entry will be ignored."), Index, Entry.DayNumber);
                    continue;
                }

                if (RegisteredDayNumbers.Contains(Entry.DayNumber))
                {
                    UE_LOG(LogTemp, Warning, TEXT("Duplicate day schedule entry for Day %d detected. Only the first occurrence will be used."), Entry.DayNumber);
                    continue;
                }

                RegisteredDayNumbers.Add(Entry.DayNumber);
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
                UE_LOG(LogTemp, Warning, TEXT("Day schedule root-level entry is missing required DayNumber. Entry will be ignored."));
            }
            else if (Entry.DayNumber < 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("Day schedule root-level entry has invalid DayNumber %d. Entry will be ignored."), Entry.DayNumber);
            }
            else if (RegisteredDayNumbers.Contains(Entry.DayNumber))
            {
                UE_LOG(LogTemp, Warning, TEXT("Duplicate day schedule entry for Day %d detected at root level. Entry will be ignored."), Entry.DayNumber);
            }
            else
            {
                RegisteredDayNumbers.Add(Entry.DayNumber);
                Schedule.Days.Add(Entry);
            }
        }
    }

    if (Schedule.DefaultProgramId.IsNone())
    {
        Schedule.DefaultProgramId = ScreenProgramIds::DailyPacket;
    }

    Schedule.Days.Sort([](const FElevatorDayProgramEntry& LHS, const FElevatorDayProgramEntry& RHS)
    {
        return LHS.DayNumber < RHS.DayNumber;
    });

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
    const FElevatorDayProgramEntry* OverrideEntry = FindConfigForDay(CurrentDayIndex);
    FElevatorDayProgramEntry DefaultEntry = MakeDefaultEntry(CurrentDayIndex);

    if (OverrideEntry)
    {
        FElevatorDayProgramEntry MergedEntry = DefaultEntry;

        if (!OverrideEntry->ProgramId.IsNone())
        {
            MergedEntry.ProgramId = OverrideEntry->ProgramId;
        }

        if (OverrideEntry->LockedButtons.Num() > 0)
        {
            MergedEntry.LockedButtons = OverrideEntry->LockedButtons;
        }

        if (!OverrideEntry->OfficeLayout.IsEmpty())
        {
            MergedEntry.OfficeLayout = OverrideEntry->OfficeLayout;
        }

        if (OverrideEntry->EnabledMonitors.Num() > 0)
        {
            MergedEntry.EnabledMonitors = OverrideEntry->EnabledMonitors;
        }

        if (OverrideEntry->SoundsToPlay.Num() > 0)
        {
            MergedEntry.SoundsToPlay = OverrideEntry->SoundsToPlay;
        }

        if (OverrideEntry->ElementOverrideSets.Num() > 0)
        {
            MergedEntry.ElementOverrideSets = OverrideEntry->ElementOverrideSets;
        }

        ActiveDayConfig = MergedEntry;
    }
    else
    {
        ActiveDayConfig = DefaultEntry;
    }

    ActiveDayConfig.DayNumber = CurrentDayIndex;

    ActiveProgramId = ActiveDayConfig.ProgramId.IsNone() ? Schedule.DefaultProgramId : ActiveDayConfig.ProgramId;
    if (ActiveProgramId.IsNone())
    {
        ActiveProgramId = ScreenProgramIds::DailyPacket;
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
    Entry.ProgramId = Schedule.DefaultProgramId.IsNone() ? ScreenProgramIds::DailyPacket : Schedule.DefaultProgramId;
    Entry.LockedButtons = Schedule.DefaultLockedButtons;
    Entry.OfficeLayout = Schedule.DefaultOfficeLayout;
    Entry.ElementOverrideSets = Schedule.DefaultElementOverrideSets;
    return Entry;
}

const FElevatorDayProgramEntry* UElevatorGameManagerSubsystem::FindConfigForDay(int32 DayIndex) const
{
    for (const FElevatorDayProgramEntry& Entry : Schedule.Days)
    {
        if (Entry.DayNumber == DayIndex)
        {
            return &Entry;
        }
    }

    return nullptr;
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
