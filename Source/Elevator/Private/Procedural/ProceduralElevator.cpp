#include "Procedural/ProceduralElevator.h"

#include "Procedural/ProceduralElevatorDoorController.h"
#include "Procedural/DoorInterpolationFunctions.h"
#include "System/ElevatorGameManagerSubsystem.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "Sound/SoundBase.h"
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
    LastHighlightedButton = nullptr;

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
        const FElevatorButtonInteractionState ButtonState = GetButtonInteractionState(ButtonComponent);
        if (!ButtonState.bIsInteractable)
        {
            ButtonComponent = nullptr;
        }
    }

    if (ButtonComponent)
    {
        OutHighlightComponent = ButtonComponent;
        LastHighlightedButton = ButtonComponent;
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

        const FElevatorButtonInteractionState ButtonState = GetButtonInteractionState(Button);
        if (!ButtonState.bIsInteractable)
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

    // Play button click sound
    if (USoundBase* ButtonSound = AudioRegistry.ElevatorButtonClick)
    {
        if (UWorld* World = GetWorld())
        {
            UGameplayStatics::SpawnSoundAtLocation(World, ButtonSound, ButtonComponent->GetComponentLocation(), FRotator::ZeroRotator, 0.8f);
        }
    }

    const FName ButtonId = GetButtonId(ButtonComponent);

    const FElevatorButtonInteractionState ButtonState = GetButtonInteractionState(ButtonComponent);
    if (!ButtonState.bIsInteractable)
    {
        switch (ButtonState.LockReason)
        {
        case AProceduralElevator::EElevatorButtonLockReason::RequiresTaskCompletion:
            if (const UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
            {
                UE_LOG(LogTemp, Log, TEXT("[Elevator] Ignoring press on %s until current task completes (Day=%d TaskComplete=%s)."),
                    *ButtonId.ToString(),
                    Manager->GetCurrentDay(),
                    Manager->IsTaskComplete() ? TEXT("true") : TEXT("false"));
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("[Elevator] Ignoring press on %s until current task completes."), *ButtonId.ToString());
            }
            break;
        case AProceduralElevator::EElevatorButtonLockReason::ManagerDisabled:
            UE_LOG(LogTemp, Log, TEXT("[Elevator] Ignoring press on %s because it is disabled by scenario rules."), *ButtonId.ToString());
            break;
        case AProceduralElevator::EElevatorButtonLockReason::InvalidComponent:
            UE_LOG(LogTemp, Warning, TEXT("[Elevator] Ignoring press on invalid elevator button component."));
            break;
        default:
            UE_LOG(LogTemp, Log, TEXT("[Elevator] Ignoring press on %s because it is disabled."), *ButtonId.ToString());
            break;
        }

        return ButtonState.bConsumesPress;
    }

    if (ButtonComponent == ButtonDoorOpen)
    {
        // Check if we can open doors before triggering motion
        if (DoorController)
        {
            if (bDoorsLocked)
            {
                UE_LOG(LogTemp, Log, TEXT("[Elevator] Door open button ignored - doors locked."));
                return true; // Consume input while locked
            }
            if (DoorController->AreDoorsFullyOpen())
            {
                UE_LOG(LogTemp, Log, TEXT("[Elevator] Door open button ignored - doors already fully open."));
                return true; // Consume input so we do not toggle via fallback interact
            }
            if (DoorController->IsAnimating())
            {
                UE_LOG(LogTemp, Log, TEXT("[Elevator] Door open button ignored - doors still animating."));
                return true; // Consume input so we do not toggle via fallback interact
            }
        }

        RequestOpenDoors();
        if (UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
        {
            Manager->HandleElevatorButtonPressed(ButtonId);
        }
        return true;
    }

    if (ButtonComponent == ButtonDoorClose)
    {
        // Check if we can close doors before triggering motion
        if (DoorController)
        {
            if (bDoorsLocked)
            {
                UE_LOG(LogTemp, Log, TEXT("[Elevator] Door close button ignored - doors locked."));
                return true; // Consume input while locked
            }
            if (DoorController->AreDoorsFullyClosed())
            {
                UE_LOG(LogTemp, Log, TEXT("[Elevator] Door close button ignored - doors already fully closed."));
                return true; // Consume input so we do not toggle via fallback interact
            }
            if (DoorController->IsAnimating())
            {
                UE_LOG(LogTemp, Log, TEXT("[Elevator] Door close button ignored - doors still animating."));
                return true; // Consume input so we do not toggle via fallback interact
            }
        }

        RequestCloseDoors(false);
        if (UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
        {
            Manager->HandleElevatorButtonPressed(ButtonId);
        }
        return true;
    }

    if (ButtonComponent == ButtonCallDown || ButtonComponent == ButtonCallUp)
    {
        UnlockDoorsAndOpen();
        if (UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
        {
            Manager->HandleElevatorButtonPressed(ButtonId);
        }
        return true;
    }

    if (ButtonComponent == ButtonAlarm)
    {
        if (UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
        {
            Manager->HandleElevatorButtonPressed(ButtonId);
        }
        return true;
    }

    if (IsFloorButtonComponent(ButtonComponent))
    {
        UE_LOG(LogTemp, Log, TEXT("[Elevator] Floor button %s pressed."), *ButtonId.ToString());
        return HandleFloorButtonPressed(ButtonId);
    }

    return false;
}

bool AProceduralElevator::CanInteract_Implementation(APawn* PlayerPawn) const
{
    (void)PlayerPawn;

    if (!DoorController)
    {
        return false;
    }

    UPrimitiveComponent* const HighlightedButton = LastHighlightedButton.Get();
    if (!IsValid(HighlightedButton))
    {
        return false;
    }

    return GetButtonInteractionState(HighlightedButton).bIsInteractable;
}

void AProceduralElevator::OnInteract_Implementation(APawn* PlayerPawn)
{
    (void)PlayerPawn;
    // Elevator interaction is routed exclusively through button components.
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

    // Don't open if already fully open or animating
    if (DoorController->AreDoorsFullyOpen())
    {
        UE_LOG(LogTemp, Log, TEXT("[Elevator] Doors are already fully open."));
        return;
    }

    if (DoorController->IsAnimating())
    {
        UE_LOG(LogTemp, Log, TEXT("[Elevator] Doors are still animating."));
        return;
    }

    // Play door open sound
    if (USoundBase* DoorOpenSound = AudioRegistry.ElevatorDoorOpen)
    {
        if (UWorld* World = GetWorld())
        {
            UGameplayStatics::SpawnSoundAtLocation(World, DoorOpenSound, GetActorLocation(), FRotator::ZeroRotator, 1.0f);
        }
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

    // Don't close if already fully closed or animating (unless forced)
    if (!bForce)
    {
        if (DoorController->AreDoorsFullyClosed())
        {
            UE_LOG(LogTemp, Log, TEXT("[Elevator] Doors are already fully closed."));
            return;
        }

        if (DoorController->IsAnimating())
        {
            UE_LOG(LogTemp, Log, TEXT("[Elevator] Doors are still animating."));
            return;
        }
    }

    // Play door close sound
    if (USoundBase* DoorCloseSound = AudioRegistry.ElevatorDoorClose)
    {
        if (UWorld* World = GetWorld())
        {
            UGameplayStatics::SpawnSoundAtLocation(World, DoorCloseSound, GetActorLocation(), FRotator::ZeroRotator, 1.0f);
        }
    }

    DoorController->CloseDoors();
}

bool AProceduralElevator::IsFloorButtonComponent(const UPrimitiveComponent* Component) const
{
    return Component == Button0 || Component == Button1 || Component == Button2 || Component == Button3 ||
           Component == Button4 || Component == Button5 || Component == Button6 || Component == Button7 ||
           Component == Button8 || Component == Button9;
}

bool AProceduralElevator::HandleFloorButtonPressed(FName ButtonId)
{
    CancelDoorUnlockTimer();
    bDoorsLocked = true;

    RequestCloseDoors(true);

    const float DoorCloseDuration = DoorController ? DoorController->GetMaxRemainingDuration() : 0.0f;
    const float RideStartDelay = DoorCloseDuration;

    UE_LOG(LogTemp, Log, TEXT("[Elevator] Handling floor selection %s. Doors locked until timer expires."), *ButtonId.ToString());

    // Play elevator ride sound after doors close (non-looping, plays once)
    if (USoundBase* RideSound = AudioRegistry.ElevatorRide)
    {
        if (UWorld* World = GetWorld())
        {
            FTimerHandle RideSoundStartHandle;
            World->GetTimerManager().SetTimer(RideSoundStartHandle, [this, RideSound, World]()
            {
                UGameplayStatics::SpawnSoundAtLocation(World, RideSound, GetActorLocation(), FRotator::ZeroRotator, 0.7f);
            }, RideStartDelay, false);
        }
    }

    if (UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
    {
        Manager->HandleElevatorButtonPressed(ButtonId);
    }

    const float UnlockDelay = DoorCloseDuration + ElevatorRideDuration;

    if (UnlockDelay <= KINDA_SMALL_NUMBER)
    {
        HandleDoorUnlockTimerElapsed();
        return true;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(DoorUnlockTimerHandle, this, &AProceduralElevator::HandleDoorUnlockTimerElapsed, UnlockDelay, false);
    }

    return true;
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

FName AProceduralElevator::GetButtonId(const UPrimitiveComponent* Component) const
{
    if (!Component)
    {
        return NAME_None;
    }

    if (Component == Button0) { static const FName Floor0Id(TEXT("Floor0")); return Floor0Id; }
    if (Component == Button1) { static const FName Floor1Id(TEXT("Floor1")); return Floor1Id; }
    if (Component == Button2) { static const FName Floor2Id(TEXT("Floor2")); return Floor2Id; }
    if (Component == Button3) { static const FName Floor3Id(TEXT("Floor3")); return Floor3Id; }
    if (Component == Button4) { static const FName Floor4Id(TEXT("Floor4")); return Floor4Id; }
    if (Component == Button5) { static const FName Floor5Id(TEXT("Floor5")); return Floor5Id; }
    if (Component == Button6) { static const FName Floor6Id(TEXT("Floor6")); return Floor6Id; }
    if (Component == Button7) { static const FName Floor7Id(TEXT("Floor7")); return Floor7Id; }
    if (Component == Button8) { static const FName Floor8Id(TEXT("Floor8")); return Floor8Id; }
    if (Component == Button9) { static const FName Floor9Id(TEXT("Floor9")); return Floor9Id; }
    if (Component == ButtonAlarm) { static const FName AlarmId(TEXT("Alarm")); return AlarmId; }
    if (Component == ButtonCallDown) { static const FName CallDownId(TEXT("CallDown")); return CallDownId; }
    if (Component == ButtonCallUp) { static const FName CallUpId(TEXT("CallUp")); return CallUpId; }
    if (Component == ButtonDoorClose) { static const FName DoorCloseId(TEXT("DoorClose")); return DoorCloseId; }
    if (Component == ButtonDoorOpen) { static const FName DoorOpenId(TEXT("DoorOpen")); return DoorOpenId; }

    return Component->GetFName();
}

AProceduralElevator::FElevatorButtonInteractionState AProceduralElevator::GetButtonInteractionState(const UPrimitiveComponent* Component) const
{
    FElevatorButtonInteractionState State;

    if (!Component)
    {
        State.bIsInteractable = false;
        State.bConsumesPress = false;
        State.LockReason = AProceduralElevator::EElevatorButtonLockReason::InvalidComponent;
        return State;
    }

    if (!IsInteractiveButton(const_cast<UPrimitiveComponent*>(Component)))
    {
        State.bIsInteractable = false;
        State.bConsumesPress = false;
        State.LockReason = AProceduralElevator::EElevatorButtonLockReason::InvalidComponent;
        return State;
    }

    const FName ButtonId = GetButtonId(Component);
    if (ButtonId.IsNone())
    {
        return State;
    }

    if (const UElevatorGameManagerSubsystem* Manager = UElevatorGameManagerSubsystem::Get(this))
    {
        const FString ButtonIdString = ButtonId.ToString();
        if (ButtonIdString.StartsWith(TEXT("Floor")))
        {
            const bool bTaskComplete = Manager->IsTaskComplete();
            State.bIsInteractable = bTaskComplete;
            State.bConsumesPress = true;
            if (!bTaskComplete)
            {
                State.LockReason = AProceduralElevator::EElevatorButtonLockReason::RequiresTaskCompletion;
            }
            return State;
        }

        const bool bEnabled = Manager->IsElevatorButtonEnabled(ButtonId);
        State.bIsInteractable = bEnabled;
        State.bConsumesPress = true;
        if (!bEnabled)
        {
            State.LockReason = AProceduralElevator::EElevatorButtonLockReason::ManagerDisabled;
        }
        return State;
    }

    return State;
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
