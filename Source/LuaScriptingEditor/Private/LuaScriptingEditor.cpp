#include "LuaScriptingEditor.h"
#include "LuaScriptEditor.h"
#include "LuaScript.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetTypeActions_Base.h"

class FLuaScriptAssetTypeActions : public FAssetTypeActions_Base
{
public:
    // IAssetTypeActions interface
    virtual FText GetName() const override { return FText::FromString(TEXT("Lua Script")); }
    virtual FColor GetTypeColor() const override { return FColor(111, 51, 222); } // Purple color for Lua
    virtual uint32 GetCategories() override { return EAssetTypeCategories::Misc; }
    virtual UClass* GetSupportedClass() const override { return ULuaScript::StaticClass(); }
    virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
    // End of IAssetTypeActions interface

    // Opens the Lua script editor
    virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override
    {
        for (UObject* Object : InObjects)
        {
            ULuaScript* LuaScript = Cast<ULuaScript>(Object);
            if (LuaScript)
            {
                TSharedRef<FLuaScriptEditor> NewLuaEditor(new FLuaScriptEditor());
                NewLuaEditor->Initialize(EToolkitMode::Standalone, EditWithinLevelEditor, LuaScript);
            }
        }
    }
};

void FLuaScriptingEditor::StartupModule()
{
    // Register asset type actions
    RegisterAssetTypeActions();
}

void FLuaScriptingEditor::ShutdownModule()
{
    // Unregister asset type actions
    UnregisterAssetTypeActions();
}

void FLuaScriptingEditor::RegisterAssetTypeActions()
{
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    // Register Lua script asset type actions
    TSharedRef<IAssetTypeActions> LuaScriptAssetTypeActions = MakeShareable(new FLuaScriptAssetTypeActions);
    AssetTools.RegisterAssetTypeActions(LuaScriptAssetTypeActions);
    RegisteredAssetTypeActions.Add(LuaScriptAssetTypeActions);
}

void FLuaScriptingEditor::UnregisterAssetTypeActions()
{
    if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
    {
        IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

        // Unregister all registered asset type actions
        for (auto TypeAction : RegisteredAssetTypeActions)
        {
            AssetTools.UnregisterAssetTypeActions(TypeAction.ToSharedRef());
        }
    }

    RegisteredAssetTypeActions.Empty();
}

IMPLEMENT_MODULE(FLuaScriptingEditor, LuaScriptingEditor)