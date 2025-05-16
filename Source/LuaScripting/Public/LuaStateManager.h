#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"

// Forward declarations for Lua
struct lua_State;

// Define a proper logging category
DECLARE_LOG_CATEGORY_EXTERN(LogLuaScripting, Log, All);

/**
 * Manager class for Lua states in Unreal Engine
 * Handles creation, management, and destruction of Lua states
 */
class LUASCRIPTING_API FLuaStateManager
{
public:
    // Private constructor to enforce singleton pattern
    FLuaStateManager();
    ~FLuaStateManager();

    /**
     * Singleton-like access to the state manager
     * @return Reference to the singleton instance
     */
    static FLuaStateManager& Get();

    /**
     * Initialize the Lua state manager and create default state
     * @return True if initialization succeeds
     */
    bool Initialize();

    /**
     * Shut down the Lua state manager and destroy all states
     */
    void Shutdown();

    /**
     * Execute a Lua script from a string
     * @param ScriptString The Lua script to execute
     * @param ErrorMessage Error message if execution fails
     * @return True if script executed successfully
     */
    bool ExecuteString(const FString& ScriptString, FString& ErrorMessage);

    /**
     * Execute a Lua script from a file
     * @param FilePath Path to the Lua script file
     * @param ErrorMessage Error message if execution fails
     * @return True if script executed successfully
     */
    bool ExecuteFile(const FString& FilePath, FString& ErrorMessage);

    /**
     * Get the main Lua state
     * @return Pointer to the main Lua state
     */
    lua_State* GetLuaState();

    /**
     * Acquire a Lua state from the pool
     * @param ErrorMessage Error message if acquisition fails
     * @return Pointer to a Lua state or nullptr if error
     */
    lua_State* AcquireState(FString& ErrorMessage);

    /**
     * Release a Lua state back to the pool
     * @param State The Lua state to release
     */
    void ReleaseState(lua_State* State);

    /**
     * Configure Lua garbage collection
     * @param State The Lua state to configure
     */
    void ConfigureGarbageCollection(lua_State* State);

    /**
     * Run a step of garbage collection
     * @param State The Lua state to run GC on
     */
    void RunGarbageCollection(lua_State* State);

private:
    // Disallow copying and assignment
    FLuaStateManager(const FLuaStateManager&) = delete;
    FLuaStateManager& operator=(const FLuaStateManager&) = delete;

    /**
     * Set up standard Lua libraries and UE-specific functions
     * @param State The Lua state to set up
     */
    void SetupLuaState(lua_State* State);

    /**
     * Helper function to handle Lua errors
     * @param State The Lua state where the error occurred
     * @param ErrorMessage Output parameter for the error message
     * @return False to indicate an error occurred
     */
    bool HandleLuaError(lua_State* State, FString& ErrorMessage);

    /**
     * Error handler function for Lua errors with stack trace
     * @param State The Lua state
     * @return 1 (the error message with stack trace)
     */
    static int LuaErrorHandler(lua_State* State);

private:
    // Main Lua state
    lua_State* MainLuaState;

    // Pool of Lua states for script components
    TArray<lua_State*> StatePool;

    // Critical section for thread safety
    FCriticalSection StateLock;

    // Flag to track initialization state
    bool bIsInitialized;

    // Maximum number of states to keep in the pool
    static constexpr int32 MaxPoolSize = 10;
};