#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "InteractionFocusProvider.h"
#include "Audio/ProceduralAudioSettings.h"
#include "Procedural/ElementOverrides.h"
#include "ProceduralOfficeGenerator.generated.h"

class UChildActorComponent;
class URectLightComponent;
class AProceduralElevator;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UArrowComponent;
class UInteractiveScreenComponent;
class UAudioComponent;
struct FElevatorDayProgramEntry;

UENUM(BlueprintType)
enum class EOfficeElementType : uint8
{
    Floor,
    Ceiling,
    Wall,
    Window,
    SpawnPoint,
    Cubicle,
    CeilingLight,
    Door,
    Elevator,
    RoomTone
};

USTRUCT(BlueprintType)
struct FOfficeElementDefinition
{
    GENERATED_BODY()

    /** Optional unique identifier for this element, used for property overrides */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    FName Id = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    EOfficeElementType Type = EOfficeElementType::Floor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    FVector2D Start = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    FVector2D End = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0.1"))
    float Thickness = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    float HeightOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    float Yaw = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    FVector2D Dimensions = FVector2D(240.0f, 240.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    FVector2D Spacing = FVector2D(400.0f, 400.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    FVector2D Padding = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "1"))
    int32 SectionCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "1.0"))
    float Height = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    FName AudioId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    bool bOmnidirectional = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    float VolumeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    float AttenuationRadius = 0.0f;
};

USTRUCT(BlueprintType)
struct FOfficeLayout
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    TArray<FOfficeElementDefinition> Elements;
};

/**
 * Actor that can read lightweight JSON layout definitions and stamp out modular office geometry using instanced meshes.
 */
UCLASS(Blueprintable)
class ELEVATOR_API AProceduralOfficeGenerator : public AActor, public IInteractable, public IInteractionFocusProvider
{
    GENERATED_BODY()

public:
    static const FName WorkstationMonitorComponentKey;
    static const FName WorkstationMonitorTag;

    AProceduralOfficeGenerator();
    virtual ~AProceduralOfficeGenerator();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UFUNCTION(CallInEditor, Category = "Generation")
    void GenerateFromData();

    UFUNCTION(CallInEditor, Category = "Generation")
    void ClearGeneratedContent();

    // IInteractable interface
    virtual bool CanInteract_Implementation(APawn* PlayerPawn) const override;
    virtual void OnInteract_Implementation(APawn* PlayerPawn) override;
    virtual FText GetInteractionPrompt_Implementation() const override;
    virtual void OnInteractionCanceled_Implementation(APawn* PlayerPawn) override;
    virtual void OnInteractionViewOpened_Implementation(APawn* PlayerPawn) override;
    virtual void OnInteractionPointerPressed_Implementation(APawn* PlayerPawn, const FHitResult& Hit, FKey PointerKey) override;
    virtual void OnInteractionPointerReleased_Implementation(APawn* PlayerPawn, const FHitResult& Hit, FKey PointerKey) override;
    virtual void OnInteractionHover_Implementation(const FHitResult& Hit) override;

    // IInteractionFocusProvider interface
    virtual bool EvaluateInteractionFocus_Implementation(APawn* PlayerPawn, const FHitResult& Hit, float AssistRadius, UPrimitiveComponent*& OutHighlightComponent) override;

    /** Check if an element is locked by its ID. Returns true (locked) if element not found. */
    UFUNCTION(BlueprintPure, Category = "Elevator")
    bool IsElementLocked(FName ElementId) const;

    /** Ensure element overrides are loaded (used by child actors spawned via child actor components). */
    void EnsureElementOverridesLoaded() const;

protected:
    bool LoadLayoutData(FOfficeLayout& OutLayout) const;
    bool LoadElementOverrides() const;
    void BuildFromLayout(const FOfficeLayout& Layout);
    void BuildElement(const FOfficeElementDefinition& Element);
    
    /** Get the property override for an element, returns nullptr if no override exists */
    const FElementPropertyOverride* GetElementOverride(FName ElementId) const;
    
    void PlaceSurface(EOfficeElementType Type, const FVector2D& Start, const FVector2D& End);
    void PlaceWall(const FVector2D& Start, const FVector2D& End);
    void PlaceWindow(const FVector2D& Start, const FVector2D& End, float Thickness, int32 SectionCount);
    void PlaceSpawnPoint(const FVector2D& Location, float HeightOffset, float Yaw);
    void PlaceCubicle(const FVector2D& Center, float Yaw, FName ElementId = NAME_None, const FElementPropertyOverride* Override = nullptr);
    void PlaceCeilingLights(const FVector2D& Start, const FVector2D& End, const FVector2D& Spacing, const FVector2D& Padding, float DirectionYawDegrees);
    void PlaceDoor(const FOfficeElementDefinition& Element);
    void PlaceElevator(const FOfficeElementDefinition& Element, const FElementPropertyOverride* Override = nullptr);
    void PlaceRoomTone(const FOfficeElementDefinition& Element);

    UInstancedStaticMeshComponent* GetOrCreateISMC(UStaticMesh* Mesh, const FName& ComponentName, UMaterialInterface* OverrideMaterial = nullptr);
    void DestroySpawnedComponents();
    
    /** Convert a layout ID to a file path using convention: ID -> Layouts/ID.json */
    static FString ResolveLayoutPath(const FString& LayoutId);

    /** Cached element overrides data loaded from JSON (mutable for lazy-loading in const methods) */
    mutable FElementOverridesData ElementOverridesData;
    
    /** Currently active element override set IDs (mutable for lazy-loading in const methods) */
    mutable TArray<FName> ActiveOverrideSetIds;

    /** The currently active layout path (may be overridden by day schedule) */
    FString ActiveLayoutPath;

    UPROPERTY(VisibleAnywhere, Category = "Generation")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(EditAnywhere, Category = "Layout")
    FString LayoutFileRelativePath;

    UPROPERTY(EditAnywhere, Category = "Layout")
    bool bRegenerateOnConstruction = true;

    UPROPERTY(EditAnywhere, Category = "Layout")
    float FloorHeight = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "0.1"))
    float FloorThickness = 20.0f;

    UPROPERTY(EditAnywhere, Category = "Layout")
    float CeilingHeight = 320.0f;

    UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "0.1"))
    float CeilingThickness = 20.0f;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TObjectPtr<UStaticMesh> FloorMesh;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TObjectPtr<UMaterialInterface> FloorMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TObjectPtr<UStaticMesh> CeilingMesh;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TObjectPtr<UMaterialInterface> CeilingMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TObjectPtr<UStaticMesh> WallMesh;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TObjectPtr<UMaterialInterface> WallMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Modules", meta = (ClampMin = "0.1"))
    float WallThickness = 25.0f;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TObjectPtr<UStaticMesh> WindowMesh;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TObjectPtr<UMaterialInterface> WindowMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TObjectPtr<UStaticMesh> WindowFrameMesh;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TObjectPtr<UMaterialInterface> WindowFrameMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Modules", meta = (ClampMin = "1.0"))
    float WindowFrameThickness = 10.0f;

    UPROPERTY(EditAnywhere, Category = "Modules", meta = (ClampMin = "1.0"))
    float WindowFrameDepth = 20.0f;

    UPROPERTY(EditAnywhere, Category = "Modules", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WindowHeightRatio = 0.8f;

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    TObjectPtr<UStaticMesh> CubiclePartitionMesh;

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    TObjectPtr<UMaterialInterface> CubiclePartitionMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    TObjectPtr<UStaticMesh> CubicleDeskMesh;

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    TObjectPtr<UMaterialInterface> CubicleDeskMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    TObjectPtr<UStaticMesh> CubicleChairMesh;

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    TObjectPtr<UMaterialInterface> CubicleChairMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Cubicles", meta = (ClampMin = "10.0"))
    float CubiclePartitionHeight = 160.0f;

    UPROPERTY(EditAnywhere, Category = "Cubicles", meta = (ClampMin = "1.0"))
    float CubiclePartitionThickness = 10.0f;

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    float CubicleDeskHeightOffset = 75.0f;

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    FVector CubicleDeskScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    FVector2D CubicleDeskOffset = FVector2D(0.0f, 60.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    FRotator CubicleChairRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    FVector CubicleChairScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    FVector CubicleChairRelativeLocation = FVector(0.0f, 84.0f, 0.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles", meta = (ClampMin = "50.0"))
    float CubicleWidth = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Cubicles", meta = (ClampMin = "50.0"))
    float CubicleDepth = 250.0f;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Monitor Mesh"))
    TObjectPtr<UStaticMesh> CubicleComputerMesh;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Monitor Material Override"))
    TObjectPtr<UMaterialInterface> CubicleComputerMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Monitor Desk Offset"))
    FVector2D CubicleComputerOffset = FVector2D(0.0f, -45.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Monitor Height Offset"))
    float CubicleComputerHeightOffset = 95.0f;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Monitor Yaw Offset"))
    float CubicleComputerYawOffset = 180.0f;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Monitor Scale"))
    FVector CubicleComputerScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Monitor Screen Offset"))
    FVector MonitorScreenOffset = FVector(15.0f, 0.0f, 45.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Monitor Screen Rotation"))
    FRotator MonitorScreenRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Monitor Screen Size"))
    FVector2D MonitorScreenSize = FVector2D(48.0f, 27.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Show Interaction Debug"))
    bool bShowMonitorInteractionDebug = false;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Monitor Relative Location"))
    FVector CubicleComputerRelativeLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Monitor Relative Rotation"))
    FRotator CubicleComputerRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    TObjectPtr<UStaticMesh> CubicleKeyboardMesh;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    TObjectPtr<UMaterialInterface> CubicleKeyboardMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FVector CubicleKeyboardRelativeLocation = FVector(20.0f, 0.0f, -10.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FRotator CubicleKeyboardRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FVector CubicleKeyboardScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    TObjectPtr<UStaticMesh> CubicleMouseMesh;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    TObjectPtr<UMaterialInterface> CubicleMouseMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FVector CubicleMouseRelativeLocation = FVector(30.0f, -20.0f, -10.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FRotator CubicleMouseRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FVector CubicleMouseScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    TObjectPtr<UStaticMesh> CubicleDeskLampMesh;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    TObjectPtr<UMaterialInterface> CubicleDeskLampMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FVector CubicleDeskLampRelativeLocation = FVector(-15.0f, 25.0f, 0.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FRotator CubicleDeskLampRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FVector CubicleDeskLampScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    TObjectPtr<UStaticMesh> CubicleMousePadMesh;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    TObjectPtr<UMaterialInterface> CubicleMousePadMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FVector CubicleMousePadRelativeLocation = FVector(35.0f, -20.0f, -10.5f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FRotator CubicleMousePadRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FVector CubicleMousePadScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Tower Mesh"))
    TObjectPtr<UStaticMesh> CubicleComputerTowerMesh;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Tower Material Override"))
    TObjectPtr<UMaterialInterface> CubicleComputerTowerMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Tower Relative Location"))
    FVector CubicleComputerTowerRelativeLocation = FVector(-55.0f, -40.0f, -40.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Tower Relative Rotation"))
    FRotator CubicleComputerTowerRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Tower Scale"))
    FVector CubicleComputerTowerScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    TObjectPtr<UStaticMesh> CubiclePhoneMesh;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    TObjectPtr<UMaterialInterface> CubiclePhoneMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FVector CubiclePhoneRelativeLocation = FVector(-25.0f, 20.0f, -10.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FRotator CubiclePhoneRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FVector CubiclePhoneScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    TObjectPtr<UStaticMesh> CubicleNotepadMesh;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    TObjectPtr<UMaterialInterface> CubicleNotepadMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FVector CubicleNotepadRelativeLocation = FVector(10.0f, 25.0f, -10.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FRotator CubicleNotepadRelativeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    FVector CubicleNotepadScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (DisplayName = "Workstation Interaction Prompt"))
    FText WorkstationInteractionPrompt = NSLOCTEXT("ProceduralOfficeGenerator", "WorkstationPrompt", "Press E to use workstation");

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (MakeEditWidget = true))
    FTransform WorkstationInteractionTargetOffset = FTransform(FRotator(-5.0f, 180.0f, 0.0f), FVector(45.0f, 0.0f, 110.0f));

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (ClampMin = "0.1"))
    float WorkstationInteractionMoveDuration = 1.5f;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (ClampMin = "0.0"))
    float WorkstationInteractionArcHeight = 35.0f;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float WorkstationInteractionCurveBias = 0.35f;

    UPROPERTY(EditAnywhere, Category = "Cubicles|Workstation")
    bool bShowWorkstationTargetPreview = true;

    UPROPERTY(EditAnywhere, Category = "Lighting")
    TObjectPtr<UStaticMesh> CeilingLightMesh;

    UPROPERTY(EditAnywhere, Category = "Lighting")
    TObjectPtr<UMaterialInterface> CeilingLightMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Lighting")
    FVector CeilingLightScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Lighting")
    bool bSpawnCeilingLightComponents = true;

    UPROPERTY(EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.0"))
    float CeilingLightIntensity = 3000.0f;

    UPROPERTY(EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.0"))
    float CeilingLightAttenuationRadius = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Lighting")
    FLinearColor CeilingLightColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Lighting")
    bool bCeilingLightsCastShadows = true;

    UPROPERTY(EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.0"))
    float CeilingLightVerticalOffset = 20.0f;

    UPROPERTY(EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float CeilingRectLightBarnDoorAngle = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.0"))
    float CeilingRectLightBarnDoorLength = 20.0f;

    UPROPERTY(EditAnywhere, Category = "Door")
    TSubclassOf<AActor> DoorActorClass;

    UPROPERTY(EditAnywhere, Category = "Door")
    FTransform DoorActorOffset = FTransform::Identity;

    UPROPERTY(EditAnywhere, Category = "Door")
    FVector DoorActorScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Door", meta = (ClampMin = "0.0"))
    float DoorWallPadding = 50.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0"))
    float ElevatorWallPadding = 50.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator")
    float ElevatorWallInset = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator")
    TSubclassOf<AProceduralElevator> ElevatorActorClass;

    UPROPERTY(EditAnywhere, Category = "Elevator")
    FTransform ElevatorActorOffset = FTransform::Identity;

    UPROPERTY(EditAnywhere, Category = "Elevator")
    FVector ElevatorActorScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "1.0"))
    float ElevatorDefaultWidth = 240.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "1.0"))
    float ElevatorDefaultDepth = 240.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "1.0"))
    float ElevatorDefaultHeight = 240.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator")
    bool bSpawnElevatorLightComponents = true;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0"))
    float ElevatorLightIntensity = 3000.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0"))
    float ElevatorLightAttenuationRadius = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator")
    FLinearColor ElevatorLightColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Elevator")
    bool bElevatorLightsCastShadows = true;

    UPROPERTY(EditAnywhere, Category = "Elevator")
    float ElevatorLightVerticalOffset = 20.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0"))
    float ElevatorRectLightSourceWidth = 130.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0"))
    float ElevatorRectLightSourceHeight = 130.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float ElevatorRectLightBarnDoorAngle = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0"))
    float ElevatorRectLightBarnDoorLength = 20.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator")
    TObjectPtr<UStaticMesh> ElevatorShaftMesh;

    UPROPERTY(EditAnywhere, Category = "Elevator")
    TObjectPtr<UMaterialInterface> ElevatorShaftMaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.1"))
    float ElevatorShaftThickness = 25.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0"))
    float ElevatorShaftSidePadding = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0"))
    float ElevatorShaftDoorOffset = 20.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0"))
    float ElevatorShaftBackPadding = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0"))
    float ElevatorShaftHeightOverride = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator")
    float ElevatorShaftVerticalOffset = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0"))
    float ElevatorShaftTopHeight = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator", meta = (ClampMin = "0.0"))
    float ElevatorShaftBottomHeight = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator")
    float ElevatorTopWallPadding = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Elevator|Shaft")
    bool bGenerateElevatorShaftCaps = true;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInstancedStaticMeshComponent>> SpawnedInstancedComponents;

    UPROPERTY(EditAnywhere, Category = "Layout")
    FName SpawnPointTag = FName(TEXT("ProceduralSpawn"));

    UPROPERTY()
    TArray<TObjectPtr<class UChildActorComponent>> SpawnedChildActors;

    UPROPERTY(Transient)
    TArray<TObjectPtr<URectLightComponent>> SpawnedCeilingLights;

    UPROPERTY(Transient)
    TArray<TObjectPtr<URectLightComponent>> SpawnedElevatorLights;

    UPROPERTY(EditAnywhere, Category = "UI")
    TObjectPtr<UTextureRenderTarget2D> ScreenRenderTarget;

    UPROPERTY(EditAnywhere, Category = "Audio")
    FProceduralAudioRegistry AudioRegistry;

    UPROPERTY()
    TArray<TObjectPtr<class UAudioComponent>> SpawnedAudioComponents;

    UPROPERTY(VisibleAnywhere, Category = "UI")
    TObjectPtr<UInteractiveScreenComponent> MonitorScreenComponent;

    void InitializeMonitorScreen();
    void HandleActiveProgramChanged(FName ProgramId);
    void HandleDayChanged(int32 DayIndex, const struct FElevatorDayProgramEntry& Config);
    void PlayWorkstationBootSound(const FVector& Location);
    
    UInstancedStaticMeshComponent* ResolveComputerMeshComponent();
    void HideComputerHighlight();
    void RefreshWorkstationTargetPreview();
    void UpdateMonitorInteractionDebug();
    int32 FindClosestComputerInstance(const FVector& WorldPoint, float Radius) const;
    UStaticMeshComponent* GetOrCreateComputerHighlightProxy(UInstancedStaticMeshComponent* SourceComponent);

    /** Check if a workstation at a given instance index is powered on (based on element overrides) */
    bool IsWorkstationPoweredOn(int32 InstanceIndex) const;
    
    /** Check if an elevator with the given element ID is locked */
    bool IsElevatorLocked(FName ElementId) const;

    UPROPERTY()
    TMap<FName, TObjectPtr<UInstancedStaticMeshComponent>> InstancedCache;

    UPROPERTY(Transient)
    TObjectPtr<UInstancedStaticMeshComponent> ComputerMeshComponent;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> ComputerHighlightProxy;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> MonitorInteractionDebugProxy;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UArrowComponent>> WorkstationTargetVisualizers;

    FTransform PendingWorkstationViewTransform = FTransform::Identity;
    bool bHasPendingWorkstationViewTransform = false;

    int32 HoveredComputerInstanceIndex = INDEX_NONE;
    int32 CurrentInteractionInstanceIndex = INDEX_NONE;

    FDelegateHandle ProgramChangedHandle;
    FDelegateHandle DayChangedHandle;

    UPROPERTY(Transient)
    TSet<int32> BootedComputerIndicesThisDay;

    UPROPERTY(Transient)
    bool bBootPendingForCurrentInteraction = false;

    /** Maps workstation instance index to the element ID (for looking up overrides) */
    TMap<int32, FName> WorkstationInstanceToElementId;
    
    /** Maps elevator child actor component to its element ID */
    TMap<TObjectPtr<UChildActorComponent>, FName> ElevatorComponentToElementId;

    void NotifyComputerLookedAt(const UPrimitiveComponent* Component, int32 InstanceIndex);
};
