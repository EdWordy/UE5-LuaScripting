#include "LuaStateManager.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "LuaBinding.h"

// Include Lua headers
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

// Static instance for singleton pattern
static FLuaStateManager* LuaStateManagerInstance = nullptr;

FLuaStateManager& FLuaStateManager::Get()
{
    if (!LuaStateManagerInstance)
    {
        LuaStateManagerInstance = new FLuaStateManager();
    }
    return *LuaStateManagerInstance;
}

FLuaStateManager::FLuaStateManager()
    : MainLuaState(nullptr)
    , bIsInitialized(false)
{
}

FLuaStateManager::~FLuaStateManager()
{
    Shutdown();
}

bool FLuaStateManager::Initialize()
{
    FScopeLock Lock(&StateLock);

    if (bIsInitialized)
    {
        return true;
    }

    // Create a new Lua state
    MainLuaState = luaL_newstate();
    if (!MainLuaState)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create Lua state"));
        return false;
    }

    // Set up the Lua state with standard libraries and UE-specific functions
    SetupLuaState(MainLuaState);

    bIsInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("Lua state manager initialized successfully"));
    return true;
}

void FLuaStateManager::Shutdown()
{
    FScopeLock Lock(&StateLock);

    if (MainLuaState)
    {
        lua_close(MainLuaState);
        MainLuaState = nullptr;
    }

    bIsInitialized = false;
    UE_LOG(LogTemp, Log, TEXT("Lua state manager shut down"));
}

void FLuaStateManager::SetupLuaState(lua_State* State)
{
    // Open standard Lua libraries
    luaL_openlibs(State);

    // Register UE-specific functions
    FLuaBinding::RegisterCoreFunctions(State);
    FLuaBinding::RegisterMathFunctions(State);
    FLuaBinding::RegisterLogFunctions(State);
    FLuaBinding::RegisterActorFunctions(State);
}

bool FLuaStateManager::ExecuteString(const FString& ScriptString, FString& ErrorMessage)
{
    FScopeLock Lock(&StateLock);

    if (!bIsInitialized || !MainLuaState)
    {
        ErrorMessage = TEXT("Lua state not initialized");
        return false;
    }

    // Convert FString to ANSI string
    const char* Script = TCHAR_TO_UTF8(*ScriptString);

    // Load the string as Lua code
    int Status = luaL_loadstring(MainLuaState, Script);
    if (Status != LUA_OK)
    {
        return HandleLuaError(MainLuaState, ErrorMessage);
    }

    // Execute the loaded code
    Status = lua_pcall(MainLuaState, 0, LUA_MULTRET, 0);
    if (Status != LUA_OK)
    {
        return HandleLuaError(MainLuaState, ErrorMessage);
    }

    return true;
}

bool FLuaStateManager::ExecuteFile(const FString& FilePath, FString& ErrorMessage)
{
    // Check if file exists
    if (!FPaths::FileExists(FilePath))
    {
        ErrorMessage = FString::Printf(TEXT("File not found: %s"), *FilePath);
        return false;
    }

    // Read file contents
    FString ScriptContent;
    if (!FFileHelper::LoadFileToString(ScriptContent, *FilePath))
    {
        ErrorMessage = FString::Printf(TEXT("Failed to read file: %s"), *FilePath);
        return false;
    }

    // Execute the file content as a string
    return ExecuteString(ScriptContent, ErrorMessage);
}

bool FLuaStateManager::HandleLuaError(lua_State* State, FString& ErrorMessage)
{
    // Get error message from the top of the stack
    const char* ErrorCStr = lua_tostring(State, -1);
    ErrorMessage = FString(UTF8_TO_TCHAR(ErrorCStr));

    // Pop the error message from the stack
    lua_pop(State, 1);

    UE_LOG(LogTemp, Error, TEXT("Lua error: %s"), *ErrorMessage);
    return false;
}