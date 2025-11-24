#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundAttenuation.h"
#include "ProceduralAudioSettings.generated.h"

class USoundBase;

/**
 * Minimal set of sound references consumed by the procedural office sample.
 */
USTRUCT(BlueprintType)
struct ELEVATOR_API FProceduralAudioRegistry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Elevator")
    TObjectPtr<USoundBase> ElevatorDoorOpen = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Elevator")
    TObjectPtr<USoundBase> ElevatorDoorClose = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Elevator")
    TObjectPtr<USoundBase> ElevatorRide = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Elevator")
    TObjectPtr<USoundBase> ElevatorButtonClick = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Workstation")
    TObjectPtr<USoundBase> WorkstationMousePress = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Workstation")
    TObjectPtr<USoundBase> WorkstationMouseRelease = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Workstation")
    TObjectPtr<USoundBase> WorkstationBoot = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience")
    TObjectPtr<USoundBase> OfficeRoomTone = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience")
    TObjectPtr<USoundBase> AnnexRoomTone = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience")
    TObjectPtr<USoundBase> OutsideRoomTone = nullptr;
};
