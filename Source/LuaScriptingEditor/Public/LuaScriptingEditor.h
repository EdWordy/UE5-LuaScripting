#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IAssetTypeActions.h"

/**
 * Module for editor-specific functionality for the Lua scripting plugin
 */
class FLuaScriptingEditor : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /**
     * Registers asset type actions for Lua script assets
     */
    void RegisterAssetTypeActions();

    /**
     * Unregisters asset type actions for Lua script assets
     */
    void UnregisterAssetTypeActions();

private:
    /** Asset type actions created by this module */
    TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
};