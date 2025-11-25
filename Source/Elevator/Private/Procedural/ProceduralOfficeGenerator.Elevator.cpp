#include "Procedural/ProceduralOfficeGenerator.h"
#include "Procedural/ProceduralOfficeGenerator.Helpers.h"
#include "Procedural/ProceduralOfficeGenerator.Log.h"
#include "Procedural/ProceduralElevator.h"
#include "Procedural/ElementOverrides.h"

#include "Components/ChildActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SceneComponent.h"

void AProceduralOfficeGenerator::PlaceElevator(const FOfficeElementDefinition &Element, const FElementPropertyOverride* Override)
{
    // Log override information if present
    if (Override)
    {
        UE_LOG(LogProceduralOffice, Log, TEXT("Elevator '%s' override: bLocked=%s bPoweredOn=%s"),
            *Element.Id.ToString(),
            Override->bLocked ? TEXT("true") : TEXT("false"),
            Override->bPoweredOn ? TEXT("true") : TEXT("false"));
    }
    
    float TotalSpan = 0.0f;
    FVector2D UnitDirection2D = ProceduralOffice::Utils::CalculateUnitDirection(Element.Start, Element.End, TotalSpan);
    FVector2D Center2D = Element.Start;
    float YawDegrees = Element.Yaw;

    FVector2D WallStart2D = Element.Start;
    FVector2D WallEnd2D = Element.End;

    const float ApproxWidth = (Element.Dimensions.X > KINDA_SMALL_NUMBER) ? Element.Dimensions.X : ElevatorDefaultWidth;
    const float ApproxDepth = (Element.Dimensions.Y > KINDA_SMALL_NUMBER) ? Element.Dimensions.Y : ElevatorDefaultDepth;
    const float ApproxHeight = (Element.Height > KINDA_SMALL_NUMBER) ? Element.Height : ElevatorDefaultHeight;

    if (TotalSpan > KINDA_SMALL_NUMBER)
    {
        Center2D = (Element.Start + Element.End) * 0.5f;
        YawDegrees = ProceduralOffice::Utils::DirectionToYawDegrees(UnitDirection2D);
    }
    else
    {
        const float YawRadians = FMath::DegreesToRadians(YawDegrees);
        UnitDirection2D = FVector2D(FMath::Cos(YawRadians), FMath::Sin(YawRadians));

        TotalSpan = FMath::Max(ApproxWidth, 1.0f);
        const FVector2D HalfSpan = UnitDirection2D * (TotalSpan * 0.5f);
        WallStart2D = Center2D - HalfSpan;
        WallEnd2D = Center2D + HalfSpan;
    }

    const FVector2D PerpDirection2D(-UnitDirection2D.Y, UnitDirection2D.X);
    const FVector ElevatorInsetOffset(PerpDirection2D.X * ElevatorWallInset, PerpDirection2D.Y * ElevatorWallInset, 0.0f);

    const FVector BaseLocation = FVector(Center2D.X, Center2D.Y, FloorHeight + Element.HeightOffset) + ElevatorInsetOffset;
    const FRotator BaseRotation(0.0f, YawDegrees, 0.0f);
    const FTransform BaseTransform(BaseRotation, BaseLocation, FVector::OneVector);

    if (ElevatorActorClass)
    {
        const FTransform SpawnTransform = ElevatorActorOffset * BaseTransform;
        const FVector SpawnLocation = SpawnTransform.GetTranslation();
        const FRotator SpawnRotation = SpawnTransform.GetRotation().Rotator();
        const FVector OffsetScale = SpawnTransform.GetScale3D();

        const FName ComponentName = MakeUniqueObjectName(this, UChildActorComponent::StaticClass(), FName(TEXT("ProceduralElevator")));
        if (UChildActorComponent *ElevatorComponent = NewObject<UChildActorComponent>(this, ComponentName))
        {
            ElevatorComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
            ElevatorComponent->SetMobility(EComponentMobility::Movable);
            ElevatorComponent->SetupAttachment(Root);
            ElevatorComponent->SetChildActorClass(ElevatorActorClass);
            ElevatorComponent->RegisterComponent();

            FVector BlueprintScale = FVector::OneVector;
            if (AActor *ExistingChild = ElevatorComponent->GetChildActor())
            {
                BlueprintScale = ExistingChild->GetActorScale3D();
            }

            const FVector DesiredScale = BlueprintScale * OffsetScale * ElevatorActorScale;
            ElevatorComponent->SetWorldLocationAndRotation(SpawnLocation, SpawnRotation);
            ElevatorComponent->SetWorldScale3D(DesiredScale);
            ElevatorComponent->UpdateComponentToWorld();

            if (AActor *ChildActor = ElevatorComponent->GetChildActor())
            {
                ChildActor->SetActorTransform(ElevatorComponent->GetComponentTransform());
                
                // Apply locked state and element ID from override (elevators are locked by default)
                if (AProceduralElevator* Elevator = Cast<AProceduralElevator>(ChildActor))
                {
                    // Store the element ID as a component tag on the ChildActorComponent
                    // Component tags ARE preserved through PIE duplication, unlike child actor properties
                    if (!Element.Id.IsNone())
                    {
                        ElevatorComponent->ComponentTags.Add(Element.Id);
                    }
                    
                    // Also set it directly on the elevator for editor use
                    Elevator->SetElementId(Element.Id);
                    
                    // bLocked defaults to true in FElementPropertyOverride, so unlocked elevators need explicit bLocked=false
                    const bool bShouldBeLocked = Override ? Override->bLocked : true;
                    Elevator->SetLocked(bShouldBeLocked);
                }
            }

            SpawnedChildActors.Add(ElevatorComponent);
            
            // Track elevator component to element ID mapping for override lookups
            if (!Element.Id.IsNone())
            {
                ElevatorComponentToElementId.Add(ElevatorComponent, Element.Id);
            }
        }
    }
    else
    {
        UE_LOG(LogProceduralOffice, Verbose, TEXT("Elevator actor class not set; skipping cab actor spawn."));
    }

    if (bSpawnElevatorLightComponents && ElevatorLightIntensity > KINDA_SMALL_NUMBER)
    {
        URectLightComponent *NewLight = NewObject<URectLightComponent>(this);
        if (NewLight)
        {
            NewLight->SetMobility(EComponentMobility::Stationary);
            NewLight->SetIntensity(ElevatorLightIntensity);
            NewLight->SetAttenuationRadius(FMath::Max(1.0f, ElevatorLightAttenuationRadius));
            NewLight->SetLightColor(ElevatorLightColor);
            NewLight->SetCastShadows(bElevatorLightsCastShadows);
            NewLight->SetSourceWidth(FMath::Max(1.0f, ElevatorRectLightSourceWidth));
            NewLight->SetSourceHeight(FMath::Max(1.0f, ElevatorRectLightSourceHeight));
            NewLight->SetBarnDoorAngle(FMath::Clamp(ElevatorRectLightBarnDoorAngle, 0.0f, 90.0f));
            NewLight->SetBarnDoorLength(FMath::Max(0.0f, ElevatorRectLightBarnDoorLength));
            NewLight->SetupAttachment(Root);
            NewLight->RegisterComponent();

            float ElevatorTop = CeilingHeight;
            if (ApproxHeight > KINDA_SMALL_NUMBER)
            {
                ElevatorTop = FloorHeight + Element.HeightOffset + ApproxHeight;
            }

            const FVector LightLocation = FVector(Center2D.X, Center2D.Y, ElevatorTop - ElevatorLightVerticalOffset) + ElevatorInsetOffset;
            NewLight->SetWorldLocation(LightLocation);
            NewLight->SetWorldRotation(FRotator(-90.0f, 90.0f, 0.0f));
            SpawnedElevatorLights.Add(NewLight);
        }
    }

    if (!WallMesh)
    {
        return;
    }

    float RequestedOpeningWidth = Element.Dimensions.X;
    if (RequestedOpeningWidth <= KINDA_SMALL_NUMBER)
    {
        RequestedOpeningWidth = TotalSpan;
    }
    RequestedOpeningWidth = FMath::Max(RequestedOpeningWidth, KINDA_SMALL_NUMBER);

    if (TotalSpan > KINDA_SMALL_NUMBER)
    {
        RequestedOpeningWidth = FMath::Min(RequestedOpeningWidth, TotalSpan);
    }

    const FVector2D HalfOpeningOffset = UnitDirection2D * (RequestedOpeningWidth * 0.5f);
    const FVector2D OpeningLeftEdge = Center2D - HalfOpeningOffset;
    const FVector2D OpeningRightEdge = Center2D + HalfOpeningOffset;

    const float RequestedPadding = FMath::Max(ElevatorWallPadding, 0.0f);
    const float MaxAllowedPadding = RequestedOpeningWidth * 0.49f;
    const float AppliedPadding = FMath::Clamp(RequestedPadding, 0.0f, MaxAllowedPadding);
    const FVector2D PaddingOffset = UnitDirection2D * AppliedPadding;

    const FVector2D AdjustedLeftEdge = OpeningLeftEdge + PaddingOffset;
    const FVector2D AdjustedRightEdge = OpeningRightEdge - PaddingOffset;
    const float FinalOpeningWidth = FMath::Max((AdjustedRightEdge - AdjustedLeftEdge).Size(), KINDA_SMALL_NUMBER);

    if ((AdjustedLeftEdge - WallStart2D).Size() > KINDA_SMALL_NUMBER)
    {
        PlaceWall(WallStart2D, AdjustedLeftEdge);
    }

    if ((WallEnd2D - AdjustedRightEdge).Size() > KINDA_SMALL_NUMBER)
    {
        PlaceWall(AdjustedRightEdge, WallEnd2D);
    }

    // Build an interior shaft using dedicated meshes so the cab sits cleanly behind the doorway.
    float RequestedShaftDepth = Element.Dimensions.Y;
    if (RequestedShaftDepth <= KINDA_SMALL_NUMBER)
    {
        RequestedShaftDepth = ApproxDepth;
    }
    if (RequestedShaftDepth <= KINDA_SMALL_NUMBER)
    {
        RequestedShaftDepth = 200.0f;
    }

    const float MeshHalfDepth = ApproxDepth * 0.5f;
    const float InsetForDepth = FMath::Max(ElevatorWallInset, 0.0f);
    const float MinimumDepth = InsetForDepth + MeshHalfDepth;
    float ShaftDepth = FMath::Max(FMath::Max(RequestedShaftDepth, MinimumDepth), KINDA_SMALL_NUMBER);
    ShaftDepth += FMath::Max(ElevatorShaftBackPadding, 0.0f);

    if (ShaftDepth > KINDA_SMALL_NUMBER)
    {
        UStaticMesh *ElevatorShaftMeshPtr = ElevatorShaftMesh.Get();
        if (!ElevatorShaftMeshPtr)
        {
            UE_LOG(LogProceduralOffice, Warning, TEXT("Elevator shaft mesh not assigned."));
        }
        else
        {
            UInstancedStaticMeshComponent *ShaftComponent = GetOrCreateISMC(ElevatorShaftMeshPtr, FName(TEXT("ElevatorShaft")), ElevatorShaftMaterialOverride.Get());
            if (ShaftComponent)
            {
                const FVector ShaftMeshSize = ElevatorShaftMeshPtr->GetBounds().BoxExtent * 2.0f;
                float ShaftHeight = ProceduralOffice::Utils::ComputeWallHeight(FloorHeight, CeilingHeight, 0.0f);
                if (ElevatorShaftHeightOverride > KINDA_SMALL_NUMBER)
                {
                    ShaftHeight = ElevatorShaftHeightOverride;
                }
                ShaftHeight = FMath::Max(ShaftHeight, 1.0f);
                const float DesiredThickness = ProceduralOffice::Utils::ClampPositive(ElevatorShaftThickness);
                const float SidePadding = FMath::Max(ElevatorShaftSidePadding, 0.0f);
                const float DoorInset = FMath::Max(ElevatorShaftDoorOffset, 0.0f);
                const FVector2D ShaftDirection2D = -PerpDirection2D;
                const FVector2D DoorOffsetVector = ShaftDirection2D * DoorInset;
                const FVector2D SideOffset = UnitDirection2D * SidePadding;
                const FVector2D ShaftFrontLeft = AdjustedLeftEdge - SideOffset + DoorOffsetVector;
                const FVector2D ShaftFrontRight = AdjustedRightEdge + SideOffset + DoorOffsetVector;
                const FVector2D ShaftDepthOffset = ShaftDirection2D * ShaftDepth;
                const FVector2D ShaftBackLeft = ShaftFrontLeft + ShaftDepthOffset;
                const FVector2D ShaftBackRight = ShaftFrontRight + ShaftDepthOffset;

                auto AddShaftSegment = [&](const FVector2D &Start2D, const FVector2D &End2D, float SegmentHeight, float SegmentBaseZ)
                {
                    float SegmentLength = 0.0f;
                    const FVector2D SegmentDirection = ProceduralOffice::Utils::CalculateUnitDirection(Start2D, End2D, SegmentLength);
                    if (SegmentLength <= KINDA_SMALL_NUMBER)
                    {
                        return;
                    }

                    const FVector2D Midpoint2D = (Start2D + End2D) * 0.5f;
                    const float EffectiveHeight = FMath::Max(SegmentHeight, 1.0f);
                    const FVector Location3D(Midpoint2D.X, Midpoint2D.Y, SegmentBaseZ + EffectiveHeight * 0.5f);
                    const float SegmentYaw = ProceduralOffice::Utils::DirectionToYawDegrees(SegmentDirection);
                    const float ScaleX = SegmentLength / FMath::Max(ShaftMeshSize.X, KINDA_SMALL_NUMBER);
                    const float ScaleY = DesiredThickness / FMath::Max(ShaftMeshSize.Y, KINDA_SMALL_NUMBER);
                    const float ScaleZ = EffectiveHeight / FMath::Max(ShaftMeshSize.Z, KINDA_SMALL_NUMBER);

                    const FTransform SegmentTransform(FRotator(0.0f, SegmentYaw, 0.0f), Location3D, FVector(ScaleX, ScaleY, ScaleZ));
                    ShaftComponent->AddInstance(SegmentTransform);
                };

                const float ShaftBaseZ = FloorHeight + Element.HeightOffset + ElevatorShaftVerticalOffset;
                float ShaftTopZ = ShaftBaseZ + ShaftHeight;
                float ShaftBottomZ = ShaftBaseZ;
                AddShaftSegment(ShaftFrontLeft, ShaftBackLeft, ShaftHeight, ShaftBaseZ);
                AddShaftSegment(ShaftFrontRight, ShaftBackRight, ShaftHeight, ShaftBaseZ);
                AddShaftSegment(ShaftBackLeft, ShaftBackRight, ShaftHeight, ShaftBaseZ);
                const float TopExtensionHeight = ElevatorShaftTopHeight;
                if (TopExtensionHeight > KINDA_SMALL_NUMBER)
                {
                    const float TopBaseZ = ShaftBaseZ + ShaftHeight;
                    AddShaftSegment(ShaftFrontLeft, ShaftBackLeft, TopExtensionHeight, TopBaseZ);
                    AddShaftSegment(ShaftFrontRight, ShaftBackRight, TopExtensionHeight, TopBaseZ);
                    AddShaftSegment(ShaftBackLeft, ShaftBackRight, TopExtensionHeight, TopBaseZ);
                    AddShaftSegment(ShaftFrontLeft, ShaftFrontRight, TopExtensionHeight, TopBaseZ);
                    ShaftTopZ += TopExtensionHeight;
                }

                const float BottomExtensionHeight = ElevatorShaftBottomHeight;
                if (BottomExtensionHeight > KINDA_SMALL_NUMBER)
                {
                    const float BottomBaseZ = ShaftBaseZ - BottomExtensionHeight;
                    AddShaftSegment(ShaftFrontLeft, ShaftBackLeft, BottomExtensionHeight, BottomBaseZ);
                    AddShaftSegment(ShaftFrontRight, ShaftBackRight, BottomExtensionHeight, BottomBaseZ);
                    AddShaftSegment(ShaftBackLeft, ShaftBackRight, BottomExtensionHeight, BottomBaseZ);
                    AddShaftSegment(ShaftFrontLeft, ShaftFrontRight, BottomExtensionHeight, BottomBaseZ);
                    ShaftBottomZ -= BottomExtensionHeight;
                }

                if (bGenerateElevatorShaftCaps && DesiredThickness > KINDA_SMALL_NUMBER)
                {
                    const float ShaftWidth = (ShaftFrontRight - ShaftFrontLeft).Size();
                    const float ShaftDepthLength = (ShaftBackLeft - ShaftFrontLeft).Size();
                    const float CapThickness = DesiredThickness;
                    const float HalfThickness = CapThickness * 0.5f;
                    const float ScaleX = ShaftWidth / FMath::Max(ShaftMeshSize.X, KINDA_SMALL_NUMBER);
                    const float ScaleY = ShaftDepthLength / FMath::Max(ShaftMeshSize.Y, KINDA_SMALL_NUMBER);
                    const float ScaleZ = CapThickness / FMath::Max(ShaftMeshSize.Z, KINDA_SMALL_NUMBER);
                    const FVector2D ShaftCenter2D = (ShaftFrontLeft + ShaftBackRight) * 0.5f;
                    const FRotator CapRotation(0.0f, YawDegrees, 0.0f);

                    if (ShaftTopZ > ShaftBottomZ)
                    {
                        const FVector TopLocation(ShaftCenter2D.X, ShaftCenter2D.Y, ShaftTopZ - HalfThickness);
                        ShaftComponent->AddInstance(FTransform(CapRotation, TopLocation, FVector(ScaleX, ScaleY, ScaleZ)));

                        const FVector BottomLocation(ShaftCenter2D.X, ShaftCenter2D.Y, ShaftBottomZ + HalfThickness);
                        ShaftComponent->AddInstance(FTransform(CapRotation, BottomLocation, FVector(ScaleX, ScaleY, ScaleZ)));
                    }
                }
            }
        }
    }

    const float WallHeight = ProceduralOffice::Utils::ComputeWallHeight(FloorHeight, CeilingHeight, 0.0f);
    float RequestedElevatorHeight = (Element.Height > KINDA_SMALL_NUMBER) ? Element.Height : ApproxHeight;
    if (ApproxHeight > KINDA_SMALL_NUMBER)
    {
        RequestedElevatorHeight = FMath::Max(RequestedElevatorHeight, ApproxHeight);
    }
    const float FinalElevatorHeight = FMath::Min(FMath::Max(RequestedElevatorHeight, KINDA_SMALL_NUMBER), WallHeight);
    const float TopPadding = FMath::Max(ElevatorTopWallPadding, 0.0f);
    const float FillHeight = WallHeight - FinalElevatorHeight - TopPadding;

    if (FillHeight > KINDA_SMALL_NUMBER)
    {
        UInstancedStaticMeshComponent *FillComponent = GetOrCreateISMC(WallMesh.Get(), FName(TEXT("ElevatorTopFill")), WallMaterialOverride.Get());
        if (FillComponent)
        {
            const FVector MeshSize = WallMesh->GetBounds().BoxExtent * 2.0f;
            const float DesiredThickness = ProceduralOffice::Utils::ClampPositive(WallThickness);
            const float ScaleX = FinalOpeningWidth / FMath::Max(MeshSize.X, KINDA_SMALL_NUMBER);
            const float ScaleY = DesiredThickness / FMath::Max(MeshSize.Y, KINDA_SMALL_NUMBER);
            const float ScaleZ = FillHeight / FMath::Max(MeshSize.Z, KINDA_SMALL_NUMBER);

            const FVector2D FillCenter2D = (AdjustedLeftEdge + AdjustedRightEdge) * 0.5f;
            const float FillBaseZ = FloorHeight + Element.HeightOffset + FinalElevatorHeight + TopPadding;
            const FVector FillLocation(FillCenter2D.X, FillCenter2D.Y, FillBaseZ + FillHeight * 0.5f);
            const FRotator FillRotation(0.0f, YawDegrees, 0.0f);
            const FTransform FillTransform(FillRotation, FillLocation, FVector(ScaleX, ScaleY, ScaleZ));
            FillComponent->AddInstance(FillTransform);
        }
    }
}
