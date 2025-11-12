#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralOfficeGenerator.generated.h"

UENUM(BlueprintType)
enum class EProceduralRoomType : uint8
{
    OpenWorkspace,
    PrivateOffice,
    Meeting,
    Corridor,
    Utility
};

USTRUCT(BlueprintType)
struct FProceduralDoorwayDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    FVector2D Location = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "60.0"))
    float Width = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    float FacingYaw = 0.0f;
};

USTRUCT(BlueprintType)
struct FProceduralRoomDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    FVector2D Origin = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "100.0"))
    FVector2D Size = FVector2D(800.0f, 800.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    EProceduralRoomType Usage = EProceduralRoomType::OpenWorkspace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    FIntPoint CubicleGrid = FIntPoint::ZeroValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    TArray<FProceduralDoorwayDefinition> Doors;
};

USTRUCT(BlueprintType)
struct FProceduralOfficeLayout
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    TArray<FProceduralRoomDefinition> Rooms;
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
    bool LoadLayoutData(FProceduralOfficeLayout& OutLayout) const;
    void BuildFromLayout(const FProceduralOfficeLayout& Layout);
    void BuildRoom(const FProceduralRoomDefinition& Room, FRandomStream& Rng);
    void PlaceWalls(const FVector& RoomOrigin, const FVector2D& Size, float Height, FRandomStream& Rng, const TArray<FProceduralDoorwayDefinition>& Doors);
    void PlaceCubicles(const FProceduralRoomDefinition& Room, FRandomStream& Rng);
    void SpawnProps(const FProceduralRoomDefinition& Room, FRandomStream& Rng);

    UInstancedStaticMeshComponent* GetOrCreateISMC(UStaticMesh* Mesh, const FName& ComponentName, const TArray<UMaterialInterface*>& MaterialVariants, int32 VariantIndex);
    void DestroySpawnedComponents();

    UPROPERTY(VisibleAnywhere, Category = "Generation")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(EditAnywhere, Category = "Layout")
    FString LayoutFileRelativePath;

    UPROPERTY(EditAnywhere, Category = "Layout")
    bool bRegenerateOnConstruction = true;

    UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "1"))
    int32 RandomSeed = 1337;

    UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "50.0"))
    float WallModuleLength = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "200.0"))
    float CeilingHeight = 320.0f;

    UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "0.0"))
    float CubiclePadding = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Interior Dressing", meta = (ClampMin = "0"))
    int32 MaxPropsPerRoom = 4;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TArray<TObjectPtr<UStaticMesh>> WallSegmentMeshes;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TArray<TObjectPtr<UMaterialInterface>> WallMaterialVariants;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TArray<TObjectPtr<UStaticMesh>> DoorwayMeshes;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TArray<TObjectPtr<UStaticMesh>> CubicleMeshes;

    UPROPERTY(EditAnywhere, Category = "Modules")
    TArray<TObjectPtr<UMaterialInterface>> CubicleMaterialVariants;

    UPROPERTY(EditAnywhere, Category = "Interior Dressing")
    TArray<TSubclassOf<AActor>> PropPrefabs;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInstancedStaticMeshComponent>> SpawnedInstancedComponents;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AActor>> SpawnedProps;

private:
    TMap<FName, UInstancedStaticMeshComponent*> InstancedCache;
};
