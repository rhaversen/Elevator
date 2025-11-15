#include "../ProceduralOfficeGenerator.h"
#include "ProceduralOfficeGenerator.Helpers.h"
#include "ProceduralOfficeGenerator.Log.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"

void AProceduralOfficeGenerator::PlaceElevator(const FOfficeElementDefinition &Element)
{
    UStaticMesh *ElevatorMeshPtr = ElevatorMesh.Get();
    if (!ElevatorMeshPtr)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Elevator mesh not assigned."));
        return;
    }

    UInstancedStaticMeshComponent *Component = GetOrCreateISMC(ElevatorMeshPtr, FName(TEXT("Elevator")), ElevatorMaterialOverride.Get());
    if (!Component)
    {
        return;
    }

    float TotalSpan = 0.0f;
    FVector2D UnitDirection2D = ProceduralOffice::Utils::CalculateUnitDirection(Element.Start, Element.End, TotalSpan);
    FVector2D Center2D = Element.Start;
    float YawDegrees = Element.Yaw;

    FVector2D WallStart2D = Element.Start;
    FVector2D WallEnd2D = Element.End;

    if (TotalSpan > KINDA_SMALL_NUMBER)
    {
        Center2D = (Element.Start + Element.End) * 0.5f;
        YawDegrees = ProceduralOffice::Utils::DirectionToYawDegrees(UnitDirection2D);
    }
    else
    {
        const float YawRadians = FMath::DegreesToRadians(YawDegrees);
        UnitDirection2D = FVector2D(FMath::Cos(YawRadians), FMath::Sin(YawRadians));

        float DefaultSpan = Element.Dimensions.X;
        if (DefaultSpan <= KINDA_SMALL_NUMBER)
        {
            DefaultSpan = ElevatorMeshPtr->GetBounds().BoxExtent.X * 2.0f;
        }

        TotalSpan = FMath::Max(DefaultSpan, 1.0f);
        const FVector2D HalfSpan = UnitDirection2D * (TotalSpan * 0.5f);
        WallStart2D = Center2D - HalfSpan;
        WallEnd2D = Center2D + HalfSpan;
    }

    const FVector2D PerpDirection2D(-UnitDirection2D.Y, UnitDirection2D.X);
    const FVector ElevatorInsetOffset(PerpDirection2D.X * ElevatorWallInset, PerpDirection2D.Y * ElevatorWallInset, 0.0f);

    const FVector Location = FVector(Center2D.X, Center2D.Y, FloorHeight + Element.HeightOffset) + ElevatorInsetOffset;
    const FTransform InstanceTransform(FRotator(0.0f, YawDegrees, 0.0f), Location, FVector::OneVector);
    Component->AddInstance(InstanceTransform);

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
            if (ElevatorMeshPtr)
            {
                const float MeshHeight = ElevatorMeshPtr->GetBounds().BoxExtent.Z * 2.0f;
                ElevatorTop = FloorHeight + Element.HeightOffset + MeshHeight;
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

    const float WallHeight = ProceduralOffice::Utils::ComputeWallHeight(FloorHeight, CeilingHeight, 0.0f);
    float RequestedElevatorHeight = Element.Height;
    float ElevatorMeshHeight = 0.0f;
    if (ElevatorMeshPtr)
    {
        ElevatorMeshHeight = ElevatorMeshPtr->GetBounds().BoxExtent.Z * 2.0f;
    }
    if (RequestedElevatorHeight <= KINDA_SMALL_NUMBER)
    {
        RequestedElevatorHeight = ElevatorMeshHeight;
    }
    else if (ElevatorMeshHeight > KINDA_SMALL_NUMBER)
    {
        RequestedElevatorHeight = FMath::Max(RequestedElevatorHeight, ElevatorMeshHeight);
    }
    const float FinalElevatorHeight = FMath::Min(FMath::Max(RequestedElevatorHeight, KINDA_SMALL_NUMBER), WallHeight);
    const float FillHeight = WallHeight - FinalElevatorHeight;

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
            const FVector FillLocation(FillCenter2D.X, FillCenter2D.Y, FloorHeight + Element.HeightOffset + FinalElevatorHeight + FillHeight * 0.5f);
            const FRotator FillRotation(0.0f, YawDegrees, 0.0f);
            const FTransform FillTransform(FillRotation, FillLocation, FVector(ScaleX, ScaleY, ScaleZ));
            FillComponent->AddInstance(FillTransform);
        }
    }
}
