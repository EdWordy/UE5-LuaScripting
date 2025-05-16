#pragma once

#include "CoreMinimal.h"

// Forward declarations
struct lua_State;
class AActor;
class UWorld;

/**
 * Class for binding Unreal Engine functionality to Lua
 */
class LUASCRIPTING_API FLuaBinding
{
public:
    /**
     * Register core UE functions with the Lua state
     * @param L The Lua state to register functions with
     */
    static void RegisterCoreFunctions(lua_State* L);

    /**
     * Register math functions with the Lua state
     * @param L The Lua state to register functions with
     */
    static void RegisterMathFunctions(lua_State* L);

    /**
     * Register logging functions with the Lua state
     * @param L The Lua state to register functions with
     */
    static void RegisterLogFunctions(lua_State* L);

    /**
     * Register actor-related functions with the Lua state
     * @param L The Lua state to register functions with
     */
    static void RegisterActorFunctions(lua_State* L);

    /**
     * Get the current UWorld from the Lua state
     * @param L The Lua state
     * @return Pointer to the current UWorld or nullptr if not available
     */
    static UWorld* GetWorld(lua_State* L);

    /**
     * Push a UObject to the Lua stack
     * @param L The Lua state
     * @param Object The UObject to push
     */
    static void PushUObject(lua_State* L, UObject* Object);

    /**
     * Get a UObject from the Lua stack
     * @param L The Lua state
     * @param Index The stack index
     * @return The UObject at the given stack index or nullptr if not a UObject
     */
    static UObject* GetUObject(lua_State* L, int Index);

    /**
     * Set a global UObject in the Lua state
     * @param L The Lua state
     * @param Name The name to use for the global
     * @param Object The UObject to set as a global
     */
    static void SetGlobalUObject(lua_State* L, const char* Name, UObject* Object);

private:
    // Method dispatching
    static int UObjectIndex(lua_State* L);
    static int UObjectToString(lua_State* L);
    static int DispatchUObjectMethod(lua_State* L, UObject* Object, const char* MethodName);

    // Helper functions for common UE structs
    static void PushVector(lua_State* L, const FVector& Vector);
    static FVector GetVector(lua_State* L, int Index);
    static void PushRotator(lua_State* L, const FRotator& Rotator);
    static FRotator GetRotator(lua_State* L, int Index);

    // Core function implementations (Lua C functions)
    static int Lua_GetWorld(lua_State* L);
    static int Lua_Print(lua_State* L);
    static int Lua_GetDeltaTime(lua_State* L);
    static int Lua_Trace(lua_State* L);
    static int Lua_Warning(lua_State* L);
    static int Lua_Error(lua_State* L);
    static int Lua_FindActor(lua_State* L);
    static int Lua_SpawnActor(lua_State* L);
    static int Lua_DestroyActor(lua_State* L);

    // Helper function to register the event system
    static void RegisterEventSystem(lua_State* L);
};