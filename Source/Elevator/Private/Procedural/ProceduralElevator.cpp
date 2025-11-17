#include "Procedural/ProceduralElevator.h"

#include "Procedural/ProceduralElevatorDoorController.h"
#include "Procedural/DoorInterpolationFunctions.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Math/UnrealMathUtility.h"

namespace
{
    constexpr int32 GDoorSlotCount = AProceduralElevator::DoorSlotCount;
    constexpr EProceduralElevatorDoorSlot GDoorSlots[GDoorSlotCount] = {
        EProceduralElevatorDoorSlot::FrontLeft,
        EProceduralElevatorDoorSlot::FrontRight,
        EProceduralElevatorDoorSlot::BackLeft,
        EProceduralElevatorDoorSlot::BackRight
    };

    constexpr int32 ToDoorIndex(EProceduralElevatorDoorSlot Slot)
    {
        return static_cast<int32>(Slot);
    }

    constexpr bool IsValidDoorIndex(int32 Index)
    {
        return Index >= 0 && Index < GDoorSlotCount;
    }

    constexpr bool IsLeftDoor(EProceduralElevatorDoorSlot Slot)
    {
        return Slot == EProceduralElevatorDoorSlot::FrontLeft || Slot == EProceduralElevatorDoorSlot::BackLeft;
    }
}

AProceduralElevator::AProceduralElevator()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
    Root->SetMobility(EComponentMobility::Movable);

    CabMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cab"));
    CabMesh->SetupAttachment(RootComponent);
    CabMesh->SetMobility(EComponentMobility::Movable);

    FrontDoorsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FrontDoorsRoot"));
    FrontDoorsRoot->SetupAttachment(RootComponent);
    FrontDoorsRoot->SetMobility(EComponentMobility::Movable);

    BackDoorsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BackDoorsRoot"));
    BackDoorsRoot->SetupAttachment(RootComponent);
    BackDoorsRoot->SetMobility(EComponentMobility::Movable);

    FrontLeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontLeftDoor"));
    FrontLeftDoorMesh->SetupAttachment(FrontDoorsRoot);
    FrontLeftDoorMesh->SetMobility(EComponentMobility::Movable);

    FrontRightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontRightDoor"));
    FrontRightDoorMesh->SetupAttachment(FrontDoorsRoot);
    FrontRightDoorMesh->SetMobility(EComponentMobility::Movable);

    BackLeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackLeftDoor"));
    BackLeftDoorMesh->SetupAttachment(BackDoorsRoot);
    BackLeftDoorMesh->SetMobility(EComponentMobility::Movable);

    BackRightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackRightDoor"));
    BackRightDoorMesh->SetupAttachment(BackDoorsRoot);
    BackRightDoorMesh->SetMobility(EComponentMobility::Movable);

    ButtonRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ButtonRoot"));
    ButtonRoot->SetupAttachment(RootComponent);
    ButtonRoot->SetMobility(EComponentMobility::Movable);

    ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
    ButtonMesh->SetupAttachment(ButtonRoot);
    ButtonMesh->SetMobility(EComponentMobility::Movable);
    ButtonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ButtonTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ButtonTrigger"));
    ButtonTrigger->SetupAttachment(ButtonRoot);
    ButtonTrigger->SetMobility(EComponentMobility::Movable);
    ButtonTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ButtonTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    ButtonTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    ButtonTrigger->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
    ButtonTrigger->SetGenerateOverlapEvents(true);
    ButtonTrigger->OnComponentBeginOverlap.AddDynamic(this, &AProceduralElevator::HandleButtonOverlap);

    FrontDoorRootOffset = FVector::ZeroVector;
    BackDoorRootOffset = FVector::ZeroVector;

    DoorController = MakeUnique<FProceduralElevatorDoorController>();
    DoorController->Initialize(this);
    DoorController->SetParameters(MakeDoorControllerParameters());
}

void AProceduralElevator::OnConstruction(const FTransform &Transform)
{
    Super::OnConstruction(Transform);
    if (DoorController)
    {
        DoorController->SetParameters(MakeDoorControllerParameters());
    }

    RefreshDoorTransforms();
    UpdateButtonConfiguration();
}

void AProceduralElevator::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (DoorController)
    {
        DoorController->Tick(DeltaSeconds);
    }
}

void AProceduralElevator::RefreshDoorTransforms()
{
    UpdateDoorRootOffsets();
    
    // Apply all door offsets
    ApplyDoorOffset(EProceduralElevatorDoorSlot::FrontLeft);
    ApplyDoorOffset(EProceduralElevatorDoorSlot::FrontRight);
    ApplyDoorOffset(EProceduralElevatorDoorSlot::BackLeft);
    ApplyDoorOffset(EProceduralElevatorDoorSlot::BackRight);
}

#if WITH_EDITOR
void AProceduralElevator::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (DoorController && PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();
        if (PropertyName == GET_MEMBER_NAME_CHECKED(AProceduralElevator, DoorSlideSpeed))
        {
            DoorController->SetParameters(MakeDoorControllerParameters());
        }
    }

    RefreshDoorTransforms();
    UpdateButtonConfiguration();
}
#endif

void AProceduralElevator::OpenAllDoors()
{
#if WITH_EDITOR
    Modify();
#endif
    if (DoorController)
    {
        DoorController->OpenDoors();
    }
}

void AProceduralElevator::CloseAllDoors()
{
#if WITH_EDITOR
    Modify();
#endif
    if (DoorController)
    {
        DoorController->CloseDoors();
    }
}

void AProceduralElevator::SetDoorFraction(EProceduralElevatorDoorSlot Slot, float Fraction)
{
    if (!DoorController)
    {
        return;
    }

    if (!IsValidDoorIndex(ToDoorIndex(Slot)))
    {
        return;
    }

    DoorController->SetDoorFractionImmediate(Slot, FMath::Clamp(Fraction, 0.0f, 1.0f));
}

void AProceduralElevator::SetDoorPairFraction(bool bFront, float Fraction)
{
    const EProceduralElevatorDoorSlot PairSlots[2] = {
        bFront ? EProceduralElevatorDoorSlot::FrontLeft : EProceduralElevatorDoorSlot::BackLeft,
        bFront ? EProceduralElevatorDoorSlot::FrontRight : EProceduralElevatorDoorSlot::BackRight
    };

    for (EProceduralElevatorDoorSlot Slot : PairSlots)
    {
        SetDoorFraction(Slot, Fraction);
    }
}

void AProceduralElevator::SetAllDoorFractions(float Fraction)
{
    const float Clamped = FMath::Clamp(Fraction, 0.0f, 1.0f);
    SetDoorFraction(EProceduralElevatorDoorSlot::FrontLeft, Clamped);
    SetDoorFraction(EProceduralElevatorDoorSlot::FrontRight, Clamped);
    SetDoorFraction(EProceduralElevatorDoorSlot::BackLeft, Clamped);
    SetDoorFraction(EProceduralElevatorDoorSlot::BackRight, Clamped);
}

float AProceduralElevator::GetDoorFraction(EProceduralElevatorDoorSlot Slot) const
{
    return DoorController ? DoorController->GetDoorFraction(Slot) : 0.0f;
}

void AProceduralElevator::ApplyDoorOffset(EProceduralElevatorDoorSlot Slot)
{
    const int32 Index = ToDoorIndex(Slot);
    if (!IsValidDoorIndex(Index))
    {
        return;
    }

    UStaticMeshComponent *DoorMesh = GetDoorMesh(Slot);
    if (!DoorMesh)
    {
        return;
    }

    const float DirectionSign = IsLeftDoor(Slot) ? -1.0f : 1.0f;
    const float ClosedX = DirectionSign * DoorClosedOffsetMagnitude;
    const float OpenX = ClosedX + DirectionSign * DoorSlideDistance;
    const float Fraction = FMath::Clamp(GetDoorFraction(Slot), 0.0f, 1.0f);

    FVector DesiredLocation = DoorMesh->GetRelativeLocation();
    DesiredLocation.X = FMath::Lerp(ClosedX, OpenX, Fraction);

#if WITH_EDITOR
    if (UWorld *World = DoorMesh->GetWorld(); World && World->WorldType == EWorldType::Editor)
    {
        DoorMesh->Modify();
        DoorMesh->MarkRenderTransformDirty();
    }
#endif

    DoorMesh->SetRelativeLocation(DesiredLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AProceduralElevator::UpdateButtonConfiguration()
{
    if (!ButtonRoot || !ButtonTrigger || !ButtonMesh)
    {
        return;
    }

    ButtonRoot->SetRelativeLocation(ButtonRelativeLocation);
    ButtonRoot->SetRelativeRotation(ButtonRelativeRotation);

    const bool bShouldEnable = bEnableDoorButton;

    ButtonRoot->SetVisibility(bShouldEnable, true);
    ButtonRoot->SetHiddenInGame(!bShouldEnable, true);
    ButtonMesh->SetVisibility(bShouldEnable, true);
    ButtonMesh->SetHiddenInGame(!bShouldEnable);
    if (bShouldEnable)
    {
        const FVector ClampedExtents(
            FMath::Max(ButtonTriggerExtents.X, 1.0f),
            FMath::Max(ButtonTriggerExtents.Y, 1.0f),
            FMath::Max(ButtonTriggerExtents.Z, 1.0f));
        ButtonTrigger->SetBoxExtent(ClampedExtents);
        ButtonTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        ButtonTrigger->SetGenerateOverlapEvents(true);
    }
    else
    {
        ButtonTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ButtonTrigger->SetGenerateOverlapEvents(false);
    }
}

void AProceduralElevator::HandleButtonOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
    if (!bEnableDoorButton || !OtherActor || OtherActor == this)
    {
        return;
    }

    if (!OtherActor->IsA(APawn::StaticClass()))
    {
        return;
    }

    if (!DoorController)
    {
        return;
    }

    const float CurrentFraction = DoorController->GetDoorFraction(EProceduralElevatorDoorSlot::FrontLeft);
    if (CurrentFraction < 0.5f)
    {
        DoorController->OpenDoors();
    }
    else
    {
        DoorController->CloseDoors();
    }
}

UStaticMeshComponent *AProceduralElevator::GetDoorMesh(EProceduralElevatorDoorSlot Slot) const
{
    UStaticMeshComponent *const DoorMeshArray[GDoorSlotCount] = {
        FrontLeftDoorMesh,
        FrontRightDoorMesh,
        BackLeftDoorMesh,
        BackRightDoorMesh
    };

    const int32 Index = ToDoorIndex(Slot);
    return IsValidDoorIndex(Index) ? DoorMeshArray[Index] : nullptr;
}

void AProceduralElevator::UpdateDoorRootOffsets()
{
    if (FrontDoorsRoot)
    {
        FrontDoorsRoot->SetRelativeLocation(FrontDoorRootOffset);
    }

    if (BackDoorsRoot)
    {
        const float DepthOffset = (BackDoorPanelThickness > KINDA_SMALL_NUMBER)
                                      ? BackDoorPanelThickness + BackDoorDepthPadding
                                      : 0.0f;
        const float DepthSign = bBackDoorsOffsetNegativeDepth ? -1.0f : 1.0f;

        FVector BaseOffset = bInheritFrontRootOffsetForBackPair ? FrontDoorRootOffset : FVector::ZeroVector;
        BaseOffset += FVector(0.0f, DepthSign * DepthOffset, 0.0f);
        BaseOffset += BackDoorRootOffset;

        BackDoorsRoot->SetRelativeLocation(BaseOffset);
    }
}

// Hardcode the selected interpolation function for now
FProceduralElevatorDoorControllerParameters AProceduralElevator::MakeDoorControllerParameters() const
{
    FProceduralElevatorDoorControllerParameters Params;
    Params.SlideSpeed = DoorSlideSpeed;
    Params.InterpolationFunction = DoorInterpolation::EaseInOut;
    return Params;
}
