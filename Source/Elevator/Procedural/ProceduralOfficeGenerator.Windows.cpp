#include "../ProceduralOfficeGenerator.h"
#include "ProceduralOfficeGenerator.Helpers.h"
#include "ProceduralOfficeGenerator.Log.h"

#include "Components/InstancedStaticMeshComponent.h"

void AProceduralOfficeGenerator::PlaceWindow(const FVector2D &Start, const FVector2D &End, float Thickness, int32 SectionCount)
{
    if (!WindowMesh)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Window mesh not assigned."));
        return;
    }

    float TotalLength = 0.0f;
    const FVector2D UnitDirection = ProceduralOffice::Utils::CalculateUnitDirection(Start, End, TotalLength);
    if (TotalLength <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const int32 ActualSectionCount = FMath::Max(1, SectionCount);
    const float FrameThickness = WindowFrameThickness;

    const float TotalFrameLength = (ActualSectionCount + 1) * FrameThickness;
    const float AvailableGlassLength = TotalLength - TotalFrameLength;
    const float SectionLength = AvailableGlassLength / ActualSectionCount;

    if (SectionLength <= KINDA_SMALL_NUMBER)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Window sections too small to fit frames."));
        return;
    }

    const float WallHeight = ProceduralOffice::Utils::ComputeWallHeight(FloorHeight, CeilingHeight);
    const float WindowHeight = WallHeight * FMath::Clamp(WindowHeightRatio, 0.0f, 1.0f) - 2.0f * FrameThickness;
    const float DesiredThickness = Thickness > 0.0f ? Thickness : 20.0f;
    const float Yaw = ProceduralOffice::Utils::DirectionToYawDegrees(UnitDirection);

    const float WindowBottomZ = FloorHeight + (WallHeight - WindowHeight - 2.0f * FrameThickness) * 0.5f + FrameThickness;
    const float WindowCenterZ = WindowBottomZ + WindowHeight * 0.5f;

    UInstancedStaticMeshComponent *GlassComponent = GetOrCreateISMC(WindowMesh.Get(), FName(TEXT("WindowGlass")), WindowMaterialOverride.Get());
    if (GlassComponent)
    {
        const FVector GlassMeshSize = WindowMesh->GetBounds().BoxExtent * 2.0f;
        const float GlassScaleZ = WindowHeight / FMath::Max(GlassMeshSize.Z, KINDA_SMALL_NUMBER);
        const float GlassScaleY = DesiredThickness / FMath::Max(GlassMeshSize.Y, KINDA_SMALL_NUMBER);

        float CurrentOffset = FrameThickness + SectionLength * 0.5f;
        for (int32 Index = 0; Index < ActualSectionCount; ++Index)
        {
            const FVector2D SectionCenter2D = Start + UnitDirection * CurrentOffset;
            const FVector SectionLocation(SectionCenter2D.X, SectionCenter2D.Y, WindowCenterZ);
            const float GlassScaleX = SectionLength / FMath::Max(GlassMeshSize.X, KINDA_SMALL_NUMBER);
            const FTransform GlassTransform(FRotator(0.0f, Yaw, 0.0f), SectionLocation, FVector(GlassScaleX, GlassScaleY, GlassScaleZ));
            GlassComponent->AddInstance(GlassTransform);

            CurrentOffset += SectionLength + FrameThickness;
        }
    }

    if (WindowFrameMesh)
    {
        UInstancedStaticMeshComponent *VerticalFrameComponent = GetOrCreateISMC(WindowFrameMesh.Get(), FName(TEXT("WindowFrameVertical")), WindowFrameMaterialOverride.Get());
        if (VerticalFrameComponent)
        {
            const FVector FrameMeshSize = WindowFrameMesh->GetBounds().BoxExtent * 2.0f;
            const float FrameScaleX = FrameThickness / FMath::Max(FrameMeshSize.X, KINDA_SMALL_NUMBER);
            const float FrameScaleY = WindowFrameDepth / FMath::Max(FrameMeshSize.Y, KINDA_SMALL_NUMBER);
            const float FrameScaleZ = WindowHeight / FMath::Max(FrameMeshSize.Z, KINDA_SMALL_NUMBER);

            {
                const FVector2D FrameCenter2D = Start + UnitDirection * (FrameThickness * 0.5f);
                const FVector FrameLocation(FrameCenter2D.X, FrameCenter2D.Y, WindowCenterZ);
                const FTransform FrameTransform(FRotator(0.0f, Yaw, 0.0f), FrameLocation, FVector(FrameScaleX, FrameScaleY, FrameScaleZ));
                VerticalFrameComponent->AddInstance(FrameTransform);
            }

            float FrameOffset = FrameThickness + SectionLength + FrameThickness * 0.5f;
            for (int32 Index = 0; Index < ActualSectionCount - 1; ++Index)
            {
                const FVector2D FrameCenter2D = Start + UnitDirection * FrameOffset;
                const FVector FrameLocation(FrameCenter2D.X, FrameCenter2D.Y, WindowCenterZ);
                const FTransform FrameTransform(FRotator(0.0f, Yaw, 0.0f), FrameLocation, FVector(FrameScaleX, FrameScaleY, FrameScaleZ));
                VerticalFrameComponent->AddInstance(FrameTransform);

                FrameOffset += SectionLength + FrameThickness;
            }

            {
                const FVector2D FrameCenter2D = Start + UnitDirection * (TotalLength - FrameThickness * 0.5f);
                const FVector FrameLocation(FrameCenter2D.X, FrameCenter2D.Y, WindowCenterZ);
                const FTransform FrameTransform(FRotator(0.0f, Yaw, 0.0f), FrameLocation, FVector(FrameScaleX, FrameScaleY, FrameScaleZ));
                VerticalFrameComponent->AddInstance(FrameTransform);
            }
        }

        UInstancedStaticMeshComponent *HorizontalFrameComponent = GetOrCreateISMC(WindowFrameMesh.Get(), FName(TEXT("WindowFrameHorizontal")), WindowFrameMaterialOverride.Get());
        if (HorizontalFrameComponent)
        {
            const FVector FrameMeshSize = WindowFrameMesh->GetBounds().BoxExtent * 2.0f;
            const float FrameScaleX = TotalLength / FMath::Max(FrameMeshSize.X, KINDA_SMALL_NUMBER);
            const float FrameScaleY = WindowFrameDepth / FMath::Max(FrameMeshSize.Y, KINDA_SMALL_NUMBER);
            const float FrameScaleZ = FrameThickness / FMath::Max(FrameMeshSize.Z, KINDA_SMALL_NUMBER);

            const FVector2D MidPoint2D = (Start + End) * 0.5f;
            const float BottomZ = WindowBottomZ - FrameThickness * 0.5f;
            const float TopZ = WindowBottomZ + WindowHeight + FrameThickness * 0.5f;

            {
                const FVector FrameLocation(MidPoint2D.X, MidPoint2D.Y, BottomZ);
                const FTransform FrameTransform(FRotator(0.0f, Yaw, 0.0f), FrameLocation, FVector(FrameScaleX, FrameScaleY, FrameScaleZ));
                HorizontalFrameComponent->AddInstance(FrameTransform);
            }

            {
                const FVector FrameLocation(MidPoint2D.X, MidPoint2D.Y, TopZ);
                const FTransform FrameTransform(FRotator(0.0f, Yaw, 0.0f), FrameLocation, FVector(FrameScaleX, FrameScaleY, FrameScaleZ));
                HorizontalFrameComponent->AddInstance(FrameTransform);
            }
        }
    }
}
