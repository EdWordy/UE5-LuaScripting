#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "LuaScript.h"

class LUASCRIPTINGEDITOR_API FLuaScriptEditor : public FAssetEditorToolkit
{
public:
    // IToolkit interface
    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual FString GetWorldCentricTabPrefix() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;
    // End of IToolkit interface

    // Initialize the editor for a specific Lua script asset
    void Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, ULuaScript* InLuaScript);

private:
    // Create and register a tab for editing the Lua script
    TSharedRef<SDockTab> SpawnTab_ScriptEditor(const FSpawnTabArgs& Args);

    // Create the widget for editing the Lua script
    TSharedRef<SWidget> CreateScriptEditorWidget();

    // Handler for when the script content is changed in the editor
    void OnScriptTextChanged(const FText& NewText);

    // Handler for when the script content is committed in the editor
    void OnScriptTextCommitted(const FText& NewText, ETextCommit::Type CommitType);

    // Execute the current script for testing
    void ExecuteScript();

private:
    // The Lua script being edited
    ULuaScript* LuaScript;

    // The text editor widget
    TSharedPtr<class SMultiLineEditableTextBox> ScriptEditorWidget;

    // Unique ID for the script editor tab
    static const FName ScriptEditorTabId;
};