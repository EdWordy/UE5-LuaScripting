#include "LuaScriptComponent.h"
#include "LuaStateManager.h"
#include "LuaBinding.h"

// Include Lua headers
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

ULuaScriptComponent::ULuaScriptComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoRun = true;
    bCallTickFunction = true;
    bScriptInitialized = false;
    ComponentLuaState = nullptr;
    GCInterval = 30;  // Run GC every 30 frames
    GCCounter = 0;
}

void ULuaScriptComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoRun)
    {
        FString ErrorMessage;
        if (!ExecuteScript(ErrorMessage))
        {
            UE_LOG(LogLuaScripting, Error, TEXT("Failed to execute Lua script: %s"), *ErrorMessage);
        }
    }
}

void ULuaScriptComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up the Lua environment
    CleanupLuaEnvironment();

    Super::EndPlay(EndPlayReason);
}

void ULuaScriptComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bScriptInitialized && bCallTickFunction && ComponentLuaState)
    {
        // Call the tick function if it exists
        lua_getglobal(ComponentLuaState, "tick");
        if (lua_isfunction(ComponentLuaState, -1))
        {
            lua_pushnumber(ComponentLuaState, DeltaTime);

            // Call the function (1 argument, 0 results)
            if (lua_pcall(ComponentLuaState, 1, 0, 0) != LUA_OK)
            {
                FString ErrorMessage = UTF8_TO_TCHAR(lua_tostring(ComponentLuaState, -1));
                UE_LOG(LogLuaScripting, Error, TEXT("Error in Lua tick function: %s"), *ErrorMessage);
                lua_pop(ComponentLuaState, 1);
            }
        }
        else
        {
            // Pop the non-function value
            lua_pop(ComponentLuaState, 1);
        }

        // Run garbage collection periodically
        if (++GCCounter >= GCInterval)
        {
            GCCounter = 0;
            FLuaStateManager::Get().RunGarbageCollection(ComponentLuaState);
        }
    }
}

bool ULuaScriptComponent::ExecuteScript(FString & ErrorMessage)
{
    // Clean up any existing environment
    CleanupLuaEnvironment();

    // Initialize the Lua environment
    if (!InitializeLuaEnvironment(ErrorMessage))
    {
        return false;
    }

    // Determine script content to execute
    FString ContentToExecute = DetermineScriptContent();
    if (ContentToExecute.IsEmpty())
    {
        ErrorMessage = TEXT("No script content available");
        return false;
    }

    // Load and execute the script
    return LoadAndExecuteScript(ContentToExecute, ErrorMessage);
}

FString ULuaScriptComponent::DetermineScriptContent() const
{
    // Determine script content to execute
    FString ContentToExecute;
    if (ScriptAsset)
    {
        ContentToExecute = ScriptAsset->ScriptContent;
    }
    else if (!ScriptContent.IsEmpty())
    {
        ContentToExecute = ScriptContent;
    }

    return ContentToExecute;
}

bool ULuaScriptComponent::LoadAndExecuteScript(const FString & ContentToExecute, FString & ErrorMessage)
{
    if (!ComponentLuaState)
    {
        ErrorMessage = TEXT("Lua environment not initialized");
        return false;
    }

    // Load the script
    int Status = luaL_loadstring(ComponentLuaState, TCHAR_TO_UTF8(*ContentToExecute));
    if (Status != LUA_OK)
    {
        ErrorMessage = UTF8_TO_TCHAR(lua_tostring(ComponentLuaState, -1));
        lua_pop(ComponentLuaState, 1);
        return false;
    }

    // Execute the script
    Status = lua_pcall(ComponentLuaState, 0, LUA_MULTRET, 0);
    if (Status != LUA_OK)
    {
        ErrorMessage = UTF8_TO_TCHAR(lua_tostring(ComponentLuaState, -1));
        lua_pop(ComponentLuaState, 1);
        return false;
    }

    // Call the init function if it exists
    lua_getglobal(ComponentLuaState, "init");
    if (lua_isfunction(ComponentLuaState, -1))
    {
        Status = lua_pcall(ComponentLuaState, 0, 0, 0);
        if (Status != LUA_OK)
        {
            ErrorMessage = UTF8_TO_TCHAR(lua_tostring(ComponentLuaState, -1));
            lua_pop(ComponentLuaState, 1);
            return false;
        }
    }
    else
    {
        // Pop the non-function value
        lua_pop(ComponentLuaState, 1);
    }

    bScriptInitialized = true;
    return true;
}

bool ULuaScriptComponent::CallFunction(const FString & FunctionName, FString & ErrorMessage)
{
    if (!ComponentLuaState)
    {
        ErrorMessage = TEXT("Lua environment not initialized");
        return false;
    }

    // Get the function from the global table
    lua_getglobal(ComponentLuaState, TCHAR_TO_UTF8(*FunctionName));
    if (!lua_isfunction(ComponentLuaState, -1))
    {
        lua_pop(ComponentLuaState, 1);
        ErrorMessage = FString::Printf(TEXT("Function '%s' not found in script"), *FunctionName);
        return false;
    }

    // Call the function (0 arguments, 0 results)
    int Status = lua_pcall(ComponentLuaState, 0, 0, 0);
    if (Status != LUA_OK)
    {
        ErrorMessage = UTF8_TO_TCHAR(lua_tostring(ComponentLuaState, -1));
        lua_pop(ComponentLuaState, 1);
        return false;
    }

    return true;
}

bool ULuaScriptComponent::HotReloadScript(FString & ErrorMessage)
{
    if (!bScriptInitialized || !ComponentLuaState)
    {
        return ExecuteScript(ErrorMessage); // Just do a normal load if not initialized
    }

    // Store current script state
    FString StateVars = PreserveScriptState();

    // Determine script content
    FString ContentToExecute = DetermineScriptContent();
    if (ContentToExecute.IsEmpty())
    {
        ErrorMessage = TEXT("No script content available for hot reload");
        return false;
    }

    // Load and execute the updated script
    bool Success = LoadAndExecuteScript(ContentToExecute, ErrorMessage);

    if (Success)
    {
        // Restore previous state
        RestoreScriptState(StateVars);
        UE_LOG(LogLuaScripting, Log, TEXT("Hot reload successful for script on %s"), *GetOwner()->GetName());
    }
    else
    {
        UE_LOG(LogLuaScripting, Error, TEXT("Hot reload failed: %s"), *ErrorMessage);
    }

    return Success;
}

FString ULuaScriptComponent::PreserveScriptState() const
{
    if (!ComponentLuaState)
    {
        return TEXT("");
    }

    // Create a table to store all global variables
    lua_newtable(ComponentLuaState);

    // Get the global table (_G)
    lua_getglobal(ComponentLuaState, "_G");

    // Iterate through all global variables
    lua_pushnil(ComponentLuaState);  // First key
    while (lua_next(ComponentLuaState, -2) != 0)
    {
        // Skip functions, tables with functions, and standard Lua libraries
        if (!lua_isfunction(ComponentLuaState, -1) &&
            lua_type(ComponentLuaState, -2) == LUA_TSTRING)
        {
            const char* Key = lua_tostring(ComponentLuaState, -2);

            // Skip standard Lua globals and modules
            if (strcmp(Key, "_G") != 0 &&
                strcmp(Key, "UE") != 0 &&
                strcmp(Key, "package") != 0 &&
                strcmp(Key, "string") != 0 &&
                strcmp(Key, "math") != 0 &&
                strcmp(Key, "table") != 0 &&
                strcmp(Key, "io") != 0 &&
                strcmp(Key, "os") != 0)
            {
                // Duplicate key and value
                lua_pushvalue(ComponentLuaState, -2);  // Key
                lua_pushvalue(ComponentLuaState, -2);  // Value

                // Set key=value in our storage table
                lua_rawset(ComponentLuaState, -6);  // -6 points to our storage table (accounting for _G on stack)
            }
        }

        // Remove value, keep key for next iteration
        lua_pop(ComponentLuaState, 1);
    }

    // Pop the global table
    lua_pop(ComponentLuaState, 1);

    // Convert the table to a string representation (for simple vars only)
    FString Result;

    // Iterate through the table
    lua_pushnil(ComponentLuaState);  // First key
    while (lua_next(ComponentLuaState, -2) != 0)
    {
        // Only handle simple types
        if (lua_type(ComponentLuaState, -2) == LUA_TSTRING)
        {
            const char* Key = lua_tostring(ComponentLuaState, -2);

            if (lua_isnumber(ComponentLuaState, -1))
            {
                Result += FString::Printf(TEXT("%s=%f;"), UTF8_TO_TCHAR(Key), lua_tonumber(ComponentLuaState, -1));
            }
            else if (lua_isstring(ComponentLuaState, -1))
            {
                Result += FString::Printf(TEXT("%s=\"%s\";"), UTF8_TO_TCHAR(Key), UTF8_TO_TCHAR(lua_tostring(ComponentLuaState, -1)));
            }
            else if (lua_isboolean(ComponentLuaState, -1))
            {
                Result += FString::Printf(TEXT("%s=%s;"), UTF8_TO_TCHAR(Key), lua_toboolean(ComponentLuaState, -1) ? TEXT("true") : TEXT("false"));
            }
        }

        // Remove value, keep key for next iteration
        lua_pop(ComponentLuaState, 1);
    }

    // Pop the storage table
    lua_pop(ComponentLuaState, 1);

    return Result;
}

void ULuaScriptComponent::RestoreScriptState(const FString & StateVars)
{
    if (!ComponentLuaState || StateVars.IsEmpty())
    {
        return;
    }

    // Parse the state string and set global variables
    TArray<FString> VarPairs;
    StateVars.ParseIntoArray(VarPairs, TEXT(";"), true);

    for (const FString& Pair : VarPairs)
    {
        FString Key, Value;
        if (Pair.Split(TEXT("="), &Key, &Value))
        {
            if (Value.StartsWith(TEXT("\"")))
            {
                // String value
                Value = Value.Mid(1, Value.Len() - 2);  // Remove quotes
                lua_pushstring(ComponentLuaState, TCHAR_TO_UTF8(*Value));
                lua_setglobal(ComponentLuaState, TCHAR_TO_UTF8(*Key));
            }
            else if (Value.Equals(TEXT("true"), ESearchCase::IgnoreCase))
            {
                // Boolean true
                lua_pushboolean(ComponentLuaState, 1);
                lua_setglobal(ComponentLuaState, TCHAR_TO_UTF8(*Key));
            }
            else if (Value.Equals(TEXT("false"), ESearchCase::IgnoreCase))
            {
                // Boolean false
                lua_pushboolean(ComponentLuaState, 0);
                lua_setglobal(ComponentLuaState, TCHAR_TO_UTF8(*Key));
            }
            else
            {
                // Number value
                double NumValue = FCString::Atod(*Value);
                lua_pushnumber(ComponentLuaState, NumValue);
                lua_setglobal(ComponentLuaState, TCHAR_TO_UTF8(*Key));
            }
        }
    }
}

bool ULuaScriptComponent::InitializeLuaEnvironment(FString & ErrorMessage)
{
    // Try to acquire a state from the pool
    ComponentLuaState = FLuaStateManager::Get().AcquireState(ErrorMessage);
    if (!ComponentLuaState)
    {
        UE_LOG(LogLuaScripting, Error, TEXT("Failed to acquire Lua state: %s"), *ErrorMessage);
        return false;
    }

    // Verify UE namespace exists
    lua_getglobal(ComponentLuaState, "UE");
    if (lua_isnil(ComponentLuaState, -1))
    {
        UE_LOG(LogLuaScripting, Warning, TEXT("UE namespace not found in Lua state, re-initializing bindings"));
        lua_pop(ComponentLuaState, 1);

        // Re-register all UE bindings
        FLuaBinding::RegisterCoreFunctions(ComponentLuaState);
        FLuaBinding::RegisterMathFunctions(ComponentLuaState);
        FLuaBinding::RegisterLogFunctions(ComponentLuaState);
        FLuaBinding::RegisterActorFunctions(ComponentLuaState);
    }
    else
    {
        // UE namespace exists, pop it
        lua_pop(ComponentLuaState, 1);
    }

    // Add the component's owner (actor) as a global
    if (GetOwner())
    {
        FLuaBinding::SetGlobalUObject(ComponentLuaState, "self", GetOwner());
    }

    // Add the component as a global
    FLuaBinding::SetGlobalUObject(ComponentLuaState, "component", this);

    // Reset GC counter
    GCCounter = 0;

    return true;
}

void ULuaScriptComponent::CleanupLuaEnvironment()
{
    if (ComponentLuaState)
    {
        // Release the state back to the pool
        FLuaStateManager::Get().ReleaseState(ComponentLuaState);
        ComponentLuaState = nullptr;
    }

    bScriptInitialized = false;
}
