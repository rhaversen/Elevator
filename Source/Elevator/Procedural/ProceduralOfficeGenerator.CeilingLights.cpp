#include "../ProceduralOfficeGenerator.h"
#include "ProceduralOfficeGenerator.Log.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"

void AProceduralOfficeGenerator::PlaceCeilingLights(const FVector2D &Start, const FVector2D &End, const FVector2D &Spacing, const FVector2D &Padding, float DirectionYawDegrees)
{
    if (!CeilingLightMesh)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Ceiling light mesh not assigned."));
        return;
    }

    const float MinX = FMath::Min(Start.X, End.X) + FMath::Max(0.0f, Padding.X);
    const float MaxX = FMath::Max(Start.X, End.X) - FMath::Max(0.0f, Padding.X);
    const float MinY = FMath::Min(Start.Y, End.Y) + FMath::Max(0.0f, Padding.Y);
    const float MaxY = FMath::Max(Start.Y, End.Y) - FMath::Max(0.0f, Padding.Y);

    const float AlongSpacing = FMath::Max(Spacing.X, 1.0f);
    const float LineSpacing = FMath::Max(Spacing.Y, 1.0f);

    UInstancedStaticMeshComponent *Component = GetOrCreateISMC(CeilingLightMesh.Get(), FName(TEXT("CeilingLight")), CeilingLightMaterialOverride.Get());
    if (!Component)
    {
        return;
    }

    const FBoxSphereBounds MeshBounds = CeilingLightMesh->GetBounds();
    const FVector BoundsOrigin = MeshBounds.Origin;
    const FVector ScaledOrigin = BoundsOrigin * CeilingLightScale;
    const float TopOffset = (BoundsOrigin.Z + MeshBounds.BoxExtent.Z) * CeilingLightScale.Z;
    const float LightZ = CeilingHeight - TopOffset;
    const float MeshLength = MeshBounds.BoxExtent.X * 2.0f * CeilingLightScale.X;
    const float MeshWidth = MeshBounds.BoxExtent.Y * 2.0f * CeilingLightScale.Y;

    if (MinX > MaxX || MinY > MaxY)
    {
        return;
    }

    const float EffectiveVerticalOffset = FMath::Max(0.0f, CeilingLightVerticalOffset);
    const bool bShouldSpawnLights = bSpawnCeilingLightComponents && CeilingLightIntensity > KINDA_SMALL_NUMBER;
    const float BarnDoorAngle = FMath::Clamp(CeilingRectLightBarnDoorAngle, 0.0f, 90.0f);
    const float BarnDoorLength = FMath::Max(0.0f, CeilingRectLightBarnDoorLength);
    const float SpacingMagnitude = FVector2D(AlongSpacing, LineSpacing).Size();
    const float AutoAttenuationRadius = (SpacingMagnitude > KINDA_SMALL_NUMBER) ? SpacingMagnitude * 1.5f : FMath::Max(MeshLength, MeshWidth) * 1.1f;
    const bool bHasCustomAttenuation = CeilingLightAttenuationRadius > 0.0f;

    const FVector2D AreaCenter((MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f);
    const float DirectionRadians = FMath::DegreesToRadians(DirectionYawDegrees);
    FVector2D Direction(FMath::Cos(DirectionRadians), FMath::Sin(DirectionRadians));
    if (Direction.IsNearlyZero())
    {
        Direction = FVector2D(1.0f, 0.0f);
    }
    Direction.Normalize();
    const FVector2D Perpendicular(-Direction.Y, Direction.X);

    const FVector2D Corners[4] = {
        FVector2D(MinX, MinY),
        FVector2D(MinX, MaxY),
        FVector2D(MaxX, MinY),
        FVector2D(MaxX, MaxY)};

    FVector2D Relative = Corners[0] - AreaCenter;
    float MinU = FVector2D::DotProduct(Relative, Direction);
    float MaxU = MinU;
    float MinV = FVector2D::DotProduct(Relative, Perpendicular);
    float MaxV = MinV;

    for (int32 CornerIndex = 1; CornerIndex < 4; ++CornerIndex)
    {
        Relative = Corners[CornerIndex] - AreaCenter;
        const float U = FVector2D::DotProduct(Relative, Direction);
        const float V = FVector2D::DotProduct(Relative, Perpendicular);
        MinU = FMath::Min(MinU, U);
        MaxU = FMath::Max(MaxU, U);
        MinV = FMath::Min(MinV, V);
        MaxV = FMath::Max(MaxV, V);
    }

    const float RangeV = MaxV - MinV;
    const int32 LineCount = FMath::Max(1, FMath::FloorToInt(RangeV / LineSpacing) + 1);
    const float TotalSpanV = (LineCount - 1) * LineSpacing;
    const float FirstV = MinV + FMath::Max(0.0f, (RangeV - TotalSpanV) * 0.5f);
    const FRotator InstanceRotation(0.0f, DirectionYawDegrees + 90.0f, 0.0f);
    const FVector MeshPivotOffset = InstanceRotation.RotateVector(FVector(ScaledOrigin.X, ScaledOrigin.Y, 0.0f));

    for (int32 LineIndex = 0; LineIndex < LineCount; ++LineIndex)
    {
        const float V = FirstV + LineIndex * LineSpacing;
        const FVector2D LineBase = AreaCenter + Perpendicular * V;

        float UMin = -BIG_NUMBER;
        float UMax = BIG_NUMBER;

        auto AccumulateRange = [&](float CoordMin, float CoordMax, float OriginCoord, float DirCoord) -> bool
        {
            if (FMath::IsNearlyZero(DirCoord))
            {
                return OriginCoord >= CoordMin - KINDA_SMALL_NUMBER && OriginCoord <= CoordMax + KINDA_SMALL_NUMBER;
            }

            float Entry = (CoordMin - OriginCoord) / DirCoord;
            float Exit = (CoordMax - OriginCoord) / DirCoord;
            if (Entry > Exit)
            {
                const float Temp = Entry;
                Entry = Exit;
                Exit = Temp;
            }

            UMin = FMath::Max(UMin, Entry);
            UMax = FMath::Min(UMax, Exit);
            return UMin <= UMax;
        };

        if (!AccumulateRange(MinX, MaxX, LineBase.X, Direction.X))
        {
            continue;
        }

        if (!AccumulateRange(MinY, MaxY, LineBase.Y, Direction.Y))
        {
            continue;
        }

        const float SegmentLength = FMath::Max(0.0f, UMax - UMin);
        if (SegmentLength <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        const int32 PointsOnLine = FMath::Max(1, FMath::FloorToInt(SegmentLength / AlongSpacing) + 1);
        const float OccupiedSpan = (PointsOnLine - 1) * AlongSpacing;
        const float StartU = UMin + (SegmentLength - OccupiedSpan) * 0.5f;

        bool bHasPointOnLine = false;

        for (int32 PointIndex = 0; PointIndex < PointsOnLine; ++PointIndex)
        {
            const float U = StartU + PointIndex * AlongSpacing;
            const FVector2D Position2D = LineBase + Direction * U;
            const FVector MeshLocation(Position2D.X - MeshPivotOffset.X, Position2D.Y - MeshPivotOffset.Y, LightZ);
            const FTransform LightTransform(InstanceRotation, MeshLocation, CeilingLightScale);
            Component->AddInstance(LightTransform);

            if (!bHasPointOnLine)
            {
                bHasPointOnLine = true;
            }
        }

        if (bShouldSpawnLights && bHasPointOnLine)
        {
            const FVector2D SegmentCenter2D = LineBase + Direction * ((UMin + UMax) * 0.5f);
            const float LightLength = SegmentLength;
            const float AutoLineRadius = (LightLength > KINDA_SMALL_NUMBER) ? (LightLength * 0.75f + MeshWidth) : AutoAttenuationRadius;
            const float LightAttenuationRadius = bHasCustomAttenuation ? CeilingLightAttenuationRadius : FMath::Max(AutoAttenuationRadius, AutoLineRadius);

            URectLightComponent *NewLight = NewObject<URectLightComponent>(this);
            if (!NewLight)
            {
                continue;
            }

            NewLight->SetMobility(EComponentMobility::Stationary);
            NewLight->SetIntensity(CeilingLightIntensity);
            NewLight->SetAttenuationRadius(FMath::Max(1.0f, LightAttenuationRadius));
            NewLight->SetLightColor(CeilingLightColor);
            NewLight->SetCastShadows(bCeilingLightsCastShadows);
            NewLight->SetSourceWidth(FMath::Max(1.0f, MeshWidth));
            NewLight->SetSourceHeight(FMath::Max(1.0f, LightLength));
            NewLight->SetBarnDoorAngle(BarnDoorAngle);
            NewLight->SetBarnDoorLength(BarnDoorLength);
            NewLight->SetupAttachment(Root);
            NewLight->RegisterComponent();
            const FVector LightLocation(SegmentCenter2D.X, SegmentCenter2D.Y, CeilingHeight - EffectiveVerticalOffset);
            NewLight->SetWorldLocation(LightLocation);
            NewLight->SetWorldRotation(FRotator(-90.0f, DirectionYawDegrees, 0.0f));
            SpawnedCeilingLights.Add(NewLight);
        }
    }
}
