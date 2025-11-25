#include "Procedural/ProceduralOfficeGenerator.h"
#include "Procedural/ProceduralOfficeGenerator.Log.h"
#include "Procedural/ElementOverrides.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/EngineTypes.h"

void AProceduralOfficeGenerator::PlaceCubicle(const FVector2D &Center, float Yaw, FName ElementId, const FElementPropertyOverride* Override)
{
    if (!CubiclePartitionMesh)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Cubicle partition mesh not assigned."));
        return;
    }

    // Center is the anchor point at the middle of the back wall.
    // The cubicle extends forward (positive local Y) from this anchor.
    const float Width = FMath::Max(CubicleWidth, 50.0f);
    const float Depth = FMath::Max(CubicleDepth, 50.0f);
    const float HalfWidth = Width * 0.5f;
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

    // Back wall is at Y=0 (the anchor), side walls extend from Y=0 to Y=Depth
    const FVector2D LocalBackCenter(0.0f, 0.0f);
    const float SideCenterY = Depth * 0.5f;
    const FVector2D LocalLeftCenter(-HalfWidth, SideCenterY);
    const FVector2D LocalRightCenter(HalfWidth, SideCenterY);

    AddPartitionSegment(LocalBackCenter, Yaw, Width);
    AddPartitionSegment(LocalLeftCenter, Yaw + 90.0f, Depth);
    AddPartitionSegment(LocalRightCenter, Yaw + 90.0f, Depth);

    // Desk is placed at a fixed offset from the anchor (back wall center)
    const FVector2D LocalDeskOffset(CubicleDeskOffset.X, CubicleDeskOffset.Y);
    const FVector2D DeskOffset = Rotate2D(LocalDeskOffset);
    const FVector DeskLocation(Center.X + DeskOffset.X, Center.Y + DeskOffset.Y, FloorHeight + CubicleDeskHeightOffset);

    if (CubicleDeskMesh)
    {
        UInstancedStaticMeshComponent *DeskComponent = GetOrCreateISMC(CubicleDeskMesh.Get(), FName(TEXT("CubicleDesk")), CubicleDeskMaterialOverride.Get());
        if (DeskComponent)
        {
            const FTransform DeskTransform(FRotator(0.0f, Yaw, 0.0f), DeskLocation, CubicleDeskScale);
            DeskComponent->AddInstance(DeskTransform);
        }
    }

    if (CubicleChairMesh)
    {
        UInstancedStaticMeshComponent *ChairComponent = GetOrCreateISMC(CubicleChairMesh.Get(), FName(TEXT("CubicleChair")), CubicleChairMaterialOverride.Get());
        if (ChairComponent)
        {
            // Chair position is relative to back wall anchor
            const FVector2D LocalChairOffset(CubicleChairRelativeLocation.X, CubicleChairRelativeLocation.Y);
            const FVector2D ChairOffset = Rotate2D(LocalChairOffset);
            const FVector ChairLocation(Center.X + ChairOffset.X, Center.Y + ChairOffset.Y, FloorHeight + CubicleChairRelativeLocation.Z);
            const FRotator ChairRotation = FRotator(CubicleChairRotation.Pitch, Yaw + CubicleChairRotation.Yaw, CubicleChairRotation.Roll);
            const FTransform ChairTransform(ChairRotation, ChairLocation, CubicleChairScale);
            ChairComponent->AddInstance(ChairTransform);
        }
    }

    // Workstation dressing (monitor, keyboard, etc.) is positioned relative to the desk
    const FVector2D WorkstationLocalOffset(CubicleComputerOffset.X, CubicleComputerOffset.Y);
    const FVector2D WorkstationOffset = Rotate2D(WorkstationLocalOffset);
    const float WorkstationHeight = DeskLocation.Z + CubicleComputerHeightOffset;
    const FVector WorkstationLocation(DeskLocation.X + WorkstationOffset.X, DeskLocation.Y + WorkstationOffset.Y, WorkstationHeight);
    const float WorkstationYaw = Yaw + CubicleComputerYawOffset;
    const FTransform StationTransform(FRotator(0.0f, WorkstationYaw, 0.0f), WorkstationLocation, FVector::OneVector);

    // Track the instance index for the workstation monitor so we can look up overrides later
    int32 MonitorInstanceIndex = INDEX_NONE;

    auto AddAccessoryInstance = [&](UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& RelativeLocation, const FRotator& RelativeRotation, const FVector& RelativeScale, const FName& ComponentKey) -> int32
    {
        if (!Mesh)
        {
            return INDEX_NONE;
        }

        UInstancedStaticMeshComponent* Component = GetOrCreateISMC(Mesh, ComponentKey, Material);
        if (!Component)
        {
            return INDEX_NONE;
        }

        if (ComponentKey == AProceduralOfficeGenerator::WorkstationMonitorComponentKey)
        {
            ComputerMeshComponent = Component;
            Component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Component->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
            
            // Ensure the tag is always present, even if component was retrieved from cache
            if (!Component->ComponentTags.Contains(AProceduralOfficeGenerator::WorkstationMonitorTag))
            {
                Component->ComponentTags.AddUnique(AProceduralOfficeGenerator::WorkstationMonitorTag);
            }
        }

        const FTransform RelativeTransform(RelativeRotation, RelativeLocation, RelativeScale);
        const FTransform WorldTransform = RelativeTransform * StationTransform;
        return Component->AddInstance(WorldTransform);
    };

    MonitorInstanceIndex = AddAccessoryInstance(CubicleComputerMesh.Get(), CubicleComputerMaterialOverride.Get(), CubicleComputerRelativeLocation, CubicleComputerRelativeRotation, CubicleComputerScale, AProceduralOfficeGenerator::WorkstationMonitorComponentKey);
    
    // Set the power state custom data for this monitor instance
    // Custom data index 0: 1.0 = powered on (emissive), 0.0 = powered off (no emissive)
    if (MonitorInstanceIndex != INDEX_NONE && ComputerMeshComponent)
    {
        const bool bPoweredOn = Override ? Override->bPoweredOn : false;
        ComputerMeshComponent->SetCustomDataValue(MonitorInstanceIndex, 0, bPoweredOn ? 1.0f : 0.0f);
    }

    // Map the instance index to element ID so we can look up power state when interacting
    if (MonitorInstanceIndex != INDEX_NONE && !ElementId.IsNone())
    {
        WorkstationInstanceToElementId.Add(MonitorInstanceIndex, ElementId);
    }
    
    AddAccessoryInstance(CubicleKeyboardMesh.Get(), CubicleKeyboardMaterialOverride.Get(), CubicleKeyboardRelativeLocation, CubicleKeyboardRelativeRotation, CubicleKeyboardScale, FName(TEXT("CubicleKeyboard")));
    AddAccessoryInstance(CubicleMouseMesh.Get(), CubicleMouseMaterialOverride.Get(), CubicleMouseRelativeLocation, CubicleMouseRelativeRotation, CubicleMouseScale, FName(TEXT("CubicleMouse")));
    AddAccessoryInstance(CubicleDeskLampMesh.Get(), CubicleDeskLampMaterialOverride.Get(), CubicleDeskLampRelativeLocation, CubicleDeskLampRelativeRotation, CubicleDeskLampScale, FName(TEXT("CubicleDeskLamp")));
    AddAccessoryInstance(CubicleMousePadMesh.Get(), CubicleMousePadMaterialOverride.Get(), CubicleMousePadRelativeLocation, CubicleMousePadRelativeRotation, CubicleMousePadScale, FName(TEXT("CubicleMousePad")));
    AddAccessoryInstance(CubicleComputerTowerMesh.Get(), CubicleComputerTowerMaterialOverride.Get(), CubicleComputerTowerRelativeLocation, CubicleComputerTowerRelativeRotation, CubicleComputerTowerScale, FName(TEXT("CubicleComputerTower")));
    AddAccessoryInstance(CubiclePhoneMesh.Get(), CubiclePhoneMaterialOverride.Get(), CubiclePhoneRelativeLocation, CubiclePhoneRelativeRotation, CubiclePhoneScale, FName(TEXT("CubiclePhone")));
    AddAccessoryInstance(CubicleNotepadMesh.Get(), CubicleNotepadMaterialOverride.Get(), CubicleNotepadRelativeLocation, CubicleNotepadRelativeRotation, CubicleNotepadScale, FName(TEXT("CubicleNotepad")));
}
