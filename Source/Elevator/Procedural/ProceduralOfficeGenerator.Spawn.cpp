#include "../ProceduralOfficeGenerator.h"

#include "Components/ChildActorComponent.h"
#include "GameFramework/PlayerStart.h"

void AProceduralOfficeGenerator::PlaceSpawnPoint(const FVector2D &Location, float HeightOffset, float Yaw)
{
    const FVector SpawnLocation(Location.X, Location.Y, FloorHeight + HeightOffset);
    const FRotator SpawnRotation(0.0f, Yaw, 0.0f);

    const FName ComponentName = MakeUniqueObjectName(this, UChildActorComponent::StaticClass(), FName(TEXT("ProceduralSpawnPoint")));
    UChildActorComponent *SpawnComponent = NewObject<UChildActorComponent>(this, ComponentName);
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

    if (APlayerStart *PlayerStart = Cast<APlayerStart>(SpawnComponent->GetChildActor()))
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
