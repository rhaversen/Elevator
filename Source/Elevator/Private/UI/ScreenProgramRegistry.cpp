#include "UI/ScreenProgramRegistry.h"

#include "Logging/LogMacros.h"
#include "Misc/ScopeLock.h"
#include "UI/IScreenProgram.h"

FScreenProgramRegistry& FScreenProgramRegistry::Get()
{
    static FScreenProgramRegistry GRegistry;
    return GRegistry;
}

void FScreenProgramRegistry::RegisterProgram(FName ProgramId, FProgramFactory Factory)
{
    if (ProgramId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted to register workstation program with None identifier."));
        return;
    }

    FScopeLock Lock(&RegistryMutex);
    if (!Factory)
    {
        UE_LOG(LogTemp, Warning, TEXT("Removing workstation program '%s' because factory delegate is invalid."), *ProgramId.ToString());
        Factories.Remove(ProgramId);
        return;
    }

    Factories.Add(ProgramId, MoveTemp(Factory));
    UE_LOG(LogTemp, Verbose, TEXT("Registered workstation program '%s'."), *ProgramId.ToString());
}

void FScreenProgramRegistry::UnregisterProgram(FName ProgramId)
{
    if (ProgramId.IsNone())
    {
        return;
    }

    FScopeLock Lock(&RegistryMutex);
    if (Factories.Remove(ProgramId) > 0)
    {
        UE_LOG(LogTemp, Verbose, TEXT("Unregistered workstation program '%s'."), *ProgramId.ToString());
    }
}

TSharedPtr<IScreenProgram> FScreenProgramRegistry::CreateProgram(FName ProgramId) const
{
    return CreateProgramInternal(ProgramId);
}

bool FScreenProgramRegistry::IsProgramRegistered(FName ProgramId) const
{
    if (ProgramId.IsNone())
    {
        return false;
    }

    FScopeLock Lock(&RegistryMutex);
    return Factories.Contains(ProgramId);
}

TArray<FName> FScreenProgramRegistry::GetRegisteredProgramIds() const
{
    FScopeLock Lock(&RegistryMutex);
    TArray<FName> Keys;
    Factories.GetKeys(Keys);
    return Keys;
}

TSharedPtr<IScreenProgram> FScreenProgramRegistry::CreateProgramInternal(FName ProgramId) const
{
    if (ProgramId.IsNone())
    {
        return nullptr;
    }

    FProgramFactory Factory;
    {
        FScopeLock Lock(&RegistryMutex);
        const FProgramFactory* Found = Factories.Find(ProgramId);
        if (!Found)
        {
            return nullptr;
        }
        Factory = *Found;
    }

    if (!Factory)
    {
        return nullptr;
    }

    return Factory();
}
