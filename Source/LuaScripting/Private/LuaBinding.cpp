#include "LuaBinding.h"
#include "LuaStateManager.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "EngineUtils.h"

// Include Lua headers
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

DEFINE_LOG_CATEGORY(LogLuaScripting)

void FLuaBinding::RegisterCoreFunctions(lua_State* L)
{
    // Create the UE namespace table
    lua_newtable(L);

    // Register core functions
    lua_pushcfunction(L, Lua_Print);
    lua_setfield(L, -2, "Print");

    lua_pushcfunction(L, Lua_GetDeltaTime);
    lua_setfield(L, -2, "GetDeltaTime");

    // Add GetWorld function
    lua_pushcfunction(L, Lua_GetWorld);
    lua_setfield(L, -2, "GetWorld");

    // Set the UE table as a global
    lua_setglobal(L, "UE");

    // Register the event system
    RegisterEventSystem(L);

    UE_LOG(LogLuaScripting, Log, TEXT("Core functions registered"));
}

void FLuaBinding::RegisterMathFunctions(lua_State* L)
{
    // Get the UE namespace table
    lua_getglobal(L, "UE");

    // Create the math table
    lua_newtable(L);

    // Add math functions here
    // Example: Vector operations, rotations, transforms, etc.

    // Set the math table in the UE namespace
    lua_setfield(L, -2, "Math");

    // Pop the UE table
    lua_pop(L, 1);

    UE_LOG(LogLuaScripting, Log, TEXT("Math functions registered"));
}

void FLuaBinding::RegisterLogFunctions(lua_State* L)
{
    // Get the UE namespace table
    lua_getglobal(L, "UE");

    // Create the log table
    lua_newtable(L);

    // Register log functions
    lua_pushcfunction(L, Lua_Trace);
    lua_setfield(L, -2, "Trace");

    lua_pushcfunction(L, Lua_Warning);
    lua_setfield(L, -2, "Warning");

    lua_pushcfunction(L, Lua_Error);
    lua_setfield(L, -2, "Error");

    // Set the log table in the UE namespace
    lua_setfield(L, -2, "Log");

    // Pop the UE table
    lua_pop(L, 1);

    UE_LOG(LogLuaScripting, Log, TEXT("Log functions registered"));
}

void FLuaBinding::RegisterActorFunctions(lua_State* L)
{
    // Get the UE namespace table
    lua_getglobal(L, "UE");

    // Create the Actor table
    lua_newtable(L);

    // Register actor functions
    lua_pushcfunction(L, Lua_FindActor);
    lua_setfield(L, -2, "FindActor");

    lua_pushcfunction(L, Lua_SpawnActor);
    lua_setfield(L, -2, "SpawnActor");

    lua_pushcfunction(L, Lua_DestroyActor);
    lua_setfield(L, -2, "DestroyActor");

    // Set the Actor table in the UE namespace
    lua_setfield(L, -2, "Actor");

    // Pop the UE table
    lua_pop(L, 1);

    UE_LOG(LogLuaScripting, Log, TEXT("Actor functions registered"));
}

UWorld* FLuaBinding::GetWorld(lua_State* L)
{
    // Try to get the world from the global "self" actor if available
    lua_getglobal(L, "self");
    if (!lua_isnil(L, -1))
    {
        AActor* SelfActor = Cast<AActor>(GetUObject(L, -1));
        lua_pop(L, 1);

        if (SelfActor)
        {
            return SelfActor->GetWorld();
        }
    }
    else
    {
        lua_pop(L, 1);
    }

    // Try to get the world from the component if available
    lua_getglobal(L, "component");
    if (!lua_isnil(L, -1))
    {
        UActorComponent* Component = Cast<UActorComponent>(GetUObject(L, -1));
        lua_pop(L, 1);

        if (Component)
        {
            return Component->GetWorld();
        }
    }
    else
    {
        lua_pop(L, 1);
    }

    // As a last resort, try to get the game world
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
        {
            return Context.World();
        }
    }

    return nullptr;
}

void FLuaBinding::PushUObject(lua_State* L, UObject* Object)
{
    if (!Object)
    {
        lua_pushnil(L);
        return;
    }

    // Create a userdata to hold the UObject pointer
    void** UserData = (void**)lua_newuserdata(L, sizeof(void*));
    *UserData = Object;

    // Create or get the metatable for UObject
    if (luaL_newmetatable(L, "UObject"))
    {
        // First time creation
        // Setup the __index metamethod for method dispatching
        lua_pushcfunction(L, UObjectIndex);
        lua_setfield(L, -2, "__index");

        // Setup __tostring metamethod to display UObject info
        lua_pushcfunction(L, UObjectToString);
        lua_setfield(L, -2, "__tostring");

        // Add garbage collection method
        lua_pushcfunction(L, [](lua_State* L) {
            // UObjects are managed by Unreal, not Lua, so we don't need to do anything here
            return 0;
            });
        lua_setfield(L, -2, "__gc");
    }

    // Set the metatable for this userdata
    lua_setmetatable(L, -2);
}

UObject* FLuaBinding::GetUObject(lua_State* L, int Index)
{
    if (!lua_isuserdata(L, Index))
    {
        return nullptr;
    }

    // Check if we have a valid userdata with our metatable
    void* UserData = lua_touserdata(L, Index);
    if (!UserData)
    {
        return nullptr;
    }

    // We should validate this is actually a UObject, but for simplicity we'll just cast
    return static_cast<UObject*>(*(void**)UserData);
}

void FLuaBinding::SetGlobalUObject(lua_State* L, const char* Name, UObject* Object)
{
    PushUObject(L, Object);
    lua_setglobal(L, Name);
}

// uobject handling

int FLuaBinding::UObjectIndex(lua_State* L)
{
    // Get the UObject from the userdata
    UObject* Object = GetUObject(L, 1);
    if (!Object)
    {
        return luaL_error(L, "Invalid UObject in __index");
    }

    // Get the property/method name
    const char* MethodName = lua_tostring(L, 2);
    if (!MethodName)
    {
        return luaL_error(L, "Invalid method name in __index");
    }

    // Check if this is a known method
    return DispatchUObjectMethod(L, Object, MethodName);
}

int FLuaBinding::UObjectToString(lua_State* L)
{
    UObject* Object = GetUObject(L, 1);
    if (Object)
    {
        lua_pushfstring(L, "UObject: %p", Object);

        // Add class name if available
        if (Object->GetClass())
        {
            lua_pushfstring(L, " (%s)", TCHAR_TO_UTF8(*Object->GetClass()->GetName()));
            lua_concat(L, 2);
        }
    }
    else
    {
        lua_pushstring(L, "Invalid UObject");
    }

    return 1;
}

int FLuaBinding::DispatchUObjectMethod(lua_State* L, UObject* Object, const char* MethodName)
{
    FString MethodString = UTF8_TO_TCHAR(MethodName);

    // Component methods
    UActorComponent* Component = Cast<UActorComponent>(Object);
    if (Component)
    {
        // Methods for all UActorComponent types
        if (MethodString == TEXT("GetOwner"))
        {
            AActor* Owner = Component->GetOwner();
            PushUObject(L, Owner);
            return 1;
        }
    }

    // Handle common Actor methods
    AActor* Actor = Cast<AActor>(Object);
    if (Actor)
    {
        // Location methods
        if (MethodString == TEXT("GetActorLocation"))
        {
            const FVector& Location = Actor->GetActorLocation();
            PushVector(L, Location);
            return 1;
        }
        else if (MethodString == TEXT("SetActorLocation"))
        {
            // Expect a table with X, Y, Z
            if (lua_gettop(L) < 3)
            {
                return luaL_error(L, "SetActorLocation requires a vector parameter");
            }

            FVector NewLocation = GetVector(L, 3);
            bool bSuccess = Actor->SetActorLocation(NewLocation);
            lua_pushboolean(L, bSuccess);
            return 1;
        }

        // Rotation methods
        else if (MethodString == TEXT("GetActorRotation"))
        {
            const FRotator& Rotation = Actor->GetActorRotation();
            PushRotator(L, Rotation);
            return 1;
        }
        else if (MethodString == TEXT("SetActorRotation"))
        {
            // Expect a table with Pitch, Yaw, Roll
            if (lua_gettop(L) < 3)
            {
                return luaL_error(L, "SetActorRotation requires a rotator parameter");
            }

            FRotator NewRotation = GetRotator(L, 3);
            bool bSuccess = Actor->SetActorRotation(NewRotation);
            lua_pushboolean(L, bSuccess);
            return 1;
        }

        // Scale methods
        else if (MethodString == TEXT("GetActorScale3D"))
        {
            const FVector& Scale = Actor->GetActorScale3D();
            PushVector(L, Scale);
            return 1;
        }
        else if (MethodString == TEXT("SetActorScale3D"))
        {
            // Expect a table with X, Y, Z
            if (lua_gettop(L) < 3)
            {
                return luaL_error(L, "SetActorScale3D requires a vector parameter");
            }

            FVector NewScale = GetVector(L, 3);
            Actor->SetActorScale3D(NewScale);
            return 0;
        }

        // Visibility methods
        else if (MethodString == TEXT("SetActorHiddenInGame"))
        {
            if (lua_gettop(L) < 3)
            {
                return luaL_error(L, "SetActorHiddenInGame requires a boolean parameter");
            }

            bool bNewHidden = (lua_toboolean(L, 3) != 0);
            Actor->SetActorHiddenInGame(bNewHidden);
            return 0;
        }
        else if (MethodString == TEXT("IsHidden"))
        {
            bool bHidden = Actor->IsHidden();
            lua_pushboolean(L, bHidden);
            return 1;
        }

        // Tags
        else if (MethodString == TEXT("HasTag"))
        {
            if (lua_gettop(L) < 3)
            {
                return luaL_error(L, "HasTag requires a string parameter");
            }

            const char* TagName = lua_tostring(L, 3);
            bool bHasTag = Actor->ActorHasTag(FName(UTF8_TO_TCHAR(TagName)));
            lua_pushboolean(L, bHasTag);
            return 1;
        }
        else if (MethodString == TEXT("AddTag"))
        {
            if (lua_gettop(L) < 3)
            {
                return luaL_error(L, "AddTag requires a string parameter");
            }

            const char* TagName = lua_tostring(L, 3);
            Actor->Tags.AddUnique(FName(UTF8_TO_TCHAR(TagName)));
            return 0;
        }
        else if (MethodString == TEXT("RemoveTag"))
        {
            if (lua_gettop(L) < 3)
            {
                return luaL_error(L, "RemoveTag requires a string parameter");
            }

            const char* TagName = lua_tostring(L, 3);
            Actor->Tags.Remove(FName(UTF8_TO_TCHAR(TagName)));
            return 0;
        }
        else if (MethodString == TEXT("GetNumTags"))
        {
            lua_pushinteger(L, Actor->Tags.Num());
            return 1;
        }

        // Other actor methods
        else if (MethodString == TEXT("GetLifeSpan"))
        {
            lua_pushnumber(L, Actor->GetLifeSpan());
            return 1;
        }
        else if (MethodString == TEXT("SetLifeSpan"))
        {
            if (lua_gettop(L) < 3)
            {
                return luaL_error(L, "SetLifeSpan requires a number parameter");
            }

            float Lifespan = (float)lua_tonumber(L, 3);
            Actor->SetLifeSpan(Lifespan);
            return 0;
        }
        else if (MethodString == TEXT("CanEverTick"))
        {
            lua_pushboolean(L, Actor->PrimaryActorTick.bCanEverTick);
            return 1;
        }
    }

    // UObject methods that apply to any UObject
    if (MethodString == TEXT("GetName"))
    {
        lua_pushstring(L, TCHAR_TO_UTF8(*Object->GetName()));
        return 1;
    }
    else if (MethodString == TEXT("GetClass"))
    {
        if (Object->GetClass())
        {
            lua_pushstring(L, TCHAR_TO_UTF8(*Object->GetClass()->GetName()));
        }
        else
        {
            lua_pushnil(L);
        }
        return 1;
    }
    else if (MethodString == TEXT("IsA"))
    {
        if (lua_gettop(L) < 3)
        {
            return luaL_error(L, "IsA requires a string parameter");
        }

        const char* ClassName = lua_tostring(L, 3);
        UClass* ClassToCheck = FindObject<UClass>(nullptr, UTF8_TO_TCHAR(ClassName));

        if (ClassToCheck)
        {
            lua_pushboolean(L, Object->IsA(ClassToCheck));
        }
        else
        {
            // Class not found
            lua_pushboolean(L, false);
        }
        return 1;
    }

    // If we get here, the method is not found
    // Return nil instead of raising an error to be more forgiving in scripts
    lua_pushnil(L);
    return 1;
}

// Helper functions for vector handling

void FLuaBinding::PushVector(lua_State* L, const FVector& Vector)
{
    // Create a table with X, Y, Z fields
    lua_newtable(L);

    lua_pushnumber(L, Vector.X);
    lua_setfield(L, -2, "X");

    lua_pushnumber(L, Vector.Y);
    lua_setfield(L, -2, "Y");

    lua_pushnumber(L, Vector.Z);
    lua_setfield(L, -2, "Z");
}

FVector FLuaBinding::GetVector(lua_State* L, int Index)
{
    FVector Result = FVector::ZeroVector;

    if (lua_istable(L, Index))
    {
        // Get X component
        lua_getfield(L, Index, "X");
        if (lua_isnumber(L, -1))
        {
            Result.X = (float)lua_tonumber(L, -1);
        }
        lua_pop(L, 1);

        // Get Y component
        lua_getfield(L, Index, "Y");
        if (lua_isnumber(L, -1))
        {
            Result.Y = (float)lua_tonumber(L, -1);
        }
        lua_pop(L, 1);

        // Get Z component
        lua_getfield(L, Index, "Z");
        if (lua_isnumber(L, -1))
        {
            Result.Z = (float)lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
    }

    return Result;
}

// Helper functions for rotator handling

void FLuaBinding::PushRotator(lua_State* L, const FRotator& Rotator)
{
    // Create a table with Pitch, Yaw, Roll fields
    lua_newtable(L);

    lua_pushnumber(L, Rotator.Pitch);
    lua_setfield(L, -2, "Pitch");

    lua_pushnumber(L, Rotator.Yaw);
    lua_setfield(L, -2, "Yaw");

    lua_pushnumber(L, Rotator.Roll);
    lua_setfield(L, -2, "Roll");
}

FRotator FLuaBinding::GetRotator(lua_State* L, int Index)
{
    FRotator Result = FRotator::ZeroRotator;

    if (lua_istable(L, Index))
    {
        // Get Pitch component
        lua_getfield(L, Index, "Pitch");
        if (lua_isnumber(L, -1))
        {
            Result.Pitch = (float)lua_tonumber(L, -1);
        }
        lua_pop(L, 1);

        // Get Yaw component
        lua_getfield(L, Index, "Yaw");
        if (lua_isnumber(L, -1))
        {
            Result.Yaw = (float)lua_tonumber(L, -1);
        }
        lua_pop(L, 1);

        // Get Roll component
        lua_getfield(L, Index, "Roll");
        if (lua_isnumber(L, -1))
        {
            Result.Roll = (float)lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
    }

    return Result;
}

// core lua funcs

int FLuaBinding::Lua_GetWorld(lua_State* L)
{
    // Get the current world
    UWorld* World = GetWorld(L);

    if (World)
    {
        // Push the world to Lua
        PushUObject(L, World);
    }
    else
    {
        // No world available
        lua_pushnil(L);
    }

    return 1;
}

int FLuaBinding::Lua_Print(lua_State* L)
{
    int NumArgs = lua_gettop(L);
    FString Message;

    // Concatenate all arguments
    for (int i = 1; i <= NumArgs; i++)
    {
        if (i > 1)
        {
            Message += " ";
        }

        if (lua_isstring(L, i))
        {
            Message += UTF8_TO_TCHAR(lua_tostring(L, i));
        }
        else if (lua_isnumber(L, i))
        {
            Message += FString::Printf(TEXT("%f"), lua_tonumber(L, i));
        }
        else if (lua_isboolean(L, i))
        {
            Message += lua_toboolean(L, i) ? TEXT("true") : TEXT("false");
        }
        else if (lua_isnil(L, i))
        {
            Message += TEXT("nil");
        }
        else
        {
            Message += FString::Printf(TEXT("[%s: %p]"), UTF8_TO_TCHAR(lua_typename(L, lua_type(L, i))), lua_topointer(L, i));
        }
    }

    // Print to log
    UE_LOG(LogLuaScripting, Display, TEXT("[Lua] %s"), *Message);

    // Also print to screen if in PIE or game
    UWorld* World = GetWorld(L);
    if (World && (World->WorldType == EWorldType::PIE || World->WorldType == EWorldType::Game))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, Message);
    }

    return 0;
}

int FLuaBinding::Lua_GetDeltaTime(lua_State* L)
{
    UWorld* World = GetWorld(L);
    if (World)
    {
        lua_pushnumber(L, World->GetDeltaSeconds());
    }
    else
    {
        lua_pushnumber(L, 0.0f);
    }

    return 1;
}

int FLuaBinding::Lua_Trace(lua_State* L)
{
    const char* Message = luaL_checkstring(L, 1);
    UE_LOG(LogLuaScripting, Log, TEXT("[Lua] %s"), UTF8_TO_TCHAR(Message));
    return 0;
}

int FLuaBinding::Lua_Warning(lua_State* L)
{
    const char* Message = luaL_checkstring(L, 1);
    UE_LOG(LogLuaScripting, Warning, TEXT("[Lua] %s"), UTF8_TO_TCHAR(Message));
    return 0;
}

int FLuaBinding::Lua_Error(lua_State* L)
{
    const char* Message = luaL_checkstring(L, 1);
    UE_LOG(LogLuaScripting, Error, TEXT("[Lua] %s"), UTF8_TO_TCHAR(Message));
    return 0;
}

int FLuaBinding::Lua_FindActor(lua_State* L)
{
    const char* ActorName = luaL_checkstring(L, 1);
    UWorld* World = GetWorld(L);

    if (!World)
    {
        lua_pushnil(L);
        return 1;
    }

    // Find actor by name
    FString NameToFind = UTF8_TO_TCHAR(ActorName);
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;

        if (Actor->GetName().Equals(NameToFind) || Actor->GetActorLabel().Equals(NameToFind))
        {
            PushUObject(L, Actor);
            return 1;
        }
    }

    // Actor not found
    lua_pushnil(L);
    return 1;
}

int FLuaBinding::Lua_SpawnActor(lua_State* L)
{
    // Check for class name and optional location/rotation
    const char* ClassName = luaL_checkstring(L, 1);

    UWorld* World = GetWorld(L);
    if (!World)
    {
        lua_pushnil(L);
        return 1;
    }

    // Find the UClass by name
    UClass* ClassToSpawn = nullptr;
    for (TObjectIterator<UClass> It; It; ++It)
    {
        UClass* Class = *It;

        if (Class->IsChildOf(AActor::StaticClass()) && Class->GetName().Equals(UTF8_TO_TCHAR(ClassName)))
        {
            ClassToSpawn = Class;
            break;
        }
    }

    if (!ClassToSpawn)
    {
        lua_pushnil(L);
        return 1;
    }

    // Default spawn parameters
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;

    // Get location and rotation if provided
    if (lua_gettop(L) >= 4)
    {
        Location.X = (float)luaL_checknumber(L, 2);
        Location.Y = (float)luaL_checknumber(L, 3);
        Location.Z = (float)luaL_checknumber(L, 4);
    }

    if (lua_gettop(L) >= 7)
    {
        Rotation.Pitch = (float)luaL_checknumber(L, 5);
        Rotation.Yaw = (float)luaL_checknumber(L, 6);
        Rotation.Roll = (float)luaL_checknumber(L, 7);
    }

    // Spawn the actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AActor* NewActor = World->SpawnActor<AActor>(ClassToSpawn, Location, Rotation, SpawnParams);

    if (NewActor)
    {
        PushUObject(L, NewActor);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

int FLuaBinding::Lua_DestroyActor(lua_State* L)
{
    AActor* Actor = Cast<AActor>(GetUObject(L, 1));

    if (Actor)
    {
        Actor->Destroy();
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}

void FLuaBinding::RegisterEventSystem(lua_State* L)
{
    // Create the event system table
    lua_getglobal(L, "UE");
    lua_newtable(L);

    // Create events table to store registered events
    lua_newtable(L);
    lua_setfield(L, -2, "_events");

    // Add trigger function
    lua_pushcfunction(L, [](lua_State* L) {
        const char* EventName = luaL_checkstring(L, 1);

        // Get the events table
        lua_getglobal(L, "UE");
        lua_getfield(L, -1, "Event");
        lua_getfield(L, -1, "_events");

        // Get the event handlers for this event
        lua_getfield(L, -1, EventName);

        if (lua_istable(L, -1))
        {
            // Get the number of arguments (minus event name)
            int NumArgs = lua_gettop(L) - 4;

            // For each handler, call it with the arguments
            int TableLen = (int)lua_rawlen(L, -1);
            for (int i = 1; i <= TableLen; i++)
            {
                lua_rawgeti(L, -1, i);

                if (lua_isfunction(L, -1))
                {
                    // Push the arguments
                    for (int arg = 2; arg <= NumArgs + 1; arg++)
                    {
                        lua_pushvalue(L, arg);
                    }

                    // Call the function
                    lua_call(L, NumArgs, 0);
                }
                else
                {
                    lua_pop(L, 1);
                }
            }
        }

        // Cleanup
        lua_pop(L, 4);

        return 0;
        });
    lua_setfield(L, -2, "Trigger");

    // Add register function
    lua_pushcfunction(L, [](lua_State* L) {
        const char* EventName = luaL_checkstring(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        // Get the events table
        lua_getglobal(L, "UE");
        lua_getfield(L, -1, "Event");
        lua_getfield(L, -1, "_events");

        // Get or create the table for this event
        lua_getfield(L, -1, EventName);
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, EventName);
        }

        // Add the function to the event table
        int TableLen = (int)lua_rawlen(L, -1) + 1;
        lua_pushvalue(L, 2);
        lua_rawseti(L, -2, TableLen);

        // Cleanup
        lua_pop(L, 4);

        return 0;
        });
    lua_setfield(L, -2, "Register");

    // Add unregister function
    lua_pushcfunction(L, [](lua_State* L) {
        const char* EventName = luaL_checkstring(L, 1);

        // Get the events table
        lua_getglobal(L, "UE");
        lua_getfield(L, -1, "Event");
        lua_getfield(L, -1, "_events");

        // Clear the event table for this event
        lua_pushnil(L);
        lua_setfield(L, -2, EventName);

        // Cleanup
        lua_pop(L, 3);

        return 0;
        });
    lua_setfield(L, -2, "Unregister");

    // Set the Event table in the UE namespace
    lua_setfield(L, -2, "Event");

    // Pop the UE table
    lua_pop(L, 1);
}