#include "../ProceduralOfficeGenerator.h"
#include "ProceduralOfficeGenerator.Log.h"

#include "Components/InstancedStaticMeshComponent.h"

void AProceduralOfficeGenerator::PlaceCubicle(const FVector2D &Center, const FVector2D &Size, float Yaw)
{
    if (!CubiclePartitionMesh)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Cubicle partition mesh not assigned."));
        return;
    }

    const float Width = FMath::Max(Size.X, 50.0f);
    const float Depth = FMath::Max(Size.Y, 50.0f);
    const float PartitionHeight = FMath::Max(CubiclePartitionHeight, 10.0f);
    const float PartitionThickness = FMath::Max(CubiclePartitionThickness, 1.0f);

    const float CosYaw = FMath::Cos(FMath::DegreesToRadians(Yaw));
    const float SinYaw = FMath::Sin(FMath::DegreesToRadians(Yaw));
    const auto Rotate2D = [&](const FVector2D &Local) -> FVector2D
    {
        return FVector2D(Local.X * CosYaw - Local.Y * SinYaw, Local.X * SinYaw + Local.Y * CosYaw);
    };

    auto AddPartitionSegment = [&](const FVector2D &LocalCenter, float SegmentYawDegrees, float SegmentLength)
    {
        UInstancedStaticMeshComponent *Component = GetOrCreateISMC(CubiclePartitionMesh.Get(), FName(TEXT("CubiclePartition")), CubiclePartitionMaterialOverride.Get());
        if (!Component)
        {
            return;
        }

        const FVector2D Offset = Rotate2D(LocalCenter);
        const FVector SegmentCenter(Center.X + Offset.X, Center.Y + Offset.Y, FloorHeight + PartitionHeight * 0.5f);

        const FVector MeshSize = CubiclePartitionMesh->GetBounds().BoxExtent * 2.0f;
        const FVector Scale(
            SegmentLength / FMath::Max(MeshSize.X, KINDA_SMALL_NUMBER),
            PartitionThickness / FMath::Max(MeshSize.Y, KINDA_SMALL_NUMBER),
            PartitionHeight / FMath::Max(MeshSize.Z, KINDA_SMALL_NUMBER));

        const FTransform InstanceTransform(FRotator(0.0f, SegmentYawDegrees, 0.0f), SegmentCenter, Scale);
        Component->AddInstance(InstanceTransform);
    };

    const FVector2D LocalBackCenter(0.0f, -Depth * 0.5f);
    const FVector2D LocalLeftCenter(-Width * 0.5f, 0.0f);
    const FVector2D LocalRightCenter(Width * 0.5f, 0.0f);

    AddPartitionSegment(LocalBackCenter, Yaw, Width);
    AddPartitionSegment(LocalLeftCenter, Yaw + 90.0f, Depth);
    AddPartitionSegment(LocalRightCenter, Yaw + 90.0f, Depth);

    if (CubicleDeskMesh)
    {
        UInstancedStaticMeshComponent *DeskComponent = GetOrCreateISMC(CubicleDeskMesh.Get(), FName(TEXT("CubicleDesk")), CubicleDeskMaterialOverride.Get());
        if (DeskComponent)
        {
            const float ClampedRatio = FMath::Clamp(CubicleDeskBackOffsetRatio, 0.0f, 0.45f);
            const FVector2D LocalDeskOffset(0.0f, -Depth * (0.5f - ClampedRatio));
            const FVector2D DeskOffset = Rotate2D(LocalDeskOffset);
            const FVector DeskLocation(Center.X + DeskOffset.X, Center.Y + DeskOffset.Y, FloorHeight + CubicleDeskHeightOffset);
            const FTransform DeskTransform(FRotator(0.0f, Yaw, 0.0f), DeskLocation, CubicleDeskScale);
            DeskComponent->AddInstance(DeskTransform);
        }
    }

    if (CubicleChairMesh)
    {
        UInstancedStaticMeshComponent *ChairComponent = GetOrCreateISMC(CubicleChairMesh.Get(), FName(TEXT("CubicleChair")), CubicleChairMaterialOverride.Get());
        if (ChairComponent)
        {
            const float ClampedRatio = FMath::Clamp(CubicleChairFrontOffsetRatio, 0.0f, 0.45f);
            const FVector2D LocalChairOffset(0.0f, Depth * (0.5f - ClampedRatio));
            const FVector2D ChairOffset = Rotate2D(LocalChairOffset);
            const FVector ChairLocation(Center.X + ChairOffset.X, Center.Y + ChairOffset.Y, FloorHeight);
            const FRotator ChairRotation = FRotator(CubicleChairRotation.Pitch, Yaw + CubicleChairRotation.Yaw, CubicleChairRotation.Roll);
            const FTransform ChairTransform(ChairRotation, ChairLocation, CubicleChairScale);
            ChairComponent->AddInstance(ChairTransform);
        }
    }
}
