#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralOfficeGenerator.generated.h"

UENUM(BlueprintType)
enum class EOfficeElementType : uint8
{
    Floor,
    Ceiling,
    Wall
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

    UPROPERTY(EditAnywhere, Category = "Layout")
    float CeilingHeight = 320.0f;

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

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInstancedStaticMeshComponent>> SpawnedInstancedComponents;

private:
    TMap<FName, UInstancedStaticMeshComponent*> InstancedCache;
};
