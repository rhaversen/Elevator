#include "ProceduralOfficeGenerator.h"
#include "Procedural/ProceduralOfficeGenerator.Helpers.h"
#include "Procedural/ProceduralOfficeGenerator.Log.h"

#include "Components/ChildActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "GameFramework/PlayerStart.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Core lifecycle and shared logic for the procedural office generator lives in this translation unit.

DEFINE_LOG_CATEGORY(LogProceduralOffice);

AProceduralOfficeGenerator::AProceduralOfficeGenerator()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    LayoutFileRelativePath = TEXT("Layouts/ExampleOpenOffice.json");
}

void AProceduralOfficeGenerator::OnConstruction(const FTransform &Transform)
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
void AProceduralOfficeGenerator::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
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
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, DoorWallPadding) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, bSpawnElevatorLightComponents) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorLightIntensity) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorLightAttenuationRadius) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorLightColor) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, bElevatorLightsCastShadows) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorLightVerticalOffset) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorRectLightSourceWidth) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorRectLightSourceHeight) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorRectLightBarnDoorAngle) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorRectLightBarnDoorLength) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorWallPadding) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorWallInset))
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

bool AProceduralOfficeGenerator::LoadLayoutData(FOfficeLayout &OutLayout) const
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

void AProceduralOfficeGenerator::BuildFromLayout(const FOfficeLayout &Layout)
{
    if (Layout.Elements.IsEmpty())
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Layout contains no elements."));
        return;
    }

    for (const FOfficeElementDefinition &Element : Layout.Elements)
    {
        BuildElement(Element);
    }
}

void AProceduralOfficeGenerator::BuildElement(const FOfficeElementDefinition &Element)
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
        case EOfficeElementType::Elevator:
            PlaceElevator(Element);
            break;
        default:
            UE_LOG(LogProceduralOffice, Warning, TEXT("Unsupported element type encountered."));
            break;
    }
}

UInstancedStaticMeshComponent *AProceduralOfficeGenerator::GetOrCreateISMC(UStaticMesh *Mesh, const FName &ComponentName, UMaterialInterface *OverrideMaterial)
{
    if (!Mesh)
    {
        return nullptr;
    }

    if (UInstancedStaticMeshComponent **Found = InstancedCache.Find(ComponentName))
    {
        return *Found;
    }

    UInstancedStaticMeshComponent *NewComponent = NewObject<UInstancedStaticMeshComponent>(this, ComponentName);
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
    InstancedCache.Add(ComponentName, NewComponent);

    return NewComponent;
}

void AProceduralOfficeGenerator::DestroySpawnedComponents()
{
    for (UInstancedStaticMeshComponent *Component : SpawnedInstancedComponents)
    {
        if (Component)
        {
            Component->DestroyComponent();
        }
    }
    SpawnedInstancedComponents.Empty();
    InstancedCache.Empty();

    for (UChildActorComponent *ChildComponent : SpawnedChildActors)
    {
        if (ChildComponent)
        {
            ChildComponent->DestroyComponent();
        }
    }
    SpawnedChildActors.Empty();

    for (URectLightComponent *LightComponent : SpawnedCeilingLights)
    {
        if (LightComponent)
        {
            LightComponent->DestroyComponent();
        }
    }
    SpawnedCeilingLights.Empty();

    for (URectLightComponent *LightComponent : SpawnedElevatorLights)
    {
        if (LightComponent)
        {
            LightComponent->DestroyComponent();
        }
    }
    SpawnedElevatorLights.Empty();
}
