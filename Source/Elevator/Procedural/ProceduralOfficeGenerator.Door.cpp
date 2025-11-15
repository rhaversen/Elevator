#include "../ProceduralOfficeGenerator.h"
#include "ProceduralOfficeGenerator.Helpers.h"
#include "ProceduralOfficeGenerator.Log.h"

#include "Components/ChildActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

void AProceduralOfficeGenerator::PlaceDoor(const FOfficeElementDefinition &Element)
{
    float TotalSpan = 0.0f;
    const FVector2D UnitDirection2D = ProceduralOffice::Utils::CalculateUnitDirection(Element.Start, Element.End, TotalSpan);
    if (TotalSpan <= KINDA_SMALL_NUMBER)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Door element requires unique start and end points."));
        return;
    }

    const FVector2D Center2D = (Element.Start + Element.End) * 0.5f;
    const float BaseYaw = ProceduralOffice::Utils::DirectionToYawDegrees(UnitDirection2D);

    const FRotator BaseRotation(0.0f, BaseYaw + 90.0f, 0.0f);
    const FVector Forward = BaseRotation.RotateVector(FVector::ForwardVector);
    const FVector Right = BaseRotation.RotateVector(FVector::RightVector);
    const FVector BaseLocation(Center2D.X, Center2D.Y, FloorHeight + Element.HeightOffset);

    auto MakeWorldLocation = [&](const FVector &LocalOffset) -> FVector
    {
        return BaseLocation + Forward * LocalOffset.X + Right * LocalOffset.Y + FVector::UpVector * LocalOffset.Z;
    };

    if (DoorActorClass)
    {
        const FVector LocalDoorLocation = DoorActorOffset.GetTranslation();
        const FVector DoorLocation = MakeWorldLocation(LocalDoorLocation);
        const FRotator DoorRotation = (BaseRotation + DoorActorOffset.GetRotation().Rotator()).GetNormalized();
        const FVector DoorScale = DoorActorOffset.GetScale3D() * DoorActorScale;

        const FName ComponentName = MakeUniqueObjectName(this, UChildActorComponent::StaticClass(), FName(TEXT("ProceduralDoor")));
        if (UChildActorComponent *DoorComponent = NewObject<UChildActorComponent>(this, ComponentName))
        {
            DoorComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
            DoorComponent->SetMobility(EComponentMobility::Static);
            DoorComponent->SetupAttachment(Root);
            DoorComponent->SetChildActorClass(DoorActorClass);
            DoorComponent->RegisterComponent();
            DoorComponent->SetWorldTransform(FTransform(DoorRotation, DoorLocation, DoorScale));
            DoorComponent->UpdateComponentToWorld();

            if (AActor *DoorActor = DoorComponent->GetChildActor())
            {
                DoorActor->SetActorTransform(DoorComponent->GetComponentTransform());
            }

            SpawnedChildActors.Add(DoorComponent);
        }
    }

    if (!WallMesh)
    {
        return;
    }

    const float RequestedDoorWidth = Element.Dimensions.X;
    float EffectiveDoorWidth = RequestedDoorWidth > KINDA_SMALL_NUMBER ? RequestedDoorWidth : TotalSpan * 0.5f;
    EffectiveDoorWidth = FMath::Clamp(EffectiveDoorWidth, KINDA_SMALL_NUMBER, TotalSpan);

    const FVector2D HalfDoorOffset2D = UnitDirection2D * (EffectiveDoorWidth * 0.5f);
    const FVector2D LeftEdge2D = Center2D - HalfDoorOffset2D;
    const FVector2D RightEdge2D = Center2D + HalfDoorOffset2D;

    const float RequestedPadding = FMath::Max(DoorWallPadding, 0.0f);
    const float MaxAllowedPadding = EffectiveDoorWidth * 0.49f;
    const float AppliedPadding = FMath::Clamp(RequestedPadding, 0.0f, MaxAllowedPadding);
    const FVector2D PaddingOffset2D = UnitDirection2D * AppliedPadding;

    const FVector2D AdjustedLeftEdge2D = LeftEdge2D + PaddingOffset2D;
    const FVector2D AdjustedRightEdge2D = RightEdge2D - PaddingOffset2D;
    const float AdjustedDoorWidth = FMath::Max((AdjustedRightEdge2D - AdjustedLeftEdge2D).Size(), KINDA_SMALL_NUMBER);

    const float DoorWidthScale = ProceduralOffice::Utils::ClampPositive(DoorActorScale.Y);
    const float FinalDoorOpeningWidth = FMath::Clamp(AdjustedDoorWidth * DoorWidthScale, KINDA_SMALL_NUMBER, TotalSpan);
    const FVector2D HalfFinalOffset2D = UnitDirection2D * (FinalDoorOpeningWidth * 0.5f);
    const FVector2D ClearLeftEdge2D = Center2D - HalfFinalOffset2D;
    const FVector2D ClearRightEdge2D = Center2D + HalfFinalOffset2D;

    if ((ClearLeftEdge2D - Element.Start).Size() > KINDA_SMALL_NUMBER)
    {
        PlaceWall(Element.Start, ClearLeftEdge2D);
    }

    if ((Element.End - ClearRightEdge2D).Size() > KINDA_SMALL_NUMBER)
    {
        PlaceWall(ClearRightEdge2D, Element.End);
    }

    const float WallHeight = ProceduralOffice::Utils::ComputeWallHeight(FloorHeight, CeilingHeight, 0.0f);
    const float RequestedDoorHeight = Element.Height;
    const float BaseDoorHeight = RequestedDoorHeight > KINDA_SMALL_NUMBER ? FMath::Min(RequestedDoorHeight, WallHeight) : FMath::Min(WallHeight * 0.75f, WallHeight);
    const float DoorHeightScale = ProceduralOffice::Utils::ClampPositive(DoorActorScale.Z);
    const float FinalDoorHeight = FMath::Min(BaseDoorHeight * DoorHeightScale, WallHeight);
    const float FillHeight = WallHeight - FinalDoorHeight;

    if (FillHeight > KINDA_SMALL_NUMBER)
    {
        UInstancedStaticMeshComponent *FillComponent = GetOrCreateISMC(WallMesh.Get(), FName(TEXT("DoorTopFill")), WallMaterialOverride.Get());
        if (FillComponent)
        {
            const FVector MeshSize = WallMesh->GetBounds().BoxExtent * 2.0f;
            const float DesiredThickness = ProceduralOffice::Utils::ClampPositive(WallThickness);
            const float ScaleX = FinalDoorOpeningWidth / FMath::Max(MeshSize.X, KINDA_SMALL_NUMBER);
            const float ScaleY = DesiredThickness / FMath::Max(MeshSize.Y, KINDA_SMALL_NUMBER);
            const float ScaleZ = FillHeight / FMath::Max(MeshSize.Z, KINDA_SMALL_NUMBER);

            const FVector FillLocation = MakeWorldLocation(FVector(0.0f, 0.0f, FinalDoorHeight + FillHeight * 0.5f));
            const FRotator WallFillRotation(0.0f, BaseYaw, 0.0f);
            const FTransform FillTransform(WallFillRotation, FillLocation, FVector(ScaleX, ScaleY, ScaleZ));
            FillComponent->AddInstance(FillTransform);
        }
    }
}
