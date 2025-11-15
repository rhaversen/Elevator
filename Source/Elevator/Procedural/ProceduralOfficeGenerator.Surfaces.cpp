#include "../ProceduralOfficeGenerator.h"
#include "ProceduralOfficeGenerator.Helpers.h"
#include "ProceduralOfficeGenerator.Log.h"

#include "Components/InstancedStaticMeshComponent.h"

void AProceduralOfficeGenerator::PlaceSurface(EOfficeElementType Type, const FVector2D &Start, const FVector2D &End)
{
    const bool bIsFloor = Type == EOfficeElementType::Floor;
    UStaticMesh *Mesh = bIsFloor ? FloorMesh.Get() : CeilingMesh.Get();
    if (!Mesh)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("%s mesh not assigned."), bIsFloor ? TEXT("Floor") : TEXT("Ceiling"));
        return;
    }

    const float MinX = FMath::Min(Start.X, End.X);
    const float MaxX = FMath::Max(Start.X, End.X);
    const float MinY = FMath::Min(Start.Y, End.Y);
    const float MaxY = FMath::Max(Start.Y, End.Y);

    const float SizeX = FMath::Max(MaxX - MinX, 1.0f);
    const float SizeY = FMath::Max(MaxY - MinY, 1.0f);
    const float Thickness = ProceduralOffice::Utils::ClampPositive(bIsFloor ? FloorThickness : CeilingThickness, 1.0f);
    const FVector MeshSize = Mesh->GetBounds().BoxExtent * 2.0f;

    FVector Center((MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f, 0.0f);
    if (bIsFloor)
    {
        Center.Z = FloorHeight - (Thickness * 0.5f);
    }
    else
    {
        Center.Z = CeilingHeight + (Thickness * 0.5f);
    }

    UMaterialInterface *Material = bIsFloor ? FloorMaterialOverride.Get() : CeilingMaterialOverride.Get();
    UInstancedStaticMeshComponent *Component = GetOrCreateISMC(Mesh, bIsFloor ? FName(TEXT("Floor")) : FName(TEXT("Ceiling")), Material);
    if (!Component)
    {
        return;
    }

    const float ScaleX = SizeX / FMath::Max(MeshSize.X, KINDA_SMALL_NUMBER);
    const float ScaleY = SizeY / FMath::Max(MeshSize.Y, KINDA_SMALL_NUMBER);
    const float ScaleZ = Thickness / FMath::Max(MeshSize.Z, KINDA_SMALL_NUMBER);
    const FTransform InstanceTransform(FRotator::ZeroRotator, Center, FVector(ScaleX, ScaleY, ScaleZ));
    Component->AddInstance(InstanceTransform);
}
