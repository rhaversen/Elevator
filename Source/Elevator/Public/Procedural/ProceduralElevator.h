#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/UniquePtr.h"
#include "Procedural/ProceduralElevatorDoorController.h"
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
class ELEVATOR_API AProceduralElevator : public AActor
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

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent *Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent *CabMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent *FrontDoorsRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent *BackDoorsRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent *FrontLeftDoorMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent *FrontRightDoorMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent *BackLeftDoorMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent *BackRightDoorMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent *ButtonRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent *ButtonMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent *ButtonTrigger;

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

    /** Enables the built-in interaction button inside the elevator cab. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Button")
    bool bEnableDoorButton = true;

    /** Relative location for the interaction button, in cab local space. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Button")
    FVector ButtonRelativeLocation = FVector(10.0f, -90.0f, 110.0f);

    /** Relative rotation for the interaction button. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Button")
    FRotator ButtonRelativeRotation = FRotator::ZeroRotator;

    /** Box trigger extents for the interaction button overlap volume. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Button", meta = (ClampMin = "0.0"))
    FVector ButtonTriggerExtents = FVector(20.0f, 20.0f, 30.0f);

private:
    friend class FProceduralElevatorDoorController;

    void RefreshDoorTransforms();
    void ApplyDoorOffset(EProceduralElevatorDoorSlot Slot);
    UStaticMeshComponent *GetDoorMesh(EProceduralElevatorDoorSlot Slot) const;
    void UpdateDoorRootOffsets();
    void UpdateButtonConfiguration();
    FProceduralElevatorDoorControllerParameters MakeDoorControllerParameters() const;

    UFUNCTION()
    void HandleButtonOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

    TUniquePtr<FProceduralElevatorDoorController> DoorController;
};
