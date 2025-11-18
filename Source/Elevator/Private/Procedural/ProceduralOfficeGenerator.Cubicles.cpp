#include "Procedural/ProceduralOfficeGenerator.h"
#include "Procedural/ProceduralOfficeGenerator.Log.h"

#include "Components/InstancedStaticMeshComponent.h"

void AProceduralOfficeGenerator::PlaceCubicle(const FVector2D &Center, const FVector2D &Size, float Yaw)
{
    if (!CubiclePartitionMesh)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Cubicle partition mesh not assigned."));
        return;
    }

    const float Width = FMath::Max(Size.X, 50.0f);
    const float AdjustedWidth = FMath::Max(Width * CubiclePartitionWidthScale, 10.0f);
    const float Depth = FMath::Max(Size.Y, 50.0f);
    const float AdjustedDepth = FMath::Max(Depth * CubiclePartitionDepthScale, 10.0f);
    const float HalfDepth = Depth * 0.5f;
    const float HalfAdjustedDepth = AdjustedDepth * 0.5f;
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

    const FVector2D LocalBackCenter(0.0f, -HalfDepth);
    const float SideCenterY = -HalfDepth + HalfAdjustedDepth;
    const FVector2D LocalLeftCenter(-AdjustedWidth * 0.5f, SideCenterY);
    const FVector2D LocalRightCenter(AdjustedWidth * 0.5f, SideCenterY);

    AddPartitionSegment(LocalBackCenter, Yaw, AdjustedWidth);
    AddPartitionSegment(LocalLeftCenter, Yaw + 90.0f, AdjustedDepth);
    AddPartitionSegment(LocalRightCenter, Yaw + 90.0f, AdjustedDepth);

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
            const FVector2D LocalChairOffset(CubicleChairRelativeLocation.X, CubicleChairRelativeLocation.Y);
            const FVector2D ChairOffset = Rotate2D(LocalChairOffset);
            const FVector ChairLocation(Center.X + ChairOffset.X, Center.Y + ChairOffset.Y, FloorHeight + CubicleChairRelativeLocation.Z);
            const FRotator ChairRotation = FRotator(CubicleChairRotation.Pitch, Yaw + CubicleChairRotation.Yaw, CubicleChairRotation.Roll);
            const FTransform ChairTransform(ChairRotation, ChairLocation, CubicleChairScale);
            ChairComponent->AddInstance(ChairTransform);
        }
    }

    const float BackBoundary = -HalfDepth + 1.0f;
    const float FrontBoundary = -HalfDepth + AdjustedDepth - 1.0f;
    const float LocalComputerY = FMath::Clamp(CubicleComputerOffset.Y, BackBoundary, FrontBoundary);
    const FVector2D ComputerLocalOffset(CubicleComputerOffset.X, LocalComputerY);
    const FVector2D ComputerOffset = Rotate2D(ComputerLocalOffset);
    const FVector CorrectedComputerOffset(ComputerOffset.X, ComputerOffset.Y, 0.0f);
    const float ComputerHeight = FloorHeight + CubicleDeskHeightOffset + CubicleComputerHeightOffset;
    const FVector ComputerLocation(Center.X + CorrectedComputerOffset.X, Center.Y + CorrectedComputerOffset.Y, ComputerHeight);
    const float ComputerYaw = Yaw + CubicleComputerYawOffset;
    const FTransform StationTransform(FRotator(0.0f, ComputerYaw, 0.0f), ComputerLocation, FVector::OneVector);

    auto AddAccessoryInstance = [&](UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& RelativeLocation, const FRotator& RelativeRotation, const FVector& RelativeScale, const TCHAR* ComponentBaseName)
    {
        if (!Mesh)
        {
            return;
        }

        UInstancedStaticMeshComponent* Component = GetOrCreateISMC(Mesh, FName(ComponentBaseName), Material);
        if (!Component)
        {
            return;
        }

        const FTransform RelativeTransform(RelativeRotation, RelativeLocation, RelativeScale);
        const FTransform WorldTransform = RelativeTransform * StationTransform;
        Component->AddInstance(WorldTransform);
    };

    AddAccessoryInstance(CubicleComputerMesh.Get(), CubicleComputerMaterialOverride.Get(), CubicleComputerRelativeLocation, CubicleComputerRelativeRotation, CubicleComputerScale, TEXT("CubicleComputer"));
    AddAccessoryInstance(CubicleKeyboardMesh.Get(), CubicleKeyboardMaterialOverride.Get(), CubicleKeyboardRelativeLocation, CubicleKeyboardRelativeRotation, CubicleKeyboardScale, TEXT("CubicleKeyboard"));
    AddAccessoryInstance(CubicleMouseMesh.Get(), CubicleMouseMaterialOverride.Get(), CubicleMouseRelativeLocation, CubicleMouseRelativeRotation, CubicleMouseScale, TEXT("CubicleMouse"));
    AddAccessoryInstance(CubicleDeskLampMesh.Get(), CubicleDeskLampMaterialOverride.Get(), CubicleDeskLampRelativeLocation, CubicleDeskLampRelativeRotation, CubicleDeskLampScale, TEXT("CubicleDeskLamp"));
    AddAccessoryInstance(CubicleMousePadMesh.Get(), CubicleMousePadMaterialOverride.Get(), CubicleMousePadRelativeLocation, CubicleMousePadRelativeRotation, CubicleMousePadScale, TEXT("CubicleMousePad"));
    AddAccessoryInstance(CubicleComputerTowerMesh.Get(), CubicleComputerTowerMaterialOverride.Get(), CubicleComputerTowerRelativeLocation, CubicleComputerTowerRelativeRotation, CubicleComputerTowerScale, TEXT("CubicleComputerTower"));
    AddAccessoryInstance(CubiclePhoneMesh.Get(), CubiclePhoneMaterialOverride.Get(), CubiclePhoneRelativeLocation, CubiclePhoneRelativeRotation, CubiclePhoneScale, TEXT("CubiclePhone"));
    AddAccessoryInstance(CubicleNotepadMesh.Get(), CubicleNotepadMaterialOverride.Get(), CubicleNotepadRelativeLocation, CubicleNotepadRelativeRotation, CubicleNotepadScale, TEXT("CubicleNotepad"));
}
