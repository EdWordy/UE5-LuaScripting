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

// Define log category
DEFINE_LOG_CATEGORY(LogLuaScripting);

// Static instance for singleton pattern
static TSharedPtr<FLuaStateManager, ESPMode::ThreadSafe> LuaStateManagerInstance;

FLuaStateManager& FLuaStateManager::Get()
{
    if (!LuaStateManagerInstance.IsValid())
    {
        LuaStateManagerInstance = MakeShareable(new FLuaStateManager());
    }
    return *LuaStateManagerInstance.Get();
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
        UE_LOG(LogLuaScripting, Error, TEXT("Failed to create Lua state"));
        return false;
    }

    // Set up the Lua state with standard libraries and UE-specific functions
    SetupLuaState(MainLuaState);

    // Configure garbage collection
    ConfigureGarbageCollection(MainLuaState);

    bIsInitialized = true;
    UE_LOG(LogLuaScripting, Log, TEXT("Lua state manager initialized successfully"));
    return true;
}

void FLuaStateManager::Shutdown()
{
    FScopeLock Lock(&StateLock);

    // Clean up the main state
    if (MainLuaState)
    {
        lua_close(MainLuaState);
        MainLuaState = nullptr;
    }

    // Clean up the state pool
    for (lua_State* State : StatePool)
    {
        if (State)
        {
            lua_close(State);
        }
    }
    StatePool.Empty();

    bIsInitialized = false;
    UE_LOG(LogLuaScripting, Log, TEXT("Lua state manager shut down"));
}

lua_State* FLuaStateManager::GetLuaState()
{
    FScopeLock Lock(&StateLock);

    if (!MainLuaState)
    {
        if (!Initialize())
        {
            UE_LOG(LogLuaScripting, Error, TEXT("Failed to initialize Lua state in GetLuaState"));
            return nullptr;
        }
    }

    return MainLuaState;
}

lua_State* FLuaStateManager::AcquireState(FString& ErrorMessage)
{
    FScopeLock Lock(&StateLock);

    // Check for a state in the pool
    if (StatePool.Num() > 0)
    {
        lua_State* State = StatePool.Pop(EAllowShrinking::Yes);

        // Important: Re-setup the state to ensure all bindings are fresh
        SetupLuaState(State);

        // Run garbage collection to clean up any leftover data
        lua_gc(State, LUA_GCCOLLECT, 0);
        return State;
    }

    // Create a new state
    lua_State* NewState = luaL_newstate();
    if (!NewState)
    {
        ErrorMessage = TEXT("Failed to create new Lua state");
        return nullptr;
    }

    // Set up the state
    SetupLuaState(NewState);
    ConfigureGarbageCollection(NewState);

    return NewState;
}

void FLuaStateManager::ReleaseState(lua_State* State)
{
    if (!State)
    {
        return;
    }

    FScopeLock Lock(&StateLock);

    // Check if we should keep it in the pool
    if (StatePool.Num() < MaxPoolSize)
    {
        // For Lua 5.4, we need a more thorough reset approach
        // Rather than trying to clear everything individually, which is prone to errors,
        // we'll run a reset script that clears the environment

        // Clear the global table while preserving core Lua functionality
        static const char* resetScript = R"(
            -- Store a reference to core functions we want to keep
            local _keep = {
                assert = assert,
                collectgarbage = collectgarbage,
                error = error,
                getmetatable = getmetatable,
                ipairs = ipairs,
                load = load,
                next = next,
                pairs = pairs,
                pcall = pcall,
                print = print,
                rawequal = rawequal,
                rawget = rawget,
                rawlen = rawlen,
                rawset = rawset,
                select = select,
                setmetatable = setmetatable,
                tonumber = tonumber,
                tostring = tostring,
                type = type,
                xpcall = xpcall,
                -- Tables to keep
                string = string,
                table = table,
                math = math,
                coroutine = coroutine,
                os = os,
                package = package,
                debug = debug,
                -- Core variables
                _VERSION = _VERSION,
                _G = _G
            }
            
            -- Clear all globals except those in _keep
            for k in pairs(_G) do
                if not _keep[k] then
                    _G[k] = nil
                end
            end
        )";

        // Execute the reset script
        if (luaL_dostring(State, resetScript) != LUA_OK)
        {
            // If reset fails, log and close the state instead of reusing it
            const char* ErrorMsg = lua_tostring(State, -1);
            UE_LOG(LogLuaScripting, Warning, TEXT("Failed to reset Lua state: %s"), UTF8_TO_TCHAR(ErrorMsg));
            lua_pop(State, 1);

            lua_close(State);
            return;
        }

        // Run garbage collection
        lua_gc(State, LUA_GCCOLLECT, 0);

        // Add to pool
        StatePool.Add(State);
    }
    else
    {
        // Just close it
        lua_close(State);
    }
}

void FLuaStateManager::ConfigureGarbageCollection(lua_State* State)
{
    if (!State)
    {
        return;
    }

    // Set up incremental GC (Lua 5.4+)
    lua_gc(State, LUA_GCGEN, 0, 0);

    // Configure GC parameters
    lua_gc(State, LUA_GCSETPAUSE, 150);    // 150% pause between cycles
    lua_gc(State, LUA_GCSETSTEPMUL, 200);  // 200% speed relative to allocation
}

void FLuaStateManager::RunGarbageCollection(lua_State* State)
{
    if (!State)
    {
        return;
    }

    // Run a step of garbage collection
    lua_gc(State, LUA_GCSTEP, 10);  // 10 "kilobytes" of work
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

int FLuaStateManager::LuaErrorHandler(lua_State* State)
{
    // Get the error message
    const char* ErrorMsg = lua_tostring(State, -1);

    // Generate a stack trace
    luaL_traceback(State, State, ErrorMsg, 1);

    return 1;  // Return the stack trace as the error message
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

    // Push error handler function
    lua_pushcfunction(MainLuaState, LuaErrorHandler);
    int ErrorHandlerIndex = lua_gettop(MainLuaState);

    // Load the string as Lua code
    int Status = luaL_loadstring(MainLuaState, Script);
    if (Status != LUA_OK)
    {
        return HandleLuaError(MainLuaState, ErrorMessage);
    }

    // Execute the loaded code with error handler
    Status = lua_pcall(MainLuaState, 0, LUA_MULTRET, ErrorHandlerIndex);

    // Remove the error handler
    lua_remove(MainLuaState, ErrorHandlerIndex);

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

    UE_LOG(LogLuaScripting, Error, TEXT("Lua error: %s"), *ErrorMessage);
    return false;
}
