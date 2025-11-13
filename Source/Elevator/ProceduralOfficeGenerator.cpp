#include "ProceduralOfficeGenerator.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
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
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, SpawnPointTag))
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
            PlaceWall(Element.Start, Element.End, Element.Thickness);
            break;
        case EOfficeElementType::SpawnPoint:
            PlaceSpawnPoint(Element.Start, Element.HeightOffset, Element.Yaw);
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

void AProceduralOfficeGenerator::PlaceWall(const FVector2D& Start, const FVector2D& End, float Thickness)
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
    const float DesiredThickness = Thickness > 0.0f ? Thickness : MeshSize.Y;
    const float ScaleY = DesiredThickness / FMath::Max(MeshSize.Y, KINDA_SMALL_NUMBER);

    const FVector2D Midpoint2D = (Start + End) * 0.5f;
    const FVector Location(Midpoint2D.X, Midpoint2D.Y, FloorHeight + WallHeight * 0.5f);
    const float Yaw = FMath::Atan2(Direction2D.Y, Direction2D.X) * (180.0f / PI);
    const FTransform InstanceTransform(FRotator(0.0f, Yaw, 0.0f), Location, FVector(ScaleX, ScaleY, ScaleZ));
    Component->AddInstance(InstanceTransform);
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
}
