#include "LuaBinding.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/WeakObjectPtr.h"

// Include Lua headers
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

// UObject userdata structure for Lua
struct LuaUObject
{
    TWeakObjectPtr<UObject> Object;
};

// MetaTable name for UObjects
static const char* UObjectMetaTableName = "UE.UObject";

void FLuaBinding::RegisterCoreFunctions(lua_State* L)
{
    // Create a table for the UE namespace
    lua_newtable(L);

    // Register the print function
    lua_pushcfunction(L, Lua_Print);
    lua_setfield(L, -2, "Print");

    // Register the GetDeltaTime function
    lua_pushcfunction(L, Lua_GetDeltaTime);
    lua_setfield(L, -2, "GetDeltaTime");

    // Set the UE table as a global
    lua_setglobal(L, "UE");

    // Create a metatable for UObjects
    luaL_newmetatable(L, UObjectMetaTableName);

    // Set the __gc metamethod for UObject cleanup
    lua_pushcfunction(L, [](lua_State* L) -> int {
        LuaUObject* UserData = (LuaUObject*)luaL_checkudata(L, 1, UObjectMetaTableName);
        UserData->Object.Reset();
        return 0;
        });
    lua_setfield(L, -2, "__gc");

    // Set the __index metamethod for UObject property access
    lua_pushcfunction(L, [](lua_State* L) -> int {
        LuaUObject* UserData = (LuaUObject*)luaL_checkudata(L, 1, UObjectMetaTableName);
        const char* PropertyName = luaL_checkstring(L, 2);

        if (!UserData->Object.IsValid())
        {
            return luaL_error(L, "Attempt to access property of invalid UObject");
        }

        UObject* Object = UserData->Object.Get();
        UClass* Class = Object->GetClass();

        // Look for a property with the given name
        FProperty* Property = Class->FindPropertyByName(FName(PropertyName));
        if (Property)
        {
            // Get property value and push to Lua stack
            // This is a simplified example - a real implementation would handle different property types
            if (Property->IsA<FBoolProperty>())
            {
                FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property);
                bool Value = BoolProperty->GetPropertyValue_InContainer(Object);
                lua_pushboolean(L, Value);
                return 1;
            }
            else if (Property->IsA<FIntProperty>())
            {
                FIntProperty* IntProperty = CastField<FIntProperty>(Property);
                int32 Value = IntProperty->GetPropertyValue_InContainer(Object);
                lua_pushinteger(L, Value);
                return 1;
            }
            else if (Property->IsA<FFloatProperty>())
            {
                FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property);
                float Value = FloatProperty->GetPropertyValue_InContainer(Object);
                lua_pushnumber(L, Value);
                return 1;
            }
            else if (Property->IsA<FStrProperty>())
            {
                FStrProperty* StrProperty = CastField<FStrProperty>(Property);
                FString Value = StrProperty->GetPropertyValue_InContainer(Object);
                lua_pushstring(L, TCHAR_TO_UTF8(*Value));
                return 1;
            }
            else if (Property->IsA<FObjectProperty>())
            {
                FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property);
                UObject* Value = ObjectProperty->GetPropertyValue_InContainer(Object);
                PushUObject(L, Value);
                return 1;
            }
            // Add more property types as needed
        }

        // Look for a function with the given name
        UFunction* Function = Class->FindFunctionByName(FName(PropertyName));
        if (Function)
        {
            // Push a closure that will call the UFunction when invoked
            lua_pushcfunction(L, [](lua_State* L) -> int {
                LuaUObject* UserData = (LuaUObject*)luaL_checkudata(L, lua_upvalueindex(1), UObjectMetaTableName);
                const char* FunctionName = lua_tostring(L, lua_upvalueindex(2));

                if (!UserData->Object.IsValid())
                {
                    return luaL_error(L, "Attempt to call method of invalid UObject");
                }

                UObject* Object = UserData->Object.Get();
                UFunction* Function = Object->FindFunction(FName(FunctionName));

                if (!Function)
                {
                    return luaL_error(L, "Function %s not found", FunctionName);
                }

                // This is a simplified example. A real implementation would need to:
                // 1. Create a buffer for the function parameters
                // 2. Fill the buffer with the parameters from the Lua stack
                // 3. Call the function
                // 4. Extract the return values and push them to the Lua stack

                // For now, just call the function with no parameters
                Object->ProcessEvent(Function, nullptr);

                // Return 0 values to Lua
                return 0;
                });

            // Set upvalues for the closure
            lua_pushvalue(L, 1); // The UObject userdata
            lua_pushstring(L, PropertyName); // The function name
            lua_setupvalue(L, -3, 1);
            lua_setupvalue(L, -2, 2);

            return 1;
        }

        // Property or function not found
        lua_pushnil(L);
        return 1;
        });
    lua_setfield(L, -2, "__index");

    // Set the __newindex metamethod for UObject property assignment
    lua_pushcfunction(L, [](lua_State* L) -> int {
        LuaUObject* UserData = (LuaUObject*)luaL_checkudata(L, 1, UObjectMetaTableName);
        const char* PropertyName = luaL_checkstring(L, 2);

        if (!UserData->Object.IsValid())
        {
            return luaL_error(L, "Attempt to set property of invalid UObject");
        }

        UObject* Object = UserData->Object.Get();
        UClass* Class = Object->GetClass();

        // Look for a property with the given name
        FProperty* Property = Class->FindPropertyByName(FName(PropertyName));
        if (Property)
        {
            // Set property value from Lua stack
            // This is a simplified example - a real implementation would handle different property types
            if (Property->IsA<FBoolProperty>())
            {
                FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property);
                bool Value = lua_toboolean(L, 3) != 0;
                BoolProperty->SetPropertyValue_InContainer(Object, Value);
            }
            else if (Property->IsA<FIntProperty>())
            {
                FIntProperty* IntProperty = CastField<FIntProperty>(Property);
                int32 Value = (int32)lua_tointeger(L, 3);
                IntProperty->SetPropertyValue_InContainer(Object, Value);
            }
            else if (Property->IsA<FFloatProperty>())
            {
                FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property);
                float Value = (float)lua_tonumber(L, 3);
                FloatProperty->SetPropertyValue_InContainer(Object, Value);
            }
            else if (Property->IsA<FStrProperty>())
            {
                FStrProperty* StrProperty = CastField<FStrProperty>(Property);
                const char* CStrValue = lua_tostring(L, 3);
                FString Value = UTF8_TO_TCHAR(CStrValue);
                StrProperty->SetPropertyValue_InContainer(Object, Value);
            }
            else if (Property->IsA<FObjectProperty>())
            {
                FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property);
                UObject* Value = GetUObject(L, 3);
                ObjectProperty->SetPropertyValue_InContainer(Object, Value);
            }
            // Add more property types as needed

            return 0;
        }

        // Property not found
        return luaL_error(L, "Property %s not found or cannot be set", PropertyName);
        });
    lua_setfield(L, -2, "__newindex");

    // Pop the metatable
    lua_pop(L, 1);
}

void FLuaBinding::RegisterMathFunctions(lua_State* L)
{
    // Create a table for the UE.Math namespace
    lua_getglobal(L, "UE");
    lua_newtable(L);

    // Push vector creation function
    lua_pushcfunction(L, [](lua_State* L) -> int {
        float X = (float)luaL_checknumber(L, 1);
        float Y = (float)luaL_checknumber(L, 2);
        float Z = (float)luaL_checknumber(L, 3);

        // Create a table to represent the vector
        lua_newtable(L);

        lua_pushnumber(L, X);
        lua_setfield(L, -2, "X");

        lua_pushnumber(L, Y);
        lua_setfield(L, -2, "Y");

        lua_pushnumber(L, Z);
        lua_setfield(L, -2, "Z");

        return 1;
        });
    lua_setfield(L, -2, "Vector");

    // Push rotation creation function
    lua_pushcfunction(L, [](lua_State* L) -> int {
        float Pitch = (float)luaL_checknumber(L, 1);
        float Yaw = (float)luaL_checknumber(L, 2);
        float Roll = (float)luaL_checknumber(L, 3);

        // Create a table to represent the rotation
        lua_newtable(L);

        lua_pushnumber(L, Pitch);
        lua_setfield(L, -2, "Pitch");

        lua_pushnumber(L, Yaw);
        lua_setfield(L, -2, "Yaw");

        lua_pushnumber(L, Roll);
        lua_setfield(L, -2, "Roll");

        return 1;
        });
    lua_setfield(L, -2, "Rotation");

    // Set the Math table in the UE namespace
    lua_setfield(L, -2, "Math");

    // Pop the UE table
    lua_pop(L, 1);
}

void FLuaBinding::RegisterLogFunctions(lua_State* L)
{
    // Create a table for the UE.Log namespace
    lua_getglobal(L, "UE");
    lua_newtable(L);

    // Register log functions
    lua_pushcfunction(L, Lua_Trace);
    lua_setfield(L, -2, "Trace");

    lua_pushcfunction(L, Lua_Warning);
    lua_setfield(L, -2, "Warning");

    lua_pushcfunction(L, Lua_Error);
    lua_setfield(L, -2, "Error");

    // Set the Log table in the UE namespace
    lua_setfield(L, -2, "Log");

    // Pop the UE table
    lua_pop(L, 1);
}

void FLuaBinding::RegisterActorFunctions(lua_State* L)
{
    // Create a table for the UE.Actor namespace
    lua_getglobal(L, "UE");
    lua_newtable(L);

    // Register actor functions
    lua_pushcfunction(L, Lua_FindActor);
    lua_setfield(L, -2, "Find");

    lua_pushcfunction(L, Lua_SpawnActor);
    lua_setfield(L, -2, "Spawn");

    lua_pushcfunction(L, Lua_DestroyActor);
    lua_setfield(L, -2, "Destroy");

    // Set the Actor table in the UE namespace
    lua_setfield(L, -2, "Actor");

    // Pop the UE table
    lua_pop(L, 1);
}

UWorld* FLuaBinding::GetWorld(lua_State* L)
{
    // Try to get the first valid World
    for (TObjectIterator<UWorld> It; It; ++It)
    {
        UWorld* World = *It;
        if (World && World->IsGameWorld())
        {
            return World;
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
    LuaUObject* UserData = (LuaUObject*)lua_newuserdata(L, sizeof(LuaUObject));
    UserData->Object = Object;

    // Set the metatable for the userdata
    luaL_getmetatable(L, UObjectMetaTableName);
    lua_setmetatable(L, -2);
}

UObject* FLuaBinding::GetUObject(lua_State* L, int Index)
{
    if (lua_isnil(L, Index))
    {
        return nullptr;
    }

    LuaUObject* UserData = (LuaUObject*)luaL_checkudata(L, Index, UObjectMetaTableName);
    if (!UserData || !UserData->Object.IsValid())
    {
        return nullptr;
    }

    return UserData->Object.Get();
}

void FLuaBinding::SetGlobalUObject(lua_State* L, const char* Name, UObject* Object)
{
    PushUObject(L, Object);
    lua_setglobal(L, Name);
}

int FLuaBinding::Lua_Print(lua_State* L)
{
    int NumArgs = lua_gettop(L);
    FString Message;

    for (int i = 1; i <= NumArgs; i++)
    {
        const char* Str = lua_tostring(L, i);
        if (Str)
        {
            Message += i > 1 ? TEXT("\t") : TEXT("");
            Message += UTF8_TO_TCHAR(Str);
        }
    }

    UE_LOG(LogTemp, Display, TEXT("Lua: %s"), *Message);

    // Print to screen if in PIE or Game mode
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Lua: %s"), *Message));
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
    UE_LOG(LogTemp, Display, TEXT("Lua: %s"), UTF8_TO_TCHAR(Message));
    return 0;
}

int FLuaBinding::Lua_Warning(lua_State* L)
{
    const char* Message = luaL_checkstring(L, 1);
    UE_LOG(LogTemp, Warning, TEXT("Lua Warning: %s"), UTF8_TO_TCHAR(Message));

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("Lua Warning: %s"), UTF8_TO_TCHAR(Message)));
    }

    return 0;
}

int FLuaBinding::Lua_Error(lua_State* L)
{
    const char* Message = luaL_checkstring(L, 1);
    UE_LOG(LogTemp, Error, TEXT("Lua Error: %s"), UTF8_TO_TCHAR(Message));

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Lua Error: %s"), UTF8_TO_TCHAR(Message)));
    }

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

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->GetName() == UTF8_TO_TCHAR(ActorName))
        {
            PushUObject(L, Actor);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

int FLuaBinding::Lua_SpawnActor(lua_State* L)
{
    UWorld* World = GetWorld(L);
    if (!World)
    {
        return luaL_error(L, "No valid World found");
    }

    // Get the class of actor to spawn
    const char* ClassName = luaL_checkstring(L, 1);
    UClass* ActorClass = FindObject<UClass>(nullptr, UTF8_TO_TCHAR(ClassName));

    if (!ActorClass || !ActorClass->IsChildOf(AActor::StaticClass()))
    {
        return luaL_error(L, "Invalid actor class: %s", ClassName);
    }

    // Get spawn transform
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;

    // Check if a location was provided
    if (lua_istable(L, 2))
    {
        lua_getfield(L, 2, "X");
        Location.X = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, 2, "Y");
        Location.Y = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, 2, "Z");
        Location.Z = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    // Check if a rotation was provided
    if (lua_istable(L, 3))
    {
        lua_getfield(L, 3, "Pitch");
        Rotation.Pitch = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, 3, "Yaw");
        Rotation.Yaw = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, 3, "Roll");
        Rotation.Roll = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    // Spawn parameters
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // Spawn the actor
    AActor* Actor = World->SpawnActor(ActorClass, &Location, &Rotation, SpawnParams);

    if (Actor)
    {
        PushUObject(L, Actor);
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

    if (!Actor)
    {
        return luaL_error(L, "Invalid actor provided for destruction");
    }

    Actor->Destroy();
    lua_pushboolean(L, true);
    return 1;
}