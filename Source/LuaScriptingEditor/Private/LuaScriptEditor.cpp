#include "LuaScriptEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "EditorStyleSet.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "LuaStateManager.h"

#define LOCTEXT_NAMESPACE "LuaScriptEditor"

const FName FLuaScriptEditor::ScriptEditorTabId(TEXT("LuaScriptEditor_ScriptEditor"));

void FLuaScriptEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    // Register the script editor tab spawner
    InTabManager->RegisterTabSpawner(ScriptEditorTabId, FOnSpawnTab::CreateSP(this, &FLuaScriptEditor::SpawnTab_ScriptEditor))
        .SetDisplayName(LOCTEXT("ScriptEditorTab", "Script"))
        .SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FLuaScriptEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    InTabManager->UnregisterTabSpawner(ScriptEditorTabId);
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

FName FLuaScriptEditor::GetToolkitFName() const
{
    return FName("LuaScriptEditor");
}

FText FLuaScriptEditor::GetBaseToolkitName() const
{
    return LOCTEXT("AppLabel", "Lua Script Editor");
}

FString FLuaScriptEditor::GetWorldCentricTabPrefix() const
{
    return LOCTEXT("WorldCentricTabPrefix", "Lua Script ").ToString();
}

FLinearColor FLuaScriptEditor::GetWorldCentricTabColorScale() const
{
    return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f); // Purple hue for Lua scripts
}

void FLuaScriptEditor::Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, ULuaScript* InLuaScript)
{
    LuaScript = InLuaScript;

    // Create the layout for the editor
    const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_LuaScriptEditor_Layout_v1")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
            ->Split
            (
                FTabManager::NewStack()
                ->AddTab(ScriptEditorTabId, ETabState::OpenedTab)
                ->SetHideTabWell(false)
            )
        );

    // Initialize the asset editor
    InitAssetEditor(Mode, InitToolkitHost, FName("LuaScriptEditorApp"), StandaloneDefaultLayout, /*bCreateDefaultStandaloneMenu=*/true, /*bCreateDefaultToolbar=*/true, InLuaScript);

    // Add toolbar extension
    TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
    ToolbarExtender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        GetToolkitCommands(),
        FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder)
            {
                ToolbarBuilder.BeginSection("Script");
                {
                    ToolbarBuilder.AddToolBarButton(
                        FUIAction(FExecuteAction::CreateSP(this, &FLuaScriptEditor::ExecuteScript)),
                        NAME_None,
                        LOCTEXT("ExecuteScript", "Execute"),
                        LOCTEXT("ExecuteScriptTooltip", "Execute the current Lua script"),
                        FSlateIcon(FAppStyle::GetAppStyleSetName(), "PlayWorld.PlayInViewport")
                    );
                }
                ToolbarBuilder.EndSection();
            })
    );

    AddToolbarExtender(ToolbarExtender);

    // Refresh the asset editor
    RegenerateMenusAndToolbars();
}

TSharedRef<SDockTab> FLuaScriptEditor::SpawnTab_ScriptEditor(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .Label(LOCTEXT("ScriptEditorTitle", "Script"))
        [
            CreateScriptEditorWidget()
        ];
}

TSharedRef<SWidget> FLuaScriptEditor::CreateScriptEditorWidget()
{
    // Create the text editor
    ScriptEditorWidget = SNew(SMultiLineEditableTextBox)
        .Text(FText::FromString(LuaScript->ScriptContent))
        .OnTextChanged(this, &FLuaScriptEditor::OnScriptTextChanged)
        .OnTextCommitted(this, &FLuaScriptEditor::OnScriptTextCommitted)
        .AutoWrapText(false)
        .ModiferKeyForNewLine(EModifierKey::Shift)
        .HintText(LOCTEXT("EditScriptHint", "Enter Lua script..."));

    return ScriptEditorWidget.ToSharedRef();
}

void FLuaScriptEditor::OnScriptTextChanged(const FText& NewText)
{
    // Update the script content
    LuaScript->ScriptContent = NewText.ToString();

    // Mark the asset as dirty
    LuaScript->MarkPackageDirty();
}

void FLuaScriptEditor::OnScriptTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
    // Update the script content
    LuaScript->ScriptContent = NewText.ToString();

    // Mark the asset as dirty
    LuaScript->MarkPackageDirty();
}

void FLuaScriptEditor::ExecuteScript()
{
    // Execute the script
    FString ErrorMessage;
    bool Success = LuaScript->Execute(ErrorMessage);

    // Show the result
    if (Success)
    {
        FNotificationInfo Info(LOCTEXT("ScriptExecuted", "Lua script executed successfully"));
        Info.FadeInDuration = 0.2f;
        Info.FadeOutDuration = 1.0f;
        Info.ExpireDuration = 4.0f;
        Info.bUseSuccessFailIcons = true;
        Info.bUseLargeFont = false;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
    else
    {
        FNotificationInfo Info(FText::Format(LOCTEXT("ScriptError", "Lua script error: {0}"), FText::FromString(ErrorMessage)));
        Info.FadeInDuration = 0.2f;
        Info.FadeOutDuration = 1.0f;
        Info.ExpireDuration = 6.0f;
        Info.bUseSuccessFailIcons = true;
        Info.bUseLargeFont = false;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
}

#undef LOCTEXT_NAMESPACEESPACE