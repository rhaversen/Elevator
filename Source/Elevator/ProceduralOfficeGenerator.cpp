#include "ProceduralOfficeGenerator.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, RandomSeed) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WallModuleLength) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingHeight) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubiclePadding))
        {
            GenerateFromData();
        }
    }
}
#endif

void AProceduralOfficeGenerator::GenerateFromData()
{
    DestroySpawnedComponents();

    FProceduralOfficeLayout Layout;
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

bool AProceduralOfficeGenerator::LoadLayoutData(FProceduralOfficeLayout& OutLayout) const
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

void AProceduralOfficeGenerator::BuildFromLayout(const FProceduralOfficeLayout& Layout)
{
    if (Layout.Rooms.IsEmpty())
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Layout contains no rooms."));
        return;
    }

    FRandomStream Rng(RandomSeed);
    for (const FProceduralRoomDefinition& Room : Layout.Rooms)
    {
        BuildRoom(Room, Rng);
    }
}

void AProceduralOfficeGenerator::BuildRoom(const FProceduralRoomDefinition& Room, FRandomStream& Rng)
{
    const FVector RoomOrigin(Room.Origin.X, Room.Origin.Y, 0.0f);
    PlaceWalls(RoomOrigin, Room.Size, CeilingHeight, Rng, Room.Doors);

    if (Room.Usage == EProceduralRoomType::OpenWorkspace && Room.CubicleGrid.X > 0 && Room.CubicleGrid.Y > 0)
    {
        PlaceCubicles(Room, Rng);
    }

    SpawnProps(Room, Rng);
}

void AProceduralOfficeGenerator::PlaceWalls(const FVector& RoomOrigin, const FVector2D& Size, float Height, FRandomStream& Rng, const TArray<FProceduralDoorwayDefinition>& Doors)
{
    if (WallSegmentMeshes.IsEmpty())
    {
        return;
    }

    const FVector Extents(Size.X * 0.5f, Size.Y * 0.5f, 0.0f);
    const FVector Corners[4] = {
        RoomOrigin + FVector(-Extents.X, -Extents.Y, 0.0f),
        RoomOrigin + FVector( Extents.X, -Extents.Y, 0.0f),
        RoomOrigin + FVector( Extents.X,  Extents.Y, 0.0f),
        RoomOrigin + FVector(-Extents.X,  Extents.Y, 0.0f)
    };

    const auto ShouldSkipForDoor = [&Doors, &RoomOrigin](const FVector& SegmentCenter, float SegmentHalfLength, bool bAlignedOnX) -> bool
    {
        for (const FProceduralDoorwayDefinition& Door : Doors)
        {
            const FVector DoorWorld(RoomOrigin.X + Door.Location.X, RoomOrigin.Y + Door.Location.Y, SegmentCenter.Z);
            if (bAlignedOnX)
            {
                if (FMath::IsNearlyEqual(DoorWorld.Y, SegmentCenter.Y, SegmentHalfLength) &&
                    FMath::Abs(DoorWorld.X - SegmentCenter.X) <= (Door.Width * 0.5f))
                {
                    return true;
                }
            }
            else
            {
                if (FMath::IsNearlyEqual(DoorWorld.X, SegmentCenter.X, SegmentHalfLength) &&
                    FMath::Abs(DoorWorld.Y - SegmentCenter.Y) <= (Door.Width * 0.5f))
                {
                    return true;
                }
            }
        }
        return false;
    };

    for (int32 EdgeIndex = 0; EdgeIndex < 4; ++EdgeIndex)
    {
        const FVector Start = Corners[EdgeIndex];
        const FVector End = Corners[(EdgeIndex + 1) % 4];
        const FVector EdgeVector = End - Start;
        const float EdgeLength = EdgeVector.Size2D();
        const FVector Direction = EdgeVector.GetSafeNormal2D();
        const bool bAlignedOnX = FMath::Abs(Direction.X) > FMath::Abs(Direction.Y);
        const float Step = FMath::Max(WallModuleLength, 1.0f);
        const int32 SegmentCount = FMath::Max(1, FMath::CeilToInt(EdgeLength / Step));
        const float HalfLength = Step * 0.5f;

        const float MaxDistance = FMath::Max(HalfLength, EdgeLength - HalfLength);

        for (int32 SegmentIdx = 0; SegmentIdx < SegmentCount; ++SegmentIdx)
        {
            const float DistanceAlong = FMath::Clamp((SegmentIdx + 0.5f) * Step, HalfLength, MaxDistance);
            const FVector SegmentCenter = Start + Direction * DistanceAlong;

            if (ShouldSkipForDoor(SegmentCenter, HalfLength, bAlignedOnX))
            {
                continue;
            }

            UStaticMesh* WallMesh = WallSegmentMeshes[Rng.RandRange(0, WallSegmentMeshes.Num() - 1)].Get();
            if (!WallMesh)
            {
                continue;
            }

            const int32 MaterialVariant = WallMaterialVariants.Num() > 0 ? Rng.RandRange(0, WallMaterialVariants.Num() - 1) : INDEX_NONE;
            UInstancedStaticMeshComponent* WallComponent = GetOrCreateISMC(WallMesh, FName(*FString::Printf(TEXT("Wall_%s"), *WallMesh->GetName())), WallMaterialVariants, MaterialVariant);
            if (!WallComponent)
            {
                continue;
            }

            const FRotator Rotation(0.0f, Direction.Rotation().Yaw, 0.0f);
            const FVector Scale(1.0f, Step / FMath::Max(1.0f, WallMesh->GetBounds().BoxExtent.Y * 2.0f), Height / FMath::Max(1.0f, WallMesh->GetBounds().BoxExtent.Z * 2.0f));
            const FTransform InstanceTransform(Rotation, SegmentCenter + FVector(0.0f, 0.0f, Height * 0.5f), Scale);
            WallComponent->AddInstance(InstanceTransform);
        }
    }

    if (!DoorwayMeshes.IsEmpty())
    {
        for (const FProceduralDoorwayDefinition& Door : Doors)
        {
            UStaticMesh* DoorMesh = DoorwayMeshes[Rng.RandRange(0, DoorwayMeshes.Num() - 1)].Get();
            if (!DoorMesh)
            {
                continue;
            }

            const int32 MaterialVariant = WallMaterialVariants.Num() > 0 ? Rng.RandRange(0, WallMaterialVariants.Num() - 1) : INDEX_NONE;
            UInstancedStaticMeshComponent* DoorComponent = GetOrCreateISMC(DoorMesh, FName(*FString::Printf(TEXT("Door_%s"), *DoorMesh->GetName())), WallMaterialVariants, MaterialVariant);
            if (!DoorComponent)
            {
                continue;
            }

            const FVector DoorWorld(RoomOrigin.X + Door.Location.X, RoomOrigin.Y + Door.Location.Y, Height * 0.5f);
            const FVector DoorScale(Door.Width / FMath::Max(1.0f, DoorMesh->GetBounds().BoxExtent.X * 2.0f),
                                    1.0f,
                                    Height / FMath::Max(1.0f, DoorMesh->GetBounds().BoxExtent.Z * 2.0f));
            const FTransform InstanceTransform(FRotator(0.0f, Door.FacingYaw, 0.0f), DoorWorld, DoorScale);
            DoorComponent->AddInstance(InstanceTransform);
        }
    }
}

void AProceduralOfficeGenerator::PlaceCubicles(const FProceduralRoomDefinition& Room, FRandomStream& Rng)
{
    if (CubicleMeshes.IsEmpty())
    {
        return;
    }

    const FVector RoomOrigin(Room.Origin.X, Room.Origin.Y, 0.0f);
    const FVector2D CellSize(Room.Size.X / Room.CubicleGrid.X, Room.Size.Y / Room.CubicleGrid.Y);
    const FVector Offset(-Room.Size.X * 0.5f, -Room.Size.Y * 0.5f, 0.0f);

    for (int32 X = 0; X < Room.CubicleGrid.X; ++X)
    {
        for (int32 Y = 0; Y < Room.CubicleGrid.Y; ++Y)
        {
            if (Rng.FRand() < 0.1f)
            {
                continue; // leave occasional gaps to break repetition
            }

            UStaticMesh* CubicleMesh = CubicleMeshes[Rng.RandRange(0, CubicleMeshes.Num() - 1)].Get();
            if (!CubicleMesh)
            {
                continue;
            }

            const FVector LocalPosition(Offset.X + (X + 0.5f) * CellSize.X, Offset.Y + (Y + 0.5f) * CellSize.Y, 0.0f);
            const FVector WorldPosition = RoomOrigin + LocalPosition;

            const FVector Scale((CellSize.X - CubiclePadding * 2.0f) / FMath::Max(1.0f, CubicleMesh->GetBounds().BoxExtent.X * 2.0f),
                                 (CellSize.Y - CubiclePadding * 2.0f) / FMath::Max(1.0f, CubicleMesh->GetBounds().BoxExtent.Y * 2.0f),
                                 1.0f);

            const int32 MaterialVariant = CubicleMaterialVariants.Num() > 0 ? Rng.RandRange(0, CubicleMaterialVariants.Num() - 1) : INDEX_NONE;
            UInstancedStaticMeshComponent* CubicleComponent = GetOrCreateISMC(CubicleMesh, FName(*FString::Printf(TEXT("Cubicle_%s"), *CubicleMesh->GetName())), CubicleMaterialVariants, MaterialVariant);
            if (!CubicleComponent)
            {
                continue;
            }

            const float Orientation = (Room.CubicleGrid.X >= Room.CubicleGrid.Y) ? 0.0f : 90.0f;
            const FTransform InstanceTransform(FRotator(0.0f, Orientation, 0.0f), WorldPosition, Scale);
            CubicleComponent->AddInstance(InstanceTransform);
        }
    }
}

void AProceduralOfficeGenerator::SpawnProps(const FProceduralRoomDefinition& Room, FRandomStream& Rng)
{
    if (PropPrefabs.IsEmpty() || MaxPropsPerRoom <= 0)
    {
        return;
    }

    const FVector RoomOrigin(Room.Origin.X, Room.Origin.Y, 0.0f);
    const FVector2D HalfSize(Room.Size.X * 0.5f, Room.Size.Y * 0.5f);

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (int32 Index = 0; Index < MaxPropsPerRoom; ++Index)
    {
        const TSubclassOf<AActor> Prefab = PropPrefabs[Rng.RandRange(0, PropPrefabs.Num() - 1)];
        if (!Prefab)
        {
            continue;
        }

        const FVector RandomOffset(Rng.FRandRange(-HalfSize.X, HalfSize.X), Rng.FRandRange(-HalfSize.Y, HalfSize.Y), 0.0f);
        const FVector SpawnLocation = RoomOrigin + RandomOffset;
        const float Yaw = Rng.FRandRange(0.0f, 360.0f);

        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        if (AActor* SpawnedActor = World->SpawnActor<AActor>(Prefab, FTransform(FRotator(0.0f, Yaw, 0.0f), SpawnLocation), Params))
        {
            SpawnedProps.Add(SpawnedActor);
        }
    }
}

UInstancedStaticMeshComponent* AProceduralOfficeGenerator::GetOrCreateISMC(UStaticMesh* Mesh, const FName& ComponentName, const TArray<UMaterialInterface*>& MaterialVariants, int32 VariantIndex)
{
    if (!Mesh)
    {
        return nullptr;
    }

    FString NameString = ComponentName.ToString();
    if (VariantIndex >= 0)
    {
        NameString += FString::Printf(TEXT("_M%d"), VariantIndex);
    }
    const FName CacheKey(*NameString);

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
    if (MaterialVariants.IsValidIndex(VariantIndex))
    {
        NewComponent->SetMaterial(0, MaterialVariants[VariantIndex]);
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

    for (AActor* Prop : SpawnedProps)
    {
        if (IsValid(Prop))
        {
            Prop->Destroy();
        }
    }
    SpawnedProps.Empty();
}
