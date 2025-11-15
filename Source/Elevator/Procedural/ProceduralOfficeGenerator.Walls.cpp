#include "../ProceduralOfficeGenerator.h"
#include "ProceduralOfficeGenerator.Helpers.h"
#include "ProceduralOfficeGenerator.Log.h"

#include "Components/InstancedStaticMeshComponent.h"

void AProceduralOfficeGenerator::PlaceWall(const FVector2D &Start, const FVector2D &End)
{
    if (!WallMesh)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Wall mesh not assigned."));
        return;
    }

    float Length = 0.0f;
    const FVector2D UnitDirection = ProceduralOffice::Utils::CalculateUnitDirection(Start, End, Length);
    if (Length <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    UInstancedStaticMeshComponent *Component = GetOrCreateISMC(WallMesh.Get(), FName(TEXT("Wall")), WallMaterialOverride.Get());
    if (!Component)
    {
        return;
    }

    const float WallHeight = ProceduralOffice::Utils::ComputeWallHeight(FloorHeight, CeilingHeight);
    const FVector MeshSize = WallMesh->GetBounds().BoxExtent * 2.0f;
    const float ScaleX = Length / FMath::Max(MeshSize.X, KINDA_SMALL_NUMBER);
    const float ScaleZ = WallHeight / FMath::Max(MeshSize.Z, KINDA_SMALL_NUMBER);
    const float DesiredThickness = ProceduralOffice::Utils::ClampPositive(WallThickness);
    const float ScaleY = DesiredThickness / FMath::Max(MeshSize.Y, KINDA_SMALL_NUMBER);

    const FVector2D Midpoint2D = (Start + End) * 0.5f;
    const FVector Location(Midpoint2D.X, Midpoint2D.Y, FloorHeight + WallHeight * 0.5f);
    const float Yaw = ProceduralOffice::Utils::DirectionToYawDegrees(UnitDirection);
    const FTransform InstanceTransform(FRotator(0.0f, Yaw, 0.0f), Location, FVector(ScaleX, ScaleY, ScaleZ));
    Component->AddInstance(InstanceTransform);
}
