#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralOfficeGenerator.generated.h"

class UChildActorComponent;
class URectLightComponent;

UENUM(BlueprintType)
enum class EOfficeElementType : uint8
{
    Floor,
    Ceiling,
    Wall,
    Window,
    SpawnPoint,
    Cubicle,
    CeilingLight
};

USTRUCT(BlueprintType)
struct FOfficeElementDefinition
{
    GENERATED_BODY()

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
class ELEVATOR_API AProceduralOfficeGenerator : public AActor
{
    GENERATED_BODY()

public:
    AProceduralOfficeGenerator();

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

protected:
    bool LoadLayoutData(FOfficeLayout& OutLayout) const;
    void BuildFromLayout(const FOfficeLayout& Layout);
    void BuildElement(const FOfficeElementDefinition& Element);
    void PlaceSurface(EOfficeElementType Type, const FVector2D& Start, const FVector2D& End);
    void PlaceWall(const FVector2D& Start, const FVector2D& End, float Thickness);
    void PlaceWindow(const FVector2D& Start, const FVector2D& End, float Thickness, int32 SectionCount);
    void PlaceSpawnPoint(const FVector2D& Location, float HeightOffset, float Yaw);
    void PlaceCubicle(const FVector2D& Center, const FVector2D& Size, float Yaw);
    void PlaceCeilingLights(const FVector2D& Start, const FVector2D& End, const FVector2D& Spacing, const FVector2D& Padding);

    UInstancedStaticMeshComponent* GetOrCreateISMC(UStaticMesh* Mesh, const FName& ComponentName, UMaterialInterface* OverrideMaterial = nullptr);
    void DestroySpawnedComponents();

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

    UPROPERTY(EditAnywhere, Category = "Cubicles", meta = (ClampMin = "0.0", ClampMax = "0.45"))
    float CubicleDeskBackOffsetRatio = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    FRotator CubicleChairRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "Cubicles")
    FVector CubicleChairScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Cubicles", meta = (ClampMin = "0.0", ClampMax = "0.45"))
    float CubicleChairFrontOffsetRatio = 0.15f;

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

    UPROPERTY(EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.0"))
    float CeilingRectLightSourceWidth = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.0"))
    float CeilingRectLightSourceHeight = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float CeilingRectLightBarnDoorAngle = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Lighting", meta = (ClampMin = "0.0"))
    float CeilingRectLightBarnDoorLength = 20.0f;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInstancedStaticMeshComponent>> SpawnedInstancedComponents;

    UPROPERTY(EditAnywhere, Category = "Layout")
    FName SpawnPointTag = FName(TEXT("ProceduralSpawn"));

    UPROPERTY(Transient)
    TArray<TObjectPtr<class UChildActorComponent>> SpawnedChildActors;

    UPROPERTY(Transient)
    TArray<TObjectPtr<URectLightComponent>> SpawnedCeilingLights;

private:
    TMap<FName, UInstancedStaticMeshComponent*> InstancedCache;
};
