#include "../ProceduralOfficeGenerator.h"
#include "ProceduralOfficeGenerator.Log.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"

void AProceduralOfficeGenerator::PlaceCeilingLights(const FVector2D &Start, const FVector2D &End, const FVector2D &Spacing, const FVector2D &Padding)
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

    const float SpacingX = FMath::Max(Spacing.X, 1.0f);
    const float SpacingY = FMath::Max(Spacing.Y, 1.0f);

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
    const float SourceWidth = (CeilingRectLightSourceWidth > 0.0f) ? CeilingRectLightSourceWidth : MeshLength;
    const float SourceHeight = (CeilingRectLightSourceHeight > 0.0f) ? CeilingRectLightSourceHeight : MeshWidth;
    const float FinalSourceWidth = FMath::Max(1.0f, SourceWidth);
    const float FinalSourceHeight = FMath::Max(1.0f, SourceHeight);
    const float BarnDoorAngle = FMath::Clamp(CeilingRectLightBarnDoorAngle, 0.0f, 90.0f);
    const float BarnDoorLength = FMath::Max(0.0f, CeilingRectLightBarnDoorLength);
    const float SpacingMagnitude = FVector2D(SpacingX, SpacingY).Size();
    const float AutoAttenuationRadius = (SpacingMagnitude > KINDA_SMALL_NUMBER) ? SpacingMagnitude * 1.5f : FMath::Max(MeshLength, MeshWidth) * 1.1f;
    const float EffectiveAttenuationRadius = (CeilingLightAttenuationRadius > 0.0f) ? CeilingLightAttenuationRadius : AutoAttenuationRadius;

    const float AreaWidth = MaxX - MinX;
    const float AreaHeight = MaxY - MinY;
    const int32 CountX = FMath::Max(1, FMath::FloorToInt(AreaWidth / SpacingX) + 1);
    const int32 CountY = FMath::Max(1, FMath::FloorToInt(AreaHeight / SpacingY) + 1);
    const float TotalSpanX = (CountX - 1) * SpacingX;
    const float TotalSpanY = (CountY - 1) * SpacingY;
    const float StartX = MinX + (AreaWidth - TotalSpanX) * 0.5f;
    const float StartY = MinY + (AreaHeight - TotalSpanY) * 0.5f;

    for (int32 IndexX = 0; IndexX < CountX; ++IndexX)
    {
        const float X = StartX + IndexX * SpacingX;
        const float AdjustedX = X - ScaledOrigin.X;
        for (int32 IndexY = 0; IndexY < CountY; ++IndexY)
        {
            const float Y = StartY + IndexY * SpacingY;
            const FVector MeshLocation(AdjustedX, Y - ScaledOrigin.Y, LightZ);
            const FTransform LightTransform(FRotator::ZeroRotator, MeshLocation, CeilingLightScale);
            Component->AddInstance(LightTransform);

            if (bShouldSpawnLights)
            {
                URectLightComponent *NewLight = NewObject<URectLightComponent>(this);
                if (!NewLight)
                {
                    continue;
                }

                NewLight->SetMobility(EComponentMobility::Stationary);
                NewLight->SetIntensity(CeilingLightIntensity);
                NewLight->SetAttenuationRadius(FMath::Max(1.0f, EffectiveAttenuationRadius));
                NewLight->SetLightColor(CeilingLightColor);
                NewLight->SetCastShadows(bCeilingLightsCastShadows);
                NewLight->SetSourceWidth(FinalSourceWidth);
                NewLight->SetSourceHeight(FinalSourceHeight);
                NewLight->SetBarnDoorAngle(BarnDoorAngle);
                NewLight->SetBarnDoorLength(BarnDoorLength);
                NewLight->SetupAttachment(Root);
                NewLight->RegisterComponent();
                const FVector LightLocation(X, Y, CeilingHeight - EffectiveVerticalOffset);
                NewLight->SetWorldLocation(LightLocation);
                NewLight->SetWorldRotation(FRotator(-90.0f, 90.0f, 0.0f));
                SpawnedCeilingLights.Add(NewLight);
            }
        }
    }
}
