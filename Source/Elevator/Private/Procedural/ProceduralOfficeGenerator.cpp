#include "Procedural/ProceduralOfficeGenerator.h"
#include "Procedural/ProceduralOfficeGenerator.Helpers.h"
#include "Procedural/ProceduralOfficeGenerator.Log.h"
#include "Procedural/ProceduralElevator.h"
#include "UI/InteractiveScreenComponent.h"
#include "UI/Programs/SimpleButtonProgram.h"
#include "System/ElevatorGameManagerSubsystem.h"
#include "FirstPersonCharacter.h"

#include "Components/ChildActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/PlayerStart.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/TextureRenderTarget2D.h"

// Core lifecycle and shared logic for the procedural office generator lives in this translation unit.

DEFINE_LOG_CATEGORY(LogProceduralOffice);

const FName AProceduralOfficeGenerator::WorkstationMonitorComponentKey(TEXT("CubicleComputer"));
const FName AProceduralOfficeGenerator::WorkstationMonitorTag(TEXT("WorkstationMonitor"));

AProceduralOfficeGenerator::AProceduralOfficeGenerator()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    LayoutFileRelativePath = TEXT("Layouts/ExampleOpenOffice.json");

    ElevatorActorClass = AProceduralElevator::StaticClass();

    MonitorScreenComponent = CreateDefaultSubobject<UInteractiveScreenComponent>(TEXT("MonitorScreenComponent"));

    MonitorInteractionDebugProxy = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MonitorInteractionDebugProxy"));
    MonitorInteractionDebugProxy->SetupAttachment(Root);
    MonitorInteractionDebugProxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MonitorInteractionDebugProxy->SetCastShadow(false);
    MonitorInteractionDebugProxy->SetVisibility(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        MonitorInteractionDebugProxy->SetStaticMesh(CubeMesh.Object);
    }
    // Use a semi-transparent material if possible, or just default. 
    // For now default is fine, user can see the block.
}

AProceduralOfficeGenerator::~AProceduralOfficeGenerator() = default;

void AProceduralOfficeGenerator::OnConstruction(const FTransform &Transform)
{
    Super::OnConstruction(Transform);

    if (MonitorScreenComponent)
    {
        MonitorScreenComponent->SetRenderTarget(ScreenRenderTarget);
    }

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

    InitializeMonitorScreen();

    if (UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
    {
        if (!ProgramChangedHandle.IsValid())
        {
            ProgramChangedHandle = Manager->OnProgramChanged().AddUObject(this, &AProceduralOfficeGenerator::HandleActiveProgramChanged);
            UE_LOG(LogProceduralOffice, Log, TEXT("[Workstation] Subscribed to program change notifications."));
        }

        HandleActiveProgramChanged(Manager->GetActiveProgramId());
    }
}

void AProceduralOfficeGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ProgramChangedHandle.IsValid())
    {
        if (UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
        {
            Manager->OnProgramChanged().Remove(ProgramChangedHandle);
        }
        ProgramChangedHandle.Reset();
        UE_LOG(LogProceduralOffice, Log, TEXT("[Workstation] Unsubscribed from program change notifications."));
    }

    DestroySpawnedComponents();
    Super::EndPlay(EndPlayReason);
}

void AProceduralOfficeGenerator::InitializeMonitorScreen()
{
    if (!MonitorScreenComponent)
    {
        return;
    }

    MonitorScreenComponent->SetRenderTarget(ScreenRenderTarget);

    if (UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
    {
        UE_LOG(LogProceduralOffice, Log, TEXT("[Workstation] Initializing monitor with manager-selected program %s (Day %d)."),
            *Manager->GetActiveProgramId().ToString(),
            Manager->GetCurrentDay());
        MonitorScreenComponent->SetProgram(Manager->CreateActiveProgramInstance());
    }
    else
    {
        UE_LOG(LogProceduralOffice, Log, TEXT("[Workstation] Manager unavailable; using fallback SimpleButton program."));
        MonitorScreenComponent->SetProgram(MakeShared<FSimpleButtonProgram>());
    }
    
    MonitorScreenComponent->InitializeScreen();

    if (MonitorScreenComponent->IsReady())
    {
        UE_LOG(LogProceduralOffice, Display, TEXT("Rendered Slate UI to render target %s"), *GetNameSafe(ScreenRenderTarget));
    }
    else if (!ScreenRenderTarget)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("ScreenRenderTarget is not set. Please assign RT_ScreenInterface in the details panel."));
    }
}

void AProceduralOfficeGenerator::HandleActiveProgramChanged(FName ProgramId)
{
    if (!MonitorScreenComponent)
    {
        return;
    }

    if (UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
    {
        const FName EffectiveProgramId = ProgramId.IsNone() ? Manager->GetActiveProgramId() : ProgramId;
        UE_LOG(LogProceduralOffice, Log, TEXT("[Workstation] Switching monitor to program %s for Day %d."),
            *EffectiveProgramId.ToString(),
            Manager->GetCurrentDay());
        MonitorScreenComponent->SetProgram(Manager->CreateProgramInstanceForId(ProgramId));
        MonitorScreenComponent->InitializeScreen();
        MonitorScreenComponent->ResetCursor();
    }
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
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleChairRelativeLocation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubiclePartitionWidthScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubiclePartitionDepthScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerOffset) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerHeightOffset) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerYawOffset) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerRelativeLocation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerRelativeRotation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleKeyboardMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleKeyboardMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleKeyboardRelativeLocation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleKeyboardRelativeRotation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleKeyboardScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleMouseMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleMouseMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleMouseRelativeLocation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleMouseRelativeRotation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleMouseScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleDeskLampMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleDeskLampMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleDeskLampRelativeLocation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleDeskLampRelativeRotation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleDeskLampScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleMousePadMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleMousePadMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleMousePadRelativeLocation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleMousePadRelativeRotation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleMousePadScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerTowerMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerTowerMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerTowerRelativeLocation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerTowerRelativeRotation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleComputerTowerScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubiclePhoneMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubiclePhoneMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubiclePhoneRelativeLocation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubiclePhoneRelativeRotation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubiclePhoneScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleNotepadMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleNotepadMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleNotepadRelativeLocation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleNotepadRelativeRotation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CubicleNotepadScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WorkstationInteractionTargetOffset) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WorkstationInteractionMoveDuration) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WorkstationInteractionArcHeight) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, WorkstationInteractionCurveBias) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, bShowWorkstationTargetPreview) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightMesh) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightMaterialOverride) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, bSpawnCeilingLightComponents) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightIntensity) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightAttenuationRadius) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightColor) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, bCeilingLightsCastShadows) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, CeilingLightVerticalOffset) ||
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
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorWallInset) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorActorClass) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorActorOffset) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorActorScale) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorDefaultWidth) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorDefaultDepth) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorDefaultHeight) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, MonitorScreenOffset) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, MonitorScreenRotation) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, MonitorScreenSize) ||
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, bShowMonitorInteractionDebug))
        {
            GenerateFromData();
        }

        if (Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ScreenRenderTarget))
        {
            InitializeMonitorScreen();
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

    UpdateMonitorInteractionDebug();
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

    ResolveComputerMeshComponent();
    HideComputerHighlight();
    RefreshWorkstationTargetPreview();
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
            PlaceCeilingLights(Element.Start, Element.End, Element.Spacing, Element.Padding, Element.Yaw);
            break;
        case EOfficeElementType::Door:
            PlaceDoor(Element);
            break;
        case EOfficeElementType::Elevator:
            PlaceElevator(Element);
            break;
        case EOfficeElementType::RoomTone:
            PlaceRoomTone(Element);
            break;
        default:
            UE_LOG(LogProceduralOffice, Warning, TEXT("Unsupported element type encountered."));
            break;
    }
}

void AProceduralOfficeGenerator::PlaceRoomTone(const FOfficeElementDefinition& Element)
{
    USoundBase* Sound = nullptr;
    if (Element.AudioId == FName("Office")) Sound = AudioRegistry.OfficeRoomTone;
    else if (Element.AudioId == FName("Annex")) Sound = AudioRegistry.AnnexRoomTone;
    else if (Element.AudioId == FName("Outside")) Sound = AudioRegistry.OutsideRoomTone;

    if (!Sound)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("RoomTone audio not found: %s"), *Element.AudioId.ToString());
        return;
    }

    Sound->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;

    UAudioComponent* AudioComp = NewObject<UAudioComponent>(this);
    AudioComp->SetSound(Sound);
    AudioComp->SetWorldLocation(FVector(Element.Start.X, Element.Start.Y, FloorHeight + Element.HeightOffset));
    AudioComp->SetVolumeMultiplier(Element.VolumeMultiplier);
    
    AudioComp->bAllowSpatialization = true;
    AudioComp->bOverrideAttenuation = true;
    AudioComp->AttenuationOverrides.bAttenuate = true;
    AudioComp->AttenuationOverrides.AttenuationShape = EAttenuationShape::Sphere;
    AudioComp->AttenuationOverrides.AttenuationShapeExtents = FVector(Element.AttenuationRadius);
    AudioComp->AttenuationOverrides.FalloffDistance = Element.AttenuationRadius;
    AudioComp->AttenuationOverrides.dBAttenuationAtMax = -60.0f;
    
    if (Element.bOmnidirectional)
    {
        // Omnidirectional: sound comes equally from all directions (no spatial positioning)
        AudioComp->AttenuationOverrides.bSpatialize = false;
    }
    else
    {
        // Directional: sound comes from a specific location in 3D space
        AudioComp->AttenuationOverrides.bSpatialize = true;
    }
    
    AudioComp->SetupAttachment(Root);
    AudioComp->RegisterComponent();
    AudioComp->Play();
    
    SpawnedAudioComponents.Add(AudioComp);
}

UInstancedStaticMeshComponent *AProceduralOfficeGenerator::GetOrCreateISMC(UStaticMesh *Mesh, const FName &ComponentName, UMaterialInterface *OverrideMaterial)
{
    if (!Mesh)
    {
        return nullptr;
    }

    if (TObjectPtr<UInstancedStaticMeshComponent>* Found = InstancedCache.Find(ComponentName))
    {
        return Found->Get();
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

    for (UChildActorComponent *Component : SpawnedChildActors)
    {
        if (Component)
        {
            Component->DestroyComponent();
        }
    }
    SpawnedChildActors.Empty();

    for (UAudioComponent *Component : SpawnedAudioComponents)
    {
        if (Component)
        {
            Component->DestroyComponent();
        }
    }
    SpawnedAudioComponents.Empty();
}

bool AProceduralOfficeGenerator::EvaluateInteractionFocus_Implementation(APawn *PlayerPawn, const FHitResult &Hit, float AssistRadius, UPrimitiveComponent *&OutHighlightComponent)
{
    OutHighlightComponent = nullptr;
    HoveredComputerInstanceIndex = INDEX_NONE;
    bHasPendingWorkstationViewTransform = false;

    UInstancedStaticMeshComponent* ActiveComputerComponent = ComputerMeshComponent.Get();
    if (!IsValid(ActiveComputerComponent))
    {
        ActiveComputerComponent = ResolveComputerMeshComponent();
    }

    if (!IsValid(ActiveComputerComponent))
    {
        HideComputerHighlight();
        return false;
    }

    const UPrimitiveComponent* HitComponent = Hit.GetComponent();
    const bool bHitMonitorComponent = (HitComponent == ActiveComputerComponent);

    if (!bHitMonitorComponent)
    {
        HideComputerHighlight();
        return false;
    }

    int32 InstanceIndex = Hit.Item >= 0 ? Hit.Item : INDEX_NONE;

    if (InstanceIndex == INDEX_NONE && AssistRadius > 0.0f)
    {
        const FVector SearchOrigin = Hit.ImpactPoint.IsNearlyZero() ? Hit.Location : Hit.ImpactPoint;
        InstanceIndex = FindClosestComputerInstance(SearchOrigin, AssistRadius);
    }

    if (InstanceIndex == INDEX_NONE)
    {
        HideComputerHighlight();
        return false;
    }

    NotifyComputerLookedAt(ActiveComputerComponent, InstanceIndex);

    FTransform InstanceTransform;
    if (!ActiveComputerComponent->GetInstanceTransform(InstanceIndex, InstanceTransform, true))
    {
        HideComputerHighlight();
        return false;
    }

    const FTransform TargetTransform = WorkstationInteractionTargetOffset * InstanceTransform;
    PendingWorkstationViewTransform = TargetTransform;
    bHasPendingWorkstationViewTransform = true;

    if (UStaticMeshComponent* HighlightProxyComponent = GetOrCreateComputerHighlightProxy(ActiveComputerComponent))
    {
        HighlightProxyComponent->SetWorldTransform(InstanceTransform);
        HighlightProxyComponent->SetVisibility(true);
        HighlightProxyComponent->SetRenderCustomDepth(true);
        HighlightProxyComponent->SetCustomDepthStencilValue(252);
        HighlightProxyComponent->MarkRenderTransformDirty();
        OutHighlightComponent = HighlightProxyComponent;
    }
    else
    {
        OutHighlightComponent = ActiveComputerComponent;
    }
    return true;
}

bool AProceduralOfficeGenerator::CanInteract_Implementation(APawn *PlayerPawn) const
{
    return HoveredComputerInstanceIndex != INDEX_NONE && bHasPendingWorkstationViewTransform;
}

void AProceduralOfficeGenerator::OnInteract_Implementation(APawn *PlayerPawn)
{
    if (HoveredComputerInstanceIndex == INDEX_NONE || !bHasPendingWorkstationViewTransform)
    {
        return;
    }

    if (AFirstPersonCharacter* FirstPersonCharacter = Cast<AFirstPersonCharacter>(PlayerPawn))
    {
        FirstPersonCharacter->BeginWorkstationInteraction(PendingWorkstationViewTransform, WorkstationInteractionMoveDuration, WorkstationInteractionArcHeight, WorkstationInteractionCurveBias, this);
    }

    if (MonitorScreenComponent)
    {
        MonitorScreenComponent->ResetCursor();
    }

    UE_LOG(LogProceduralOffice, Display, TEXT("Workstation monitor interaction triggered on instance %d."), HoveredComputerInstanceIndex);

    CurrentInteractionInstanceIndex = HoveredComputerInstanceIndex;
    HoveredComputerInstanceIndex = INDEX_NONE;
    bHasPendingWorkstationViewTransform = false;
    HideComputerHighlight();
}

FText AProceduralOfficeGenerator::GetInteractionPrompt_Implementation() const
{
    return HoveredComputerInstanceIndex != INDEX_NONE ? WorkstationInteractionPrompt : FText::GetEmpty();
}

void AProceduralOfficeGenerator::OnInteractionCanceled_Implementation(APawn* PlayerPawn)
{
    CurrentInteractionInstanceIndex = INDEX_NONE;

    if (MonitorScreenComponent)
    {
        MonitorScreenComponent->ResetCursor();
    }
}

void AProceduralOfficeGenerator::OnInteractionPointerPressed_Implementation(APawn* PlayerPawn, const FHitResult& Hit, FKey PointerKey)
{
    if (!MonitorScreenComponent || !MonitorScreenComponent->IsReady())
    {
        return;
    }

    MonitorScreenComponent->ProcessPointerPressed(PointerKey);
}

void AProceduralOfficeGenerator::OnInteractionPointerReleased_Implementation(APawn* PlayerPawn, const FHitResult& Hit, FKey PointerKey)
{
    if (!MonitorScreenComponent || !MonitorScreenComponent->IsReady())
    {
        return;
    }

    MonitorScreenComponent->ProcessPointerReleased(PointerKey);

    if (UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
    {
        if (MonitorScreenComponent->IsProgramTaskComplete())
        {
            UE_LOG(LogProceduralOffice, Log, TEXT("[Workstation] Program task marked complete on Day %d."), Manager->GetCurrentDay());
            Manager->SetTaskComplete(true);
        }
    }

    if (MonitorScreenComponent->ShouldExit())
    {
        UE_LOG(LogProceduralOffice, Log, TEXT("[Workstation] Program requested exit; closing workstation view."));
        if (AFirstPersonCharacter* Character = Cast<AFirstPersonCharacter>(PlayerPawn))
        {
            Character->CancelWorkstationInteraction();
        }
    }
}

void AProceduralOfficeGenerator::OnInteractionHover_Implementation(const FHitResult& Hit)
{
    if (!MonitorScreenComponent || !MonitorScreenComponent->IsReady() || CurrentInteractionInstanceIndex == INDEX_NONE)
    {
        return;
    }

    UInstancedStaticMeshComponent* ActiveComputerComponent = ComputerMeshComponent.Get();
    if (!IsValid(ActiveComputerComponent))
    {
        return;
    }

    // Verify we hit the correct instance
    if (Hit.Component != ActiveComputerComponent || Hit.Item != CurrentInteractionInstanceIndex)
    {
        return;
    }

    FTransform InstanceTransform;
    if (!ActiveComputerComponent->GetInstanceTransform(CurrentInteractionInstanceIndex, InstanceTransform, true))
    {
        return;
    }

    // Transform hit location to instance local space
    const FVector LocalHit = InstanceTransform.InverseTransformPosition(Hit.Location);

    // Transform LocalHit into Screen Space (defined by Offset and Rotation relative to Monitor Mesh)
    // Screen Space: Origin at Center of Screen.
    // X axis: Normal to screen (Forward)
    // Y axis: Right
    // Z axis: Up
    FTransform ScreenTransform(MonitorScreenRotation, MonitorScreenOffset);
    FVector ScreenSpaceHit = ScreenTransform.InverseTransformPosition(LocalHit);

    // Map to UV
    // We assume the interaction plane is the YZ plane in Screen Space (X=0).
    // Y ranges from -Width/2 to +Width/2.
    // Z ranges from -Height/2 to +Height/2.
    
    // UV X (Horizontal): 0 at Left (+Y?), 1 at Right (-Y?)
    // Previous logic: 0.5 - (Diff.Y / Width).
    // If Y is +Width/2, U = 0.5 - 0.5 = 0.
    // If Y is -Width/2, U = 0.5 - (-0.5) = 1.
    // So +Y is Left (UV=0), -Y is Right (UV=1).
    float U = 0.5f - (ScreenSpaceHit.Y / MonitorScreenSize.X);

    // UV Y (Vertical): 0 at Top (+Z), 1 at Bottom (-Z)
    // Previous logic: 0.5 - (Diff.Z / Height).
    // If Z is +Height/2, V = 0.5 - 0.5 = 0.
    // If Z is -Height/2, V = 0.5 - (-0.5) = 1.
    float V = 0.5f - (ScreenSpaceHit.Z / MonitorScreenSize.Y);

    // Clamp
    U = FMath::Clamp(U, 0.0f, 1.0f);
    V = FMath::Clamp(V, 0.0f, 1.0f);

    if (MonitorScreenComponent)
    {
        MonitorScreenComponent->UpdateCursor(FVector2D(U, V));
    }
}

void AProceduralOfficeGenerator::UpdateMonitorInteractionDebug()
{
    if (!MonitorInteractionDebugProxy) return;

    MonitorInteractionDebugProxy->SetVisibility(false);

    if (!bShowMonitorInteractionDebug) return;

    // Find the first monitor instance
    UInstancedStaticMeshComponent* MonitorComp = ResolveComputerMeshComponent();
    if (!MonitorComp || MonitorComp->GetInstanceCount() == 0) return;

    FTransform InstanceTransform;
    MonitorComp->GetInstanceTransform(0, InstanceTransform, true); // World space

    // Calculate World Transform of the Screen
    // Scale: X=Thickness (small), Y=Width, Z=Height.
    // MonitorScreenSize.X is Width, MonitorScreenSize.Y is Height.
    // Cube is 100x100x100.
    FVector Scale(0.05f, MonitorScreenSize.X / 100.0f, MonitorScreenSize.Y / 100.0f);
    FTransform ScreenTransform(MonitorScreenRotation, MonitorScreenOffset, Scale);
    
    FTransform WorldScreenTransform = ScreenTransform * InstanceTransform;

    MonitorInteractionDebugProxy->SetWorldTransform(WorldScreenTransform);
    MonitorInteractionDebugProxy->SetVisibility(true);
}

void AProceduralOfficeGenerator::NotifyComputerLookedAt(const UPrimitiveComponent *Component, int32 InstanceIndex)
{
    const bool bComponentMatches = Component && Component == ComputerMeshComponent;
    const bool bValidInstance = InstanceIndex >= 0;
    HoveredComputerInstanceIndex = (bComponentMatches && bValidInstance) ? InstanceIndex : INDEX_NONE;
}

int32 AProceduralOfficeGenerator::FindClosestComputerInstance(const FVector &WorldPoint, float Radius) const
{
    const UInstancedStaticMeshComponent* ActiveComputerComponent = ComputerMeshComponent ? ComputerMeshComponent.Get() : nullptr;
    if (!ActiveComputerComponent || Radius <= 0.0f)
    {
        return INDEX_NONE;
    }

    const float RadiusSquared = Radius * Radius;
    int32 ClosestIndex = INDEX_NONE;
    float ClosestDistanceSquared = RadiusSquared;

    const int32 InstanceCount = ActiveComputerComponent->GetInstanceCount();
    for (int32 InstanceIdx = 0; InstanceIdx < InstanceCount; ++InstanceIdx)
    {
        FTransform InstanceTransform;
        if (!ActiveComputerComponent->GetInstanceTransform(InstanceIdx, InstanceTransform, true))
        {
            continue;
        }

        const FVector InstanceLocation = InstanceTransform.GetLocation();
        const float DistanceSquared = FVector::DistSquared(InstanceLocation, WorldPoint);
        if (DistanceSquared <= ClosestDistanceSquared)
        {
            ClosestDistanceSquared = DistanceSquared;
            ClosestIndex = InstanceIdx;
        }
    }

    return ClosestIndex;
}

UStaticMeshComponent* AProceduralOfficeGenerator::GetOrCreateComputerHighlightProxy(UInstancedStaticMeshComponent* SourceComponent)
{
    if (!IsValid(SourceComponent))
    {
        return nullptr;
    }

    if (!IsValid(ComputerHighlightProxy))
    {
        ComputerHighlightProxy = NewObject<UStaticMeshComponent>(this, TEXT("ComputerHighlightProxy"));
        if (!ComputerHighlightProxy)
        {
            return nullptr;
        }

        ComputerHighlightProxy->SetMobility(EComponentMobility::Movable);
        ComputerHighlightProxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ComputerHighlightProxy->SetCastShadow(false);
        ComputerHighlightProxy->bRenderInMainPass = false;
        ComputerHighlightProxy->SetRenderCustomDepth(false);
        ComputerHighlightProxy->SetCustomDepthStencilValue(0);
        ComputerHighlightProxy->SetupAttachment(Root);
        ComputerHighlightProxy->RegisterComponent();
        ComputerHighlightProxy->SetHiddenInGame(false);
        ComputerHighlightProxy->SetVisibility(false);
    }

    if (ComputerHighlightProxy->GetStaticMesh() != SourceComponent->GetStaticMesh())
    {
        ComputerHighlightProxy->SetStaticMesh(SourceComponent->GetStaticMesh());
    }

    const int32 MaterialCount = SourceComponent->GetNumMaterials();
    for (int32 MaterialIdx = 0; MaterialIdx < MaterialCount; ++MaterialIdx)
    {
        ComputerHighlightProxy->SetMaterial(MaterialIdx, SourceComponent->GetMaterial(MaterialIdx));
    }

    return ComputerHighlightProxy;
}

void AProceduralOfficeGenerator::HideComputerHighlight()
{
    if (IsValid(ComputerHighlightProxy))
    {
        ComputerHighlightProxy->SetRenderCustomDepth(false);
        ComputerHighlightProxy->SetCustomDepthStencilValue(0);
        ComputerHighlightProxy->SetVisibility(false);
    }
}

UInstancedStaticMeshComponent* AProceduralOfficeGenerator::ResolveComputerMeshComponent()
{
    if (IsValid(ComputerMeshComponent))
    {
        return ComputerMeshComponent;
    }

    if (TObjectPtr<UInstancedStaticMeshComponent>* Found = InstancedCache.Find(AProceduralOfficeGenerator::WorkstationMonitorComponentKey))
    {
        if (IsValid(Found->Get()))
        {
            ComputerMeshComponent = Found->Get();
            return ComputerMeshComponent;
        }
    }

    // Attempt to rebuild cache from existing instanced mesh components (PIE duplication case)
    UE_LOG(LogProceduralOffice, Warning, TEXT("ResolveComputerMeshComponent: Monitor cache invalid, scanning instanced components."));
    TArray<UInstancedStaticMeshComponent*> InstancedComponents;
    GetComponents<UInstancedStaticMeshComponent>(InstancedComponents);
    const FString MonitorNamePrefix = AProceduralOfficeGenerator::WorkstationMonitorComponentKey.ToString();
    
    // Priority 1: Search for component with specific tag
    UInstancedStaticMeshComponent** TagMatch = InstancedComponents.FindByPredicate([](UInstancedStaticMeshComponent* Component)
    {
        return IsValid(Component) && Component->ComponentTags.Contains(AProceduralOfficeGenerator::WorkstationMonitorTag);
    });

    if (TagMatch)
    {
        UE_LOG(LogProceduralOffice, Display, TEXT("ResolveComputerMeshComponent: Found tagged monitor component: %s"), *(*TagMatch)->GetName());
        ComputerMeshComponent = *TagMatch;
        InstancedCache.FindOrAdd(AProceduralOfficeGenerator::WorkstationMonitorComponentKey) = ComputerMeshComponent;
        return ComputerMeshComponent;
    }

    // Priority 2: Fallback to name matching
    UInstancedStaticMeshComponent** NameMatch = InstancedComponents.FindByPredicate([&MonitorNamePrefix](UInstancedStaticMeshComponent* Component)
    {
        if (!IsValid(Component))
        {
            return false;
        }

        const FString ComponentName = Component->GetName();
        // Match exact name or name with suffix like "_0", "_1", etc. but NOT "CubicleComputerTower"
        return ComponentName == MonitorNamePrefix || 
               (ComponentName.StartsWith(MonitorNamePrefix + TEXT("_")) && !ComponentName.Contains(TEXT("Tower")));
    });

    if (NameMatch)
    {
        UE_LOG(LogProceduralOffice, Display, TEXT("ResolveComputerMeshComponent: Found monitor by name match: %s"), *(*NameMatch)->GetName());
        ComputerMeshComponent = *NameMatch;
        InstancedCache.FindOrAdd(AProceduralOfficeGenerator::WorkstationMonitorComponentKey) = ComputerMeshComponent;
        return ComputerMeshComponent;
    }

    UE_LOG(LogProceduralOffice, Warning, TEXT("ResolveComputerMeshComponent: Monitor component scan failed."));
    return nullptr;
}

void AProceduralOfficeGenerator::RefreshWorkstationTargetPreview()
{
#if WITH_EDITOR
    for (UArrowComponent* Arrow : WorkstationTargetVisualizers)
    {
        if (Arrow)
        {
            Arrow->DestroyComponent();
        }
    }
    WorkstationTargetVisualizers.Empty();

    if (!bShowWorkstationTargetPreview)
    {
        return;
    }

    UInstancedStaticMeshComponent* ActiveComputerComponent = ComputerMeshComponent.Get();
    if (!IsValid(ActiveComputerComponent))
    {
        ActiveComputerComponent = ResolveComputerMeshComponent();
    }

    if (!IsValid(ActiveComputerComponent))
    {
        return;
    }

    const int32 InstanceCount = ActiveComputerComponent->GetInstanceCount();
    WorkstationTargetVisualizers.Reserve(InstanceCount);

    for (int32 InstanceIdx = 0; InstanceIdx < InstanceCount; ++InstanceIdx)
    {
        FTransform InstanceTransform;
        if (!ActiveComputerComponent->GetInstanceTransform(InstanceIdx, InstanceTransform, true))
        {
            continue;
        }

        const FTransform TargetTransform = WorkstationInteractionTargetOffset * InstanceTransform;

        UArrowComponent* Arrow = NewObject<UArrowComponent>(this);
        if (!Arrow)
        {
            continue;
        }

        Arrow->bIsEditorOnly = true;
        Arrow->SetMobility(EComponentMobility::Movable);
        Arrow->ArrowColor = FColor::Orange;
        Arrow->ArrowSize = 1.25f;
        Arrow->SetHiddenInGame(true);
        Arrow->SetVisibility(true);
        Arrow->SetWorldTransform(TargetTransform);
        Arrow->AttachToComponent(Root, FAttachmentTransformRules::KeepWorldTransform);
        Arrow->RegisterComponent();

        WorkstationTargetVisualizers.Add(Arrow);
    }
#endif
}
