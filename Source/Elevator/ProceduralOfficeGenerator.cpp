#include "ProceduralOfficeGenerator.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogProceduralOffice, Log, All);

AProceduralOfficeGenerator::AProceduralOfficeGenerator()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    LayoutFileRelativePath = TEXT("Layouts/ExampleOpenOffice.json");
}

void AProceduralOfficeGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (bRegenerateOnConstruction)
    {
        GenerateFromData();
    }
}

void AProceduralOfficeGenerator::BeginPlay()
{
    Super::BeginPlay();

    if (!bRegenerateOnConstruction)
    {
        GenerateFromData();
    }
}

void AProceduralOfficeGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DestroySpawnedComponents();
    Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AProceduralOfficeGenerator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (bRegenerateOnConstruction && PropertyChangedEvent.Property)
    {
        const FName Name = PropertyChangedEvent.Property->GetFName();
        if (Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, LayoutFileRelativePath) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, FloorHeight) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, FloorThickness) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingHeight) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingThickness) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, FloorMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WallMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, FloorMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WallMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WallThickness) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WindowMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WindowMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WindowFrameMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WindowFrameMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WindowFrameThickness) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WindowFrameDepth) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WindowHeightRatio) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, SpawnPointTag) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubiclePartitionMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubiclePartitionMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleDeskMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleDeskMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleChairMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleChairMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubiclePartitionHeight) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubiclePartitionThickness) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleDeskHeightOffset) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleDeskScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleDeskBackOffsetRatio) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleChairRotation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleChairScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleChairFrontOffsetRatio) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, bSpawnCeilingLightComponents) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightIntensity) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightAttenuationRadius) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightColor) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, bCeilingLightsCastShadows) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightVerticalOffset) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingRectLightSourceWidth) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingRectLightSourceHeight) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingRectLightBarnDoorAngle) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingRectLightBarnDoorLength) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, DoorActorClass) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, DoorActorOffset) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, DoorActorScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, DoorWallPadding))
        {
            GenerateFromData();
        }
    }
}
#endif

void AProceduralOfficeGenerator::GenerateFromData()
{
    DestroySpawnedComponents();

    FOfficeLayout Layout;
    if (!LoadLayoutData(Layout))
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Failed to load office layout: %s"), *LayoutFileRelativePath);
        return;
    }

    BuildFromLayout(Layout);
}

void AProceduralOfficeGenerator::ClearGeneratedContent()
{
    DestroySpawnedComponents();
}

bool AProceduralOfficeGenerator::LoadLayoutData(FOfficeLayout& OutLayout) const
{
    if (LayoutFileRelativePath.IsEmpty())
    {
        return false;
    }

    const FString AbsolutePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), LayoutFileRelativePath));
    if (!FPaths::FileExists(AbsolutePath))
    {
        UE_LOG(LogProceduralOffice, Error, TEXT("Layout file does not exist: %s"), *AbsolutePath);
        return false;
    }

    FString FileContents;
    if (!FFileHelper::LoadFileToString(FileContents, *AbsolutePath))
    {
        UE_LOG(LogProceduralOffice, Error, TEXT("Unable to read layout file: %s"), *AbsolutePath);
        return false;
    }

    if (!FJsonObjectConverter::JsonObjectStringToUStruct(FileContents, &OutLayout, 0, 0))
    {
        UE_LOG(LogProceduralOffice, Error, TEXT("Layout JSON is invalid: %s"), *AbsolutePath);
        return false;
    }

    return true;
}

void AProceduralOfficeGenerator::BuildFromLayout(const FOfficeLayout& Layout)
{
    if (Layout.Elements.IsEmpty())
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Layout contains no elements."));
        return;
    }

    for (const FOfficeElementDefinition& Element : Layout.Elements)
    {
        BuildElement(Element);
    }
}

void AProceduralOfficeGenerator::BuildElement(const FOfficeElementDefinition& Element)
{
    switch (Element.Type)
    {
        case EOfficeElementType::Floor:
            PlaceSurface(EOfficeElementType::Floor, Element.Start, Element.End);
            break;
        case EOfficeElementType::Ceiling:
            PlaceSurface(EOfficeElementType::Ceiling, Element.Start, Element.End);
            break;
        case EOfficeElementType::Wall:
            PlaceWall(Element.Start, Element.End);
            break;
        case EOfficeElementType::Window:
            PlaceWindow(Element.Start, Element.End, Element.Thickness, Element.SectionCount);
            break;
        case EOfficeElementType::SpawnPoint:
            PlaceSpawnPoint(Element.Start, Element.HeightOffset, Element.Yaw);
            break;
        case EOfficeElementType::Cubicle:
            PlaceCubicle(Element.Start, Element.Dimensions, Element.Yaw);
            break;
        case EOfficeElementType::CeilingLight:
            PlaceCeilingLights(Element.Start, Element.End, Element.Spacing, Element.Padding);
            break;
        case EOfficeElementType::Door:
            PlaceDoor(Element);
            break;
        default:
            UE_LOG(LogProceduralOffice, Warning, TEXT("Unsupported element type encountered."));
            break;
    }
}

void AProceduralOfficeGenerator::PlaceSurface(EOfficeElementType Type, const FVector2D& Start, const FVector2D& End)
{
    const bool bIsFloor = Type == EOfficeElementType::Floor;
    UStaticMesh* Mesh = bIsFloor ? FloorMesh.Get() : CeilingMesh.Get();
    if (!Mesh)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("%s mesh not assigned."), bIsFloor ? TEXT("Floor") : TEXT("Ceiling"));
        return;
    }

    const float MinX = FMath::Min(Start.X, End.X);
    const float MaxX = FMath::Max(Start.X, End.X);
    const float MinY = FMath::Min(Start.Y, End.Y);
    const float MaxY = FMath::Max(Start.Y, End.Y);

    const float SizeX = FMath::Max(MaxX - MinX, 1.0f);
    const float SizeY = FMath::Max(MaxY - MinY, 1.0f);
    const float Thickness = bIsFloor ? FloorThickness : CeilingThickness;
    const FVector MeshSize = Mesh->GetBounds().BoxExtent * 2.0f;

    FVector Center((MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f, 0.0f);
    if (bIsFloor)
    {
        Center.Z = FloorHeight - (Thickness * 0.5f);
    }
    else
    {
        Center.Z = CeilingHeight + (Thickness * 0.5f);
    }

    UMaterialInterface* Material = bIsFloor ? FloorMaterialOverride.Get() : CeilingMaterialOverride.Get();
    UInstancedStaticMeshComponent* Component = GetOrCreateISMC(Mesh, bIsFloor ? FName(TEXT("Floor")) : FName(TEXT("Ceiling")), Material);
    if (!Component)
    {
        return;
    }

    const float ScaleX = SizeX / FMath::Max(MeshSize.X, KINDA_SMALL_NUMBER);
    const float ScaleY = SizeY / FMath::Max(MeshSize.Y, KINDA_SMALL_NUMBER);
    const float ScaleZ = Thickness / FMath::Max(MeshSize.Z, KINDA_SMALL_NUMBER);
    const FTransform InstanceTransform(FRotator::ZeroRotator, Center, FVector(ScaleX, ScaleY, ScaleZ));
    Component->AddInstance(InstanceTransform);
}

void AProceduralOfficeGenerator::PlaceWall(const FVector2D& Start, const FVector2D& End)
{
    if (!WallMesh)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Wall mesh not assigned."));
        return;
    }

    const FVector2D Direction2D = End - Start;
    const float Length = Direction2D.Size();
    if (Length <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    UInstancedStaticMeshComponent* Component = GetOrCreateISMC(WallMesh.Get(), FName(TEXT("Wall")), WallMaterialOverride.Get());
    if (!Component)
    {
        return;
    }

    const float WallHeight = FMath::Max(CeilingHeight - FloorHeight, 1.0f);
    const FVector MeshSize = WallMesh->GetBounds().BoxExtent * 2.0f;
    const float ScaleX = Length / FMath::Max(MeshSize.X, KINDA_SMALL_NUMBER);
    const float ScaleZ = WallHeight / FMath::Max(MeshSize.Z, KINDA_SMALL_NUMBER);
    const float DesiredThickness = FMath::Max(WallThickness, KINDA_SMALL_NUMBER);
    const float ScaleY = DesiredThickness / FMath::Max(MeshSize.Y, KINDA_SMALL_NUMBER);

    const FVector2D Midpoint2D = (Start + End) * 0.5f;
    const FVector Location(Midpoint2D.X, Midpoint2D.Y, FloorHeight + WallHeight * 0.5f);
    const float Yaw = FMath::Atan2(Direction2D.Y, Direction2D.X) * (180.0f / PI);
    const FTransform InstanceTransform(FRotator(0.0f, Yaw, 0.0f), Location, FVector(ScaleX, ScaleY, ScaleZ));
    Component->AddInstance(InstanceTransform);
}

void AProceduralOfficeGenerator::PlaceWindow(const FVector2D& Start, const FVector2D& End, float Thickness, int32 SectionCount)
{
    if (!WindowMesh)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Window mesh not assigned."));
        return;
    }

    const FVector2D Direction2D = End - Start;
    const float TotalLength = Direction2D.Size();
    if (TotalLength <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector2D UnitDirection = Direction2D / TotalLength;
    const FVector2D PerpDirection(-UnitDirection.Y, UnitDirection.X);
    const int32 ActualSectionCount = FMath::Max(1, SectionCount);
    const float FrameThickness = WindowFrameThickness;
    
    // Account for end frames in total length calculation
    const float TotalFrameLength = (ActualSectionCount + 1) * FrameThickness;
    const float AvailableGlassLength = TotalLength - TotalFrameLength;
    const float SectionLength = AvailableGlassLength / ActualSectionCount;

    if (SectionLength <= KINDA_SMALL_NUMBER)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Window sections too small to fit frames."));
        return;
    }

    const float WallHeight = FMath::Max(CeilingHeight - FloorHeight, 1.0f);
    const float WindowHeight = WallHeight * FMath::Clamp(WindowHeightRatio, 0.0f, 1.0f) - 2.0f * FrameThickness;
    const float DesiredThickness = Thickness > 0.0f ? Thickness : 20.0f;
    const float Yaw = FMath::Atan2(Direction2D.Y, Direction2D.X) * (180.0f / PI);

    const float WindowBottomZ = FloorHeight + (WallHeight - WindowHeight - 2.0f * FrameThickness) * 0.5f + FrameThickness;
    const float WindowCenterZ = WindowBottomZ + WindowHeight * 0.5f;

    // Glass sections
    UInstancedStaticMeshComponent* GlassComponent = GetOrCreateISMC(WindowMesh.Get(), FName(TEXT("WindowGlass")), WindowMaterialOverride.Get());
    if (GlassComponent)
    {
        const FVector GlassMeshSize = WindowMesh->GetBounds().BoxExtent * 2.0f;
        const float GlassScaleZ = WindowHeight / FMath::Max(GlassMeshSize.Z, KINDA_SMALL_NUMBER);
        const float GlassScaleY = DesiredThickness / FMath::Max(GlassMeshSize.Y, KINDA_SMALL_NUMBER);

        float CurrentOffset = FrameThickness + SectionLength * 0.5f;
        for (int32 i = 0; i < ActualSectionCount; ++i)
        {
            const FVector2D SectionCenter2D = Start + UnitDirection * CurrentOffset;
            const FVector SectionLocation(SectionCenter2D.X, SectionCenter2D.Y, WindowCenterZ);
            const float GlassScaleX = SectionLength / FMath::Max(GlassMeshSize.X, KINDA_SMALL_NUMBER);
            const FTransform GlassTransform(FRotator(0.0f, Yaw, 0.0f), SectionLocation, FVector(GlassScaleX, GlassScaleY, GlassScaleZ));
            GlassComponent->AddInstance(GlassTransform);

            CurrentOffset += SectionLength + FrameThickness;
        }
    }

    // Vertical frames (between sections, and at start/end)
    if (WindowFrameMesh)
    {
        UInstancedStaticMeshComponent* VerticalFrameComponent = GetOrCreateISMC(WindowFrameMesh.Get(), FName(TEXT("WindowFrameVertical")), WindowFrameMaterialOverride.Get());
        if (VerticalFrameComponent)
        {
            const FVector FrameMeshSize = WindowFrameMesh->GetBounds().BoxExtent * 2.0f;
            const float FrameScaleX = FrameThickness / FMath::Max(FrameMeshSize.X, KINDA_SMALL_NUMBER);
            const float FrameScaleY = WindowFrameDepth / FMath::Max(FrameMeshSize.Y, KINDA_SMALL_NUMBER);
            const float FrameScaleZ = WindowHeight / FMath::Max(FrameMeshSize.Z, KINDA_SMALL_NUMBER);

            // Start frame
            {
                const FVector2D FrameCenter2D = Start + UnitDirection * (FrameThickness * 0.5f);
                const FVector FrameLocation(FrameCenter2D.X, FrameCenter2D.Y, WindowCenterZ);
                const FTransform FrameTransform(FRotator(0.0f, Yaw, 0.0f), FrameLocation, FVector(FrameScaleX, FrameScaleY, FrameScaleZ));
                VerticalFrameComponent->AddInstance(FrameTransform);
            }

            // Middle frames
            float FrameOffset = FrameThickness + SectionLength + FrameThickness * 0.5f;
            for (int32 i = 0; i < ActualSectionCount - 1; ++i)
            {
                const FVector2D FrameCenter2D = Start + UnitDirection * FrameOffset;
                const FVector FrameLocation(FrameCenter2D.X, FrameCenter2D.Y, WindowCenterZ);
                const FTransform FrameTransform(FRotator(0.0f, Yaw, 0.0f), FrameLocation, FVector(FrameScaleX, FrameScaleY, FrameScaleZ));
                VerticalFrameComponent->AddInstance(FrameTransform);

                FrameOffset += SectionLength + FrameThickness;
            }

            // End frame
            {
                const FVector2D FrameCenter2D = Start + UnitDirection * (TotalLength - FrameThickness * 0.5f);
                const FVector FrameLocation(FrameCenter2D.X, FrameCenter2D.Y, WindowCenterZ);
                const FTransform FrameTransform(FRotator(0.0f, Yaw, 0.0f), FrameLocation, FVector(FrameScaleX, FrameScaleY, FrameScaleZ));
                VerticalFrameComponent->AddInstance(FrameTransform);
            }
        }

        // Horizontal frames (top and bottom, full length)
        UInstancedStaticMeshComponent* HorizontalFrameComponent = GetOrCreateISMC(WindowFrameMesh.Get(), FName(TEXT("WindowFrameHorizontal")), WindowFrameMaterialOverride.Get());
        if (HorizontalFrameComponent)
        {
            const FVector FrameMeshSize = WindowFrameMesh->GetBounds().BoxExtent * 2.0f;
            const float FrameScaleX = TotalLength / FMath::Max(FrameMeshSize.X, KINDA_SMALL_NUMBER);
            const float FrameScaleY = WindowFrameDepth / FMath::Max(FrameMeshSize.Y, KINDA_SMALL_NUMBER);
            const float FrameScaleZ = FrameThickness / FMath::Max(FrameMeshSize.Z, KINDA_SMALL_NUMBER);

            const FVector2D MidPoint2D = (Start + End) * 0.5f;
            const float BottomZ = WindowBottomZ - FrameThickness * 0.5f;
            const float TopZ = WindowBottomZ + WindowHeight + FrameThickness * 0.5f;

            // Bottom frame
            {
                const FVector FrameLocation(MidPoint2D.X, MidPoint2D.Y, BottomZ);
                const FTransform FrameTransform(FRotator(0.0f, Yaw, 0.0f), FrameLocation, FVector(FrameScaleX, FrameScaleY, FrameScaleZ));
                HorizontalFrameComponent->AddInstance(FrameTransform);
            }

            // Top frame
            {
                const FVector FrameLocation(MidPoint2D.X, MidPoint2D.Y, TopZ);
                const FTransform FrameTransform(FRotator(0.0f, Yaw, 0.0f), FrameLocation, FVector(FrameScaleX, FrameScaleY, FrameScaleZ));
                HorizontalFrameComponent->AddInstance(FrameTransform);
            }
        }
    }
}

void AProceduralOfficeGenerator::PlaceSpawnPoint(const FVector2D& Location, float HeightOffset, float Yaw)
{
    const FVector SpawnLocation(Location.X, Location.Y, FloorHeight + HeightOffset);
    const FRotator SpawnRotation(0.0f, Yaw, 0.0f);

    const FName ComponentName = MakeUniqueObjectName(this, UChildActorComponent::StaticClass(), FName(TEXT("ProceduralSpawnPoint")));
    UChildActorComponent* SpawnComponent = NewObject<UChildActorComponent>(this, ComponentName);
    if (!SpawnComponent)
    {
        return;
    }

    SpawnComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
    SpawnComponent->SetMobility(EComponentMobility::Static);
    SpawnComponent->SetupAttachment(Root);
    SpawnComponent->SetChildActorClass(APlayerStart::StaticClass());
    SpawnComponent->RegisterComponent();
    SpawnComponent->SetRelativeTransform(FTransform(SpawnRotation, SpawnLocation));
    SpawnComponent->UpdateComponentToWorld();

    const FTransform ComponentTransform = SpawnComponent->GetComponentTransform();

    if (APlayerStart* PlayerStart = Cast<APlayerStart>(SpawnComponent->GetChildActor()))
    {
        PlayerStart->SetActorTransform(ComponentTransform);
        if (!SpawnPointTag.IsNone())
        {
            PlayerStart->PlayerStartTag = SpawnPointTag;
            PlayerStart->Tags.AddUnique(SpawnPointTag);
        }
    }

    SpawnedChildActors.Add(SpawnComponent);
}

void AProceduralOfficeGenerator::PlaceCubicle(const FVector2D& Center, const FVector2D& Size, float Yaw)
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
    const auto Rotate2D = [&](const FVector2D& Local) -> FVector2D
    {
        return FVector2D(Local.X * CosYaw - Local.Y * SinYaw, Local.X * SinYaw + Local.Y * CosYaw);
    };

    auto AddPartitionSegment = [&](const FVector2D& LocalCenter, float SegmentYawDegrees, float SegmentLength)
    {
        UInstancedStaticMeshComponent* Component = GetOrCreateISMC(CubiclePartitionMesh.Get(), FName(TEXT("CubiclePartition")), CubiclePartitionMaterialOverride.Get());
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
        UInstancedStaticMeshComponent* DeskComponent = GetOrCreateISMC(CubicleDeskMesh.Get(), FName(TEXT("CubicleDesk")), CubicleDeskMaterialOverride.Get());
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
        UInstancedStaticMeshComponent* ChairComponent = GetOrCreateISMC(CubicleChairMesh.Get(), FName(TEXT("CubicleChair")), CubicleChairMaterialOverride.Get());
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

void AProceduralOfficeGenerator::PlaceCeilingLights(const FVector2D& Start, const FVector2D& End, const FVector2D& Spacing, const FVector2D& Padding)
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

    UInstancedStaticMeshComponent* Component = GetOrCreateISMC(CeilingLightMesh.Get(), FName(TEXT("CeilingLight")), CeilingLightMaterialOverride.Get());
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

    // Calculate how many lights fit and center the grid
    const float AreaWidth = MaxX - MinX;
    const float AreaHeight = MaxY - MinY;
    const int32 CountX = FMath::Max(1, FMath::FloorToInt(AreaWidth / SpacingX) + 1);
    const int32 CountY = FMath::Max(1, FMath::FloorToInt(AreaHeight / SpacingY) + 1);
    const float TotalSpanX = (CountX - 1) * SpacingX;
    const float TotalSpanY = (CountY - 1) * SpacingY;
    const float StartX = MinX + (AreaWidth - TotalSpanX) * 0.5f;
    const float StartY = MinY + (AreaHeight - TotalSpanY) * 0.5f;

    for (int32 iX = 0; iX < CountX; ++iX)
    {
        const float X = StartX + iX * SpacingX;
        const float AdjustedX = X - ScaledOrigin.X;
        for (int32 iY = 0; iY < CountY; ++iY)
        {
            const float Y = StartY + iY * SpacingY;
            const FVector MeshLocation(AdjustedX, Y - ScaledOrigin.Y, LightZ);
            const FTransform LightTransform(FRotator::ZeroRotator, MeshLocation, CeilingLightScale);
            Component->AddInstance(LightTransform);

            if (bShouldSpawnLights)
            {
                URectLightComponent* NewLight = NewObject<URectLightComponent>(this);
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

void AProceduralOfficeGenerator::PlaceDoor(const FOfficeElementDefinition& Element)
{
    const FVector2D Direction2D = Element.End - Element.Start;
    const float TotalSpan = Direction2D.Size();
    if (TotalSpan <= KINDA_SMALL_NUMBER)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Door element requires unique start and end points."));
        return;
    }

    const FVector2D UnitDirection2D = Direction2D / TotalSpan;
    const FVector2D PerpDirection2D(-UnitDirection2D.Y, UnitDirection2D.X);
    const FVector2D Center2D = (Element.Start + Element.End) * 0.5f;
    const float BaseYaw = FMath::Atan2(Direction2D.Y, Direction2D.X) * (180.0f / PI);

    const FRotator BaseRotation(0.0f, BaseYaw + 90.0f, 0.0f);
    const FVector Forward = BaseRotation.RotateVector(FVector::ForwardVector);
    const FVector Right = BaseRotation.RotateVector(FVector::RightVector);
    const FVector BaseLocation(Center2D.X, Center2D.Y, FloorHeight + Element.HeightOffset);

    auto MakeWorldLocation = [&](const FVector& LocalOffset) -> FVector
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
        if (UChildActorComponent* DoorComponent = NewObject<UChildActorComponent>(this, ComponentName))
        {
            DoorComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
            DoorComponent->SetMobility(EComponentMobility::Static);
            DoorComponent->SetupAttachment(Root);
            DoorComponent->SetChildActorClass(DoorActorClass);
            DoorComponent->RegisterComponent();
            DoorComponent->SetWorldTransform(FTransform(DoorRotation, DoorLocation, DoorScale));
            DoorComponent->UpdateComponentToWorld();

            if (AActor* DoorActor = DoorComponent->GetChildActor())
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

    const float DoorWidthScale = FMath::Max(DoorActorScale.Y, KINDA_SMALL_NUMBER);
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

    const float WallHeight = FMath::Max(CeilingHeight - FloorHeight, 0.0f);
    const float RequestedDoorHeight = Element.Height;
    const float BaseDoorHeight = RequestedDoorHeight > KINDA_SMALL_NUMBER ? FMath::Min(RequestedDoorHeight, WallHeight) : FMath::Min(WallHeight * 0.75f, WallHeight);
    const float DoorHeightScale = FMath::Max(DoorActorScale.Z, KINDA_SMALL_NUMBER);
    const float FinalDoorHeight = FMath::Min(BaseDoorHeight * DoorHeightScale, WallHeight);
    const float FillHeight = WallHeight - FinalDoorHeight;

    if (FillHeight > KINDA_SMALL_NUMBER)
    {
        UInstancedStaticMeshComponent* FillComponent = GetOrCreateISMC(WallMesh.Get(), FName(TEXT("DoorTopFill")), WallMaterialOverride.Get());
        if (FillComponent)
        {
            const FVector MeshSize = WallMesh->GetBounds().BoxExtent * 2.0f;
            const float DesiredThickness = FMath::Max(WallThickness, KINDA_SMALL_NUMBER);
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

UInstancedStaticMeshComponent* AProceduralOfficeGenerator::GetOrCreateISMC(UStaticMesh* Mesh, const FName& ComponentName, UMaterialInterface* OverrideMaterial)
{
    if (!Mesh)
    {
        return nullptr;
    }

    const FName CacheKey = ComponentName;

    if (UInstancedStaticMeshComponent** Found = InstancedCache.Find(CacheKey))
    {
        return *Found;
    }

    UInstancedStaticMeshComponent* NewComponent = NewObject<UInstancedStaticMeshComponent>(this, CacheKey);
    if (!NewComponent)
    {
        return nullptr;
    }

    NewComponent->SetMobility(EComponentMobility::Static);
    NewComponent->SetStaticMesh(Mesh);
    if (OverrideMaterial)
    {
        NewComponent->SetMaterial(0, OverrideMaterial);
    }
    NewComponent->SetupAttachment(Root);
    NewComponent->RegisterComponent();

    SpawnedInstancedComponents.Add(NewComponent);
    InstancedCache.Add(CacheKey, NewComponent);

    return NewComponent;
}

void AProceduralOfficeGenerator::DestroySpawnedComponents()
{
    for (UInstancedStaticMeshComponent* Component : SpawnedInstancedComponents)
    {
        if (Component)
        {
            Component->DestroyComponent();
        }
    }
    SpawnedInstancedComponents.Empty();
    InstancedCache.Empty();

    for (UChildActorComponent* ChildComponent : SpawnedChildActors)
    {
        if (ChildComponent)
        {
            ChildComponent->DestroyComponent();
        }
    }
    SpawnedChildActors.Empty();

    for (URectLightComponent* LightComponent : SpawnedCeilingLights)
    {
        if (LightComponent)
        {
            LightComponent->DestroyComponent();
        }
    }
    SpawnedCeilingLights.Empty();
}
