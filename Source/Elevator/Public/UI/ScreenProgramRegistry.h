#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "HAL/CriticalSection.h"

class IScreenProgram;

/**
 * Global registry mapping workstation program identifiers to factory delegates.
 * Enables modular registration of new screen programs without touching central subsystems.
 */
class ELEVATOR_API FScreenProgramRegistry
{
public:
    using FProgramFactory = TFunction<TSharedPtr<IScreenProgram>()>;

    static FScreenProgramRegistry& Get();

    /** Register or replace a program factory for the given identifier. */
    void RegisterProgram(FName ProgramId, FProgramFactory Factory);

    /** Remove a program factory previously registered. */
    void UnregisterProgram(FName ProgramId);

    /** Try to create a program instance; returns null if no factory is registered. */
    TSharedPtr<IScreenProgram> CreateProgram(FName ProgramId) const;

    /** Check whether the given identifier has a registered factory. */
    bool IsProgramRegistered(FName ProgramId) const;

    /** Enumerate all registered program identifiers. */
    TArray<FName> GetRegisteredProgramIds() const;

private:
    FScreenProgramRegistry() = default;
    FScreenProgramRegistry(const FScreenProgramRegistry&) = delete;
    FScreenProgramRegistry& operator=(const FScreenProgramRegistry&) = delete;

    TSharedPtr<IScreenProgram> CreateProgramInternal(FName ProgramId) const;

    mutable FCriticalSection RegistryMutex;
    TMap<FName, FProgramFactory> Factories;
};
