#include "Procedural/ProceduralElevator.h"

#include "Procedural/ProceduralElevatorDoorController.h"
#include "Procedural/DoorInterpolationFunctions.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Math/UnrealMathUtility.h"
#include "TimerManager.h"

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

    BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
    BaseMesh->SetupAttachment(RootComponent);
    BaseMesh->SetMobility(EComponentMobility::Movable);

    // Create the visible door mesh component that user assigns the mesh to
    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    DoorMesh->SetupAttachment(RootComponent);
    DoorMesh->SetMobility(EComponentMobility::Movable);
    DoorMesh->SetVisibility(false); // Hidden, only used as template
    DoorMesh->SetHiddenInGame(true);
    DoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

    // Create button components
    Button0 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button0"));
    Button1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button1"));
    Button2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button2"));
    Button3 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button3"));
    Button4 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button4"));
    Button5 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button5"));
    Button6 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button6"));
    Button7 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button7"));
    Button8 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button8"));
    Button9 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button9"));
    ButtonAlarm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonAlarm"));
    ButtonCallDown = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonCallDown"));
    ButtonCallUp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonCallUp"));
    ButtonDoorClose = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonDoorClose"));
    ButtonDoorOpen = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonDoorOpen"));

    // Create indicator components
    IndicatorUp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IndicatorUp"));
    IndicatorDown = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IndicatorDown"));

    SetupButtonComponents();

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

    SyncDoorMeshes();
    RefreshDoorTransforms();
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

    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();
        if (PropertyName == GET_MEMBER_NAME_CHECKED(AProceduralElevator, DoorSlideSpeed))
        {
            if (DoorController)
            {
                DoorController->SetParameters(MakeDoorControllerParameters());
            }
        }
    }

    // Check if DoorMesh component's static mesh changed
    if (PropertyChangedEvent.MemberProperty)
    {
        const FName MemberPropertyName = PropertyChangedEvent.MemberProperty->GetFName();
        if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(AProceduralElevator, DoorMesh))
        {
            SyncDoorMeshes();
        }
    }

    RefreshDoorTransforms();
}
#endif

void AProceduralElevator::BeginPlay()
{
    Super::BeginPlay();

    if (DoorController)
    {
        DoorController->CloseDoors();
    }

    RequestOpenDoors(true);
}

void AProceduralElevator::OpenAllDoors()
{
#if WITH_EDITOR
    Modify();
#endif
    RequestOpenDoors(true);
}

void AProceduralElevator::CloseAllDoors()
{
#if WITH_EDITOR
    Modify();
#endif
    RequestCloseDoors(true);
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

    UStaticMeshComponent *DoorMeshComponent = GetDoorMesh(Slot);
    if (!DoorMeshComponent)
    {
        return;
    }

    const float DirectionSign = IsLeftDoor(Slot) ? -1.0f : 1.0f;
    const float ClosedX = DirectionSign * DoorClosedOffsetMagnitude;
    const float OpenX = ClosedX + DirectionSign * DoorSlideDistance;
    const float Fraction = FMath::Clamp(GetDoorFraction(Slot), 0.0f, 1.0f);

    FVector DesiredLocation = DoorMeshComponent->GetRelativeLocation();
    DesiredLocation.X = FMath::Lerp(ClosedX, OpenX, Fraction);

#if WITH_EDITOR
    if (UWorld *World = DoorMeshComponent->GetWorld(); World && World->WorldType == EWorldType::Editor)
    {
        DoorMeshComponent->Modify();
        DoorMeshComponent->MarkRenderTransformDirty();
    }
#endif

    DoorMeshComponent->SetRelativeLocation(DesiredLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AProceduralElevator::SetupButtonComponents()
{
    TArray<UStaticMeshComponent*> AllButtons = {
        Button0, Button1, Button2, Button3, Button4, Button5,
        Button6, Button7, Button8, Button9, ButtonAlarm,
        ButtonCallDown, ButtonCallUp, ButtonDoorClose, ButtonDoorOpen
    };

    for (UStaticMeshComponent* Button : AllButtons)
    {
        if (Button)
        {
            Button->SetupAttachment(RootComponent);
            Button->SetMobility(EComponentMobility::Movable);
            Button->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Button->SetCollisionResponseToAllChannels(ECR_Ignore);
            Button->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        }
    }

    TArray<UStaticMeshComponent*> AllIndicators = { IndicatorUp, IndicatorDown };
    for (UStaticMeshComponent* Indicator : AllIndicators)
    {
        if (Indicator)
        {
            Indicator->SetupAttachment(RootComponent);
            Indicator->SetMobility(EComponentMobility::Movable);
            Indicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
}

void AProceduralElevator::SyncDoorMeshes()
{
    if (!DoorMesh)
    {
        return;
    }

    UStaticMesh* Mesh = DoorMesh->GetStaticMesh();
    if (!Mesh)
    {
        return;
    }

    // Sync the mesh to all four door panels
    TArray<UStaticMeshComponent*> AllDoorMeshes = {
        FrontLeftDoorMesh, FrontRightDoorMesh, BackLeftDoorMesh, BackRightDoorMesh
    };

    for (UStaticMeshComponent* DoorMeshComponent : AllDoorMeshes)
    {
        if (DoorMeshComponent && DoorMeshComponent->GetStaticMesh() != Mesh)
        {
            DoorMeshComponent->SetStaticMesh(Mesh);
        }
    }
}

bool AProceduralElevator::IsInteractiveButton(UPrimitiveComponent* Component) const
{
    return Component == Button0 || Component == Button1 || Component == Button2 ||
           Component == Button3 || Component == Button4 || Component == Button5 ||
           Component == Button6 || Component == Button7 || Component == Button8 ||
           Component == Button9 || Component == ButtonAlarm || Component == ButtonCallDown ||
           Component == ButtonCallUp || Component == ButtonDoorClose || Component == ButtonDoorOpen;
}

bool AProceduralElevator::EvaluateInteractionFocus_Implementation(APawn* PlayerPawn, const FHitResult& Hit, float AssistRadius, UPrimitiveComponent*& OutHighlightComponent)
{
    OutHighlightComponent = nullptr;

    UPrimitiveComponent* ButtonComponent = nullptr;
    if (UPrimitiveComponent* HitComponent = Hit.GetComponent())
    {
        if (IsInteractiveButton(HitComponent))
        {
            ButtonComponent = HitComponent;
        }
    }

    if (!ButtonComponent && AssistRadius > 0.0f)
    {
        const FVector SearchOrigin = Hit.ImpactPoint.IsNearlyZero() ? Hit.Location : Hit.ImpactPoint;
        ButtonComponent = FindClosestButtonWithinRadius(SearchOrigin, AssistRadius);
    }

    if (ButtonComponent)
    {
        OutHighlightComponent = ButtonComponent;
        return true;
    }

    return false;
}

UPrimitiveComponent* AProceduralElevator::FindClosestButtonWithinRadius(const FVector& Point, float Radius) const
{
    if (Radius <= 0.0f)
    {
        return nullptr;
    }

    const float RadiusSquared = Radius * Radius;
    float ClosestDistanceSquared = RadiusSquared;
    UPrimitiveComponent* ClosestButton = nullptr;

    const TArray<UStaticMeshComponent*> AllButtons = {
        Button0, Button1, Button2, Button3, Button4, Button5,
        Button6, Button7, Button8, Button9, ButtonAlarm,
        ButtonCallDown, ButtonCallUp, ButtonDoorClose, ButtonDoorOpen
    };

    for (UStaticMeshComponent* Button : AllButtons)
    {
        if (!Button)
        {
            continue;
        }

        FVector ClosestPoint;
        const float DistanceToCollision = Button->GetClosestPointOnCollision(Point, ClosestPoint);
        const bool bHasClosestPoint = DistanceToCollision >= 0.0f;
        const float DistanceSquared = bHasClosestPoint
            ? FVector::DistSquared(Point, ClosestPoint)
            : FVector::DistSquared(Point, Button->GetComponentLocation());

        if (DistanceSquared <= ClosestDistanceSquared)
        {
            ClosestDistanceSquared = DistanceSquared;
            ClosestButton = Button;
        }
    }

    return ClosestButton;
}

bool AProceduralElevator::HandleButtonPressed(UPrimitiveComponent* ButtonComponent)
{
    if (!ButtonComponent)
    {
        return false;
    }

    if (ButtonComponent == ButtonDoorOpen)
    {
        RequestOpenDoors();
        return true;
    }

    if (ButtonComponent == ButtonDoorClose)
    {
        RequestCloseDoors(true);
        return true;
    }

    if (ButtonComponent == ButtonCallDown || ButtonComponent == ButtonCallUp)
    {
        UnlockDoorsAndOpen();
        return true;
    }

    if (ButtonComponent == ButtonAlarm)
    {
        return true;
    }

    if (IsFloorButtonComponent(ButtonComponent))
    {
        HandleFloorButtonPressed();
        return true;
    }

    return false;
}

bool AProceduralElevator::CanInteract_Implementation(APawn* PlayerPawn) const
{
    return DoorController != nullptr;
}

void AProceduralElevator::OnInteract_Implementation(APawn* PlayerPawn)
{
    if (!DoorController)
    {
        return;
    }

    ToggleDoors();
}

FText AProceduralElevator::GetInteractionPrompt_Implementation() const
{
    return InteractionPrompt;
}

void AProceduralElevator::ToggleDoors()
{
    if (!DoorController)
    {
        return;
    }

    const float CurrentFraction = DoorController->GetDoorFraction(EProceduralElevatorDoorSlot::FrontLeft);
    if (CurrentFraction < 0.5f)
    {
        RequestOpenDoors();
    }
    else
    {
        RequestCloseDoors(true);
    }
}

void AProceduralElevator::RequestOpenDoors(bool bForce)
{
    if (!DoorController)
    {
        return;
    }

    if (!bForce && bDoorsLocked)
    {
        return;
    }

    DoorController->OpenDoors();
}

void AProceduralElevator::RequestCloseDoors(bool bForce)
{
    if (!DoorController)
    {
        return;
    }

    if (!bForce && bDoorsLocked)
    {
        return;
    }

    DoorController->CloseDoors();
}

bool AProceduralElevator::IsFloorButtonComponent(const UPrimitiveComponent* Component) const
{
    return Component == Button0 || Component == Button1 || Component == Button2 || Component == Button3 ||
           Component == Button4 || Component == Button5 || Component == Button6 || Component == Button7 ||
           Component == Button8 || Component == Button9;
}

void AProceduralElevator::HandleFloorButtonPressed()
{
    CancelDoorUnlockTimer();
    bDoorsLocked = true;

    RequestCloseDoors(true);

    if (FloorSelectionDoorHoldTime <= KINDA_SMALL_NUMBER)
    {
        HandleDoorUnlockTimerElapsed();
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(DoorUnlockTimerHandle, this, &AProceduralElevator::HandleDoorUnlockTimerElapsed, FloorSelectionDoorHoldTime, false);
    }
}

void AProceduralElevator::HandleDoorUnlockTimerElapsed()
{
    CancelDoorUnlockTimer();
    bDoorsLocked = false;
    RequestOpenDoors(true);
}

void AProceduralElevator::CancelDoorUnlockTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DoorUnlockTimerHandle);
    }
}

void AProceduralElevator::UnlockDoorsAndOpen()
{
    CancelDoorUnlockTimer();
    bDoorsLocked = false;
    RequestOpenDoors(true);
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
