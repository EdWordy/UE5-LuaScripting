#include "LuaScripting.h"
#include "LuaStateManager.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FLuaScripting"

void FLuaScripting::StartupModule()
{
    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

    // Get the base directory of this plugin
    FString BaseDir = IPluginManager::Get().FindPlugin("LuaScripting")->GetBaseDir();

    // Add on the relative location of the third party dll and load it
    FString LuaLibraryPath;
#if PLATFORM_WINDOWS
    LuaLibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/lua/lib/Win64/lua54.dll"));
#elif PLATFORM_MAC
    LuaLibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/lua/lib/Mac/liblua54.dylib"));
#elif PLATFORM_LINUX
    LuaLibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/lua/lib/Linux/liblua54.so"));
#endif

    LuaLibraryHandle = !LuaLibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LuaLibraryPath) : nullptr;

    if (LuaLibraryHandle)
    {
        // Initialize the Lua state manager
        FLuaStateManager::Get().Initialize();
        UE_LOG(LogTemp, Log, TEXT("LuaScripting plugin loaded successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Lua library at path: %s"), *LuaLibraryPath);
    }
}

void FLuaScripting::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.
    // For modules that support dynamic reloading, we call this function before unloading the module.

    // Clean up Lua state
    FLuaStateManager::Get().Shutdown();

    // Free the dll handle
    FPlatformProcess::FreeDllHandle(LuaLibraryHandle);
    LuaLibraryHandle = nullptr;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLuaScripting, LuaScripting)