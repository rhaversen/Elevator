#pragma once

#include "CoreMinimal.h"
#include "ElementOverrides.generated.h"

/**
 * Property override for PCs and Elevators.
 * - PCs: bPoweredOn controls if the monitor is on and interactable
 * - Elevators: bLocked controls if the elevator can be used
 */
USTRUCT(BlueprintType)
struct ELEVATOR_API FElementPropertyOverride
{
    GENERATED_BODY()

    /** The ID of the element to apply overrides to (matches Id field in layout JSON) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
    FName ElementId;

    /** Whether this element is locked (for elevators) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
    bool bLocked = true;

    /** Whether this element is powered on (for PCs/monitors) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
    bool bPoweredOn = false;
};

/**
 * A named collection of element overrides.
 */
USTRUCT(BlueprintType)
struct ELEVATOR_API FElementOverrideSet
{
    GENERATED_BODY()

    /** Unique identifier for this override set */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
    FName SetId;

    /** The property overrides to apply */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
    TArray<FElementPropertyOverride> Overrides;
};

/**
 * Root structure for the ElementOverrides.json file.
 * Contains named override sets that can be referenced from DaySchedule.
 */
USTRUCT(BlueprintType)
struct ELEVATOR_API FElementOverridesData
{
    GENERATED_BODY()

    /** Named override sets that can be applied per-day or per-scenario */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
    TArray<FElementOverrideSet> OverrideSets;

    /** Find an override set by its ID */
    const FElementOverrideSet* FindOverrideSet(FName SetId) const;

    /** Find a specific element override within a set */
    const FElementPropertyOverride* FindElementOverride(FName SetId, FName ElementId) const;
};

/**
 * Helper class for loading element overrides from JSON.
 */
class ELEVATOR_API FElementOverridesManager
{
public:
    /** Load overrides from the default JSON file path */
    static bool LoadFromDefaultPath(FElementOverridesData& OutData);

    /** Load overrides from a specific file path (relative to Content folder) */
    static bool LoadFromFile(const FString& RelativePath, FElementOverridesData& OutData);

    /** Default path for the element overrides JSON file */
    static const FString DefaultFilePath;
};
