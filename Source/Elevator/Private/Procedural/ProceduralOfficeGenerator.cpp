#include "Procedural/ProceduralOfficeGenerator.h"
#include "Procedural/ProceduralOfficeGenerator.Helpers.h"
#include "Procedural/ProceduralOfficeGenerator.Log.h"
#include "Procedural/ProceduralElevator.h"
#include "FirstPersonCharacter.h"

#include "Components/ChildActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/PlayerStart.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Images/SImage.h"
#include "Styling/SlateBrush.h"

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

    // Render Slate UI to the render target
    if (ScreenRenderTarget)
    {
        RenderSlateToRenderTarget();
        UpdateScreenMaterialParameters();
        UE_LOG(LogProceduralOffice, Display, TEXT("Rendered Slate UI to render target RT_ScreenInterface"));
    }
    else
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("ScreenRenderTarget is not set. Please assign RT_ScreenInterface in the details panel."));
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
            Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ElevatorDefaultHeight))
        {
            GenerateFromData();
        }
        else if (Name == GET_MEMBER_NAME_CHECKED(AProceduralOfficeGenerator, ScreenMaterialBase))
        {
            UpdateScreenMaterialParameters();
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

    ComputerMeshComponent = nullptr;
    PendingWorkstationViewTransform = FTransform::Identity;
    bHasPendingWorkstationViewTransform = false;
    HoveredComputerInstanceIndex = INDEX_NONE;

    if (IsValid(ComputerHighlightProxy))
    {
        ComputerHighlightProxy->DestroyComponent();
        ComputerHighlightProxy = nullptr;
    }

#if WITH_EDITOR
    for (UArrowComponent* Arrow : WorkstationTargetVisualizers)
    {
        if (Arrow)
        {
            Arrow->DestroyComponent();
        }
    }
    WorkstationTargetVisualizers.Empty();
#endif

    for (UChildActorComponent *ChildComponent : SpawnedChildActors)
    {
        if (!ChildComponent)
        {
            continue;
        }

        ChildComponent->DestroyChildActor();

        if (ChildComponent->IsRegistered())
        {
            ChildComponent->UnregisterComponent();
        }

        ChildComponent->DestroyComponent();
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
        FirstPersonCharacter->BeginWorkstationInteraction(PendingWorkstationViewTransform, WorkstationInteractionMoveDuration, WorkstationInteractionArcHeight, WorkstationInteractionCurveBias);
    }

    UE_LOG(LogProceduralOffice, Display, TEXT("Workstation monitor interaction triggered on instance %d."), HoveredComputerInstanceIndex);

    HoveredComputerInstanceIndex = INDEX_NONE;
    bHasPendingWorkstationViewTransform = false;
    HideComputerHighlight();
}

FText AProceduralOfficeGenerator::GetInteractionPrompt_Implementation() const
{
    return HoveredComputerInstanceIndex != INDEX_NONE ? WorkstationInteractionPrompt : FText::GetEmpty();
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

TSharedPtr<SWidget> AProceduralOfficeGenerator::CreateSlateWidget(FVector2D Size)
{
    const float BorderThickness = 3.0f;
    const FLinearColor BorderColor = FLinearColor::White;
    const float EdgePadding = 2.0f; // Padding to ensure edge pixels are black for clamping
    
    // Vertical line with circle dimensions
    const float LineHeight = Size.Y * 0.5f;
    const float LineThickness = 4.0f;
    const float CircleRadius = Size.X * 0.08f; // 8% of width
    const FVector2D LineCenter = FVector2D(Size.X * 0.5f, Size.Y * 0.5f);
    const float LineBottomY = LineCenter.Y + LineHeight * 0.5f;
    const float LineTopY = LineCenter.Y - LineHeight * 0.5f;
    const float CircleCenterY = LineTopY;
    const int32 CircleSegments = 32; // Number of segments to approximate the circle
    
    // Create the widget - fills entire render target
    TSharedRef<SCanvas> ContentCanvas = SNew(SCanvas)
        // Top border
        + SCanvas::Slot()
        .Position(FVector2D(EdgePadding, EdgePadding))
        .Size(FVector2D(Size.X - EdgePadding * 2.0f, BorderThickness))
        [
            SNew(SImage)
            .ColorAndOpacity(BorderColor)
        ]
        // Bottom border
        + SCanvas::Slot()
        .Position(FVector2D(EdgePadding, Size.Y - BorderThickness - EdgePadding))
        .Size(FVector2D(Size.X - EdgePadding * 2.0f, BorderThickness))
        [
            SNew(SImage)
            .ColorAndOpacity(BorderColor)
        ]
        // Left border
        + SCanvas::Slot()
        .Position(FVector2D(EdgePadding, EdgePadding))
        .Size(FVector2D(BorderThickness, Size.Y - EdgePadding * 2.0f))
        [
            SNew(SImage)
            .ColorAndOpacity(BorderColor)
        ]
        // Right border
        + SCanvas::Slot()
        .Position(FVector2D(Size.X - BorderThickness - EdgePadding, EdgePadding))
        .Size(FVector2D(BorderThickness, Size.Y - EdgePadding * 2.0f))
        [
            SNew(SImage)
            .ColorAndOpacity(BorderColor)
        ]
        // Vertical line
        + SCanvas::Slot()
        .Position(FVector2D(LineCenter.X - LineThickness * 0.5f, LineTopY))
        .Size(FVector2D(LineThickness, LineHeight))
        [
            SNew(SImage)
            .ColorAndOpacity(BorderColor)
        ];
    
    // Add circle segments
    for (int32 i = 0; i < CircleSegments; ++i)
    {
        const float Angle1 = (float)i / CircleSegments * 2.0f * PI;
        const float Angle2 = (float)(i + 1) / CircleSegments * 2.0f * PI;
        
        const FVector2D Point1 = FVector2D(
            LineCenter.X + CircleRadius * FMath::Cos(Angle1),
            CircleCenterY + CircleRadius * FMath::Sin(Angle1)
        );
        const FVector2D Point2 = FVector2D(
            LineCenter.X + CircleRadius * FMath::Cos(Angle2),
            CircleCenterY + CircleRadius * FMath::Sin(Angle2)
        );
        
        const FVector2D SegmentCenter = (Point1 + Point2) * 0.5f;
        const float SegmentLength = FVector2D::Distance(Point1, Point2);
        const float SegmentAngle = FMath::Atan2(Point2.Y - Point1.Y, Point2.X - Point1.X);
        
        ContentCanvas->AddSlot()
        .Position(SegmentCenter - FVector2D(SegmentLength * 0.5f, BorderThickness * 0.5f))
        .Size(FVector2D(SegmentLength, BorderThickness))
        [
            SNew(SImage)
            .ColorAndOpacity(BorderColor)
            .RenderTransform(FSlateRenderTransform(FQuat2D(SegmentAngle)))
            .RenderTransformPivot(FVector2D(0.5f, 0.5f))
        ];
    }
    
    return ContentCanvas;
}

void AProceduralOfficeGenerator::RenderSlateToRenderTarget()
{
    if (!ScreenRenderTarget)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Cannot render Slate: ScreenRenderTarget is null"));
        return;
    }

    // Ensure texture clamping so edges don't streak if we sample outside 0-1
    ScreenRenderTarget->AddressX = TA_Clamp;
    ScreenRenderTarget->AddressY = TA_Clamp;

    // Get render target dimensions
    const int32 Width = ScreenRenderTarget->SizeX;
    const int32 Height = ScreenRenderTarget->SizeY;

    if (Width <= 0 || Height <= 0)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("Invalid render target dimensions: %dx%d"), Width, Height);
        return;
    }

    // Create the Slate widget
    TSharedPtr<SWidget> Widget = CreateSlateWidget(FVector2D(Width, Height));
    if (!Widget.IsValid())
    {
        UE_LOG(LogProceduralOffice, Error, TEXT("Failed to create Slate widget"));
        return;
    }

    // Create widget renderer
    FWidgetRenderer* WidgetRenderer = new FWidgetRenderer(true, false);
    if (!WidgetRenderer)
    {
        UE_LOG(LogProceduralOffice, Error, TEXT("Failed to create FWidgetRenderer"));
        return;
    }

    // Clear the render target first
    UKismetRenderingLibrary::ClearRenderTarget2D(this, ScreenRenderTarget, FLinearColor::Black);

    // Render the widget to the render target
    WidgetRenderer->DrawWidget(
        ScreenRenderTarget,
        Widget.ToSharedRef(),
        FVector2D(Width, Height),
        0.0f,
        false
    );

    // Clean up
    delete WidgetRenderer;

    UE_LOG(LogProceduralOffice, Display, TEXT("Successfully rendered Slate widget to render target (%dx%d)"), Width, Height);
}

void AProceduralOfficeGenerator::UpdateScreenMaterialParameters()
{
    if (!ScreenMaterialBase)
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("ScreenMaterialBase is not set. Cannot update material parameters."));
        return;
    }

    UInstancedStaticMeshComponent* ActiveComputerComponent = ComputerMeshComponent.Get();
    if (!IsValid(ActiveComputerComponent))
    {
        ActiveComputerComponent = ResolveComputerMeshComponent();
    }

    if (!IsValid(ActiveComputerComponent))
    {
        UE_LOG(LogProceduralOffice, Warning, TEXT("No computer mesh component found. Cannot update material parameters."));
        return;
    }

    // Create a dynamic material instance if needed
    UMaterialInterface* CurrentMaterial = ActiveComputerComponent->GetMaterial(0);
    UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(CurrentMaterial);
    
    if (!DynamicMaterial || DynamicMaterial->Parent != ScreenMaterialBase)
    {
        DynamicMaterial = UMaterialInstanceDynamic::Create(ScreenMaterialBase, this);
        if (!DynamicMaterial)
        {
            UE_LOG(LogProceduralOffice, Error, TEXT("Failed to create dynamic material instance"));
            return;
        }
        
        ActiveComputerComponent->SetMaterial(0, DynamicMaterial);
        UE_LOG(LogProceduralOffice, Display, TEXT("Created and assigned dynamic material instance to monitor mesh"));
    }
}
