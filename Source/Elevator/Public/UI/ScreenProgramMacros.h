#pragma once

#include "UI/ScreenProgramRegistry.h"

/**
 * Macro to auto-register a screen program with the registry.
 * Place this in the .cpp file after includes.
 * 
 * Usage:
 *   REGISTER_SCREEN_PROGRAM(MyProgram, "MyProgramId")
 * 
 * This expands to a static registrar that adds the program factory on module load.
 */
#define REGISTER_SCREEN_PROGRAM(ProgramClass, ProgramIdString) \
    namespace { \
        struct F##ProgramClass##Registrar \
        { \
            F##ProgramClass##Registrar() \
            { \
                FScreenProgramRegistry::Get().RegisterProgram( \
                    FName(TEXT(ProgramIdString)), \
                    []() { return MakeShared<ProgramClass>(); }); \
            } \
        } G##ProgramClass##Registrar; \
    }
