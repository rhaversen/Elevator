#include "Procedural/ElementOverrides.h"

#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogElementOverrides, Log, All);

const FString FElementOverridesManager::DefaultFilePath = TEXT("Data/ElementOverrides.json");

const FElementOverrideSet* FElementOverridesData::FindOverrideSet(FName SetId) const
{
    if (SetId.IsNone())
    {
        return nullptr;
    }

    for (const FElementOverrideSet& Set : OverrideSets)
    {
        if (Set.SetId == SetId)
        {
            return &Set;
        }
    }

    return nullptr;
}

const FElementPropertyOverride* FElementOverridesData::FindElementOverride(FName SetId, FName ElementId) const
{
    const FElementOverrideSet* Set = FindOverrideSet(SetId);
    if (!Set)
    {
        return nullptr;
    }

    for (const FElementPropertyOverride& Override : Set->Overrides)
    {
        if (Override.ElementId == ElementId)
        {
            return &Override;
        }
    }

    return nullptr;
}

bool FElementOverridesManager::LoadFromDefaultPath(FElementOverridesData& OutData)
{
    return LoadFromFile(DefaultFilePath, OutData);
}

bool FElementOverridesManager::LoadFromFile(const FString& RelativePath, FElementOverridesData& OutData)
{
    OutData = FElementOverridesData();

    if (RelativePath.IsEmpty())
    {
        UE_LOG(LogElementOverrides, Warning, TEXT("Empty file path provided for element overrides."));
        return false;
    }

    const FString AbsolutePath = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectContentDir(), RelativePath));

    if (!FPaths::FileExists(AbsolutePath))
    {
        UE_LOG(LogElementOverrides, Log, TEXT("Element overrides file not found: %s. This is optional."), *AbsolutePath);
        return false;
    }

    FString FileContents;
    if (!FFileHelper::LoadFileToString(FileContents, *AbsolutePath))
    {
        UE_LOG(LogElementOverrides, Warning, TEXT("Failed to read element overrides file: %s"), *AbsolutePath);
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogElementOverrides, Warning, TEXT("Failed to parse element overrides JSON: %s"), *AbsolutePath);
        return false;
    }

    // Parse OverrideSets array
    const TArray<TSharedPtr<FJsonValue>>* SetsArray = nullptr;
    if (RootObject->TryGetArrayField(TEXT("OverrideSets"), SetsArray))
    {
        for (const TSharedPtr<FJsonValue>& SetValue : *SetsArray)
        {
            if (!SetValue.IsValid() || SetValue->Type != EJson::Object)
            {
                continue;
            }

            const TSharedPtr<FJsonObject>& SetObject = SetValue->AsObject();
            FElementOverrideSet NewSet;

            // Parse SetId
            FString SetIdString;
            if (SetObject->TryGetStringField(TEXT("SetId"), SetIdString))
            {
                NewSet.SetId = FName(*SetIdString);
            }

            // Parse Overrides array
            const TArray<TSharedPtr<FJsonValue>>* OverridesArray = nullptr;
            if (SetObject->TryGetArrayField(TEXT("Overrides"), OverridesArray))
            {
                for (const TSharedPtr<FJsonValue>& OverrideValue : *OverridesArray)
                {
                    if (!OverrideValue.IsValid() || OverrideValue->Type != EJson::Object)
                    {
                        continue;
                    }

                    const TSharedPtr<FJsonObject>& OverrideObject = OverrideValue->AsObject();
                    FElementPropertyOverride NewOverride;

                    // Parse ElementId
                    FString ElementIdString;
                    if (OverrideObject->TryGetStringField(TEXT("ElementId"), ElementIdString))
                    {
                        NewOverride.ElementId = FName(*ElementIdString);
                    }

                    // Parse bLocked (for elevators) - defaults to true
                    OverrideObject->TryGetBoolField(TEXT("bLocked"), NewOverride.bLocked);
                    
                    // Parse bPoweredOn (for PCs) - defaults to false
                    OverrideObject->TryGetBoolField(TEXT("bPoweredOn"), NewOverride.bPoweredOn);

                    if (!NewOverride.ElementId.IsNone())
                    {
                        NewSet.Overrides.Add(NewOverride);
                    }
                }
            }

            if (!NewSet.SetId.IsNone())
            {
                OutData.OverrideSets.Add(NewSet);
            }
        }
    }

    UE_LOG(LogElementOverrides, Log, TEXT("Loaded %d override sets from %s"),
        OutData.OverrideSets.Num(), *AbsolutePath);

    return true;
}
