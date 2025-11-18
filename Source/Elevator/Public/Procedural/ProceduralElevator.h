#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/UniquePtr.h"
#include "Interactable.h"
#include "Procedural/ProceduralElevatorDoorController.h"
#include "TimerManager.h"
#include "ProceduralElevator.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UPrimitiveComponent;
class FProceduralElevatorDoorController;

/** The individual sliding door panels we support. */
UENUM(BlueprintType)
enum class EProceduralElevatorDoorSlot : uint8
{
    FrontLeft,
    FrontRight,
    BackLeft,
    BackRight
};

/**
 * Minimal actor that owns moving elevator pieces so each elevator can be controlled independently.
 */
UCLASS()
class ELEVATOR_API AProceduralElevator : public AActor, public IInteractable
{
    GENERATED_BODY()

public:
    static constexpr int32 DoorSlotCount = 4;

    AProceduralElevator();

    virtual void OnConstruction(const FTransform &Transform) override;
    virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Elevator")
    void OpenAllDoors();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Elevator")
    void CloseAllDoors();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Elevator")
    void SetDoorFraction(EProceduralElevatorDoorSlot Slot, float Fraction);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Elevator")
    void SetDoorPairFraction(bool bFront, float Fraction);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Elevator")
    void SetAllDoorFractions(float Fraction);

    UFUNCTION(BlueprintPure, Category = "Elevator")
    float GetDoorFraction(EProceduralElevatorDoorSlot Slot) const;

    /** Helper to check if a component is one of our interactive buttons */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool IsInteractiveButton(UPrimitiveComponent* Component) const;

    /** Finds the closest interactive button to a point if it falls within the provided radius. */
    UFUNCTION(BlueprintPure, Category = "Interaction")
    UPrimitiveComponent* FindClosestButtonWithinRadius(const FVector& Point, float Radius) const;

    bool HandleButtonPressed(UPrimitiveComponent* ButtonComponent);

    // IInteractable interface
    virtual bool CanInteract_Implementation(APawn* PlayerPawn) const override;
    virtual void OnInteract_Implementation(APawn* PlayerPawn) override;
    virtual FText GetInteractionPrompt_Implementation() const override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent *Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent *FrontDoorsRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent *BackDoorsRoot;

    /** Root component for the elevator base mesh */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Base")
    UStaticMeshComponent *BaseMesh;

    /** Door mesh component - assign static mesh here, used for all four doors */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Doors")
    UStaticMeshComponent *DoorMesh;

    /** Individual button meshes - assign in blueprint */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *Button0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *Button1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *Button2;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *Button3;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *Button4;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *Button5;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *Button6;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *Button7;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *Button8;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *Button9;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *ButtonAlarm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *ButtonCallDown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *ButtonCallUp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *ButtonDoorClose;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Buttons")
    UStaticMeshComponent *ButtonDoorOpen;

    /** Indicator meshes - not used for interaction */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Indicators")
    UStaticMeshComponent *IndicatorUp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Indicators")
    UStaticMeshComponent *IndicatorDown;

    /** Allows moving the front door pair as a unit without touching every mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Front", meta = (ClampMin = "-1000.0", ClampMax = "1000.0"))
    FVector FrontDoorRootOffset;

    /** Allows moving the back door pair as a unit without touching every mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Back", meta = (ClampMin = "-1000.0", ClampMax = "1000.0"))
    FVector BackDoorRootOffset;

    /** Magnitude of the X offset from the cab center when the door panels are fully closed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Layout", meta = (ClampMin = "0.0"))
    float DoorClosedOffsetMagnitude = 51.5f;

    /** How far each panel travels along X when fully open (positive values slide away from the centre). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Layout", meta = (ClampMin = "0.0"))
    float DoorSlideDistance = 300.0f;

    /** Fraction-per-second rate used when animating doors toward a requested value. Set to 0 for instant movement. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Layout", meta = (ClampMin = "0.0"))
    float DoorSlideSpeed = 1.5f;

    /** Additional spacing applied between the front and back door stacks beyond the door thickness. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Back", meta = (ClampMin = "0.0"))
    float BackDoorDepthPadding = 0.0f;

    /** Thickness of the door panels, used to offset the back door stack. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Back", meta = (ClampMin = "0.0"))
    float BackDoorPanelThickness = 0.0f;

    /** Determines whether the back doors are offset along negative Y (true) or positive Y (false). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Back")
    bool bBackDoorsOffsetNegativeDepth = true;

    /** Mirrors the XY offset applied to the front root onto the back root before the depth adjustment. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Back")
    bool bInheritFrontRootOffsetForBackPair = true;

    /** Maximum distance from which the player can interact with buttons. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0"))
    float InteractionRange = 200.0f;

    /** Text prompt displayed when player looks at a button. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FText InteractionPrompt = FText::FromString(TEXT("Press E to toggle doors"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0"))
    float FloorSelectionDoorHoldTime = 10.0f;

private:
    friend class FProceduralElevatorDoorController;

    void RefreshDoorTransforms();
    void ApplyDoorOffset(EProceduralElevatorDoorSlot Slot);
    UStaticMeshComponent *GetDoorMesh(EProceduralElevatorDoorSlot Slot) const;
    void UpdateDoorRootOffsets();
    void SetupButtonComponents();
    void SyncDoorMeshes();
    FProceduralElevatorDoorControllerParameters MakeDoorControllerParameters() const;
    void ToggleDoors();
    void RequestOpenDoors(bool bForce = false);
    void RequestCloseDoors(bool bForce = false);
    bool IsFloorButtonComponent(const UPrimitiveComponent* Component) const;
    void HandleFloorButtonPressed();
    void HandleDoorUnlockTimerElapsed();
    void CancelDoorUnlockTimer();
    void UnlockDoorsAndOpen();

    UStaticMeshComponent *FrontLeftDoorMesh;
    UStaticMeshComponent *FrontRightDoorMesh;
    UStaticMeshComponent *BackLeftDoorMesh;
    UStaticMeshComponent *BackRightDoorMesh;

    TUniquePtr<FProceduralElevatorDoorController> DoorController;

    bool bDoorsLocked = false;
    FTimerHandle DoorUnlockTimerHandle;
};
