#include "LuaScript.h"
#include "LuaStateManager.h"
#include "Misc/FileHelper.h"

ULuaScript::ULuaScript()
{
}

bool ULuaScript::Execute(FString& ErrorMessage)
{
    if (ScriptContent.IsEmpty())
    {
        ErrorMessage = TEXT("Script is empty");
        return false;
    }

    return FLuaStateManager::Get().ExecuteString(ScriptContent, ErrorMessage);
}

#if WITH_EDITOR
void ULuaScript::PostInitProperties()
{
    Super::PostInitProperties();

    // Handle initial asset creation
    if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
    {
        // We can initialize with a default template here if needed
    }
}

void ULuaScript::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Handle property changes in the editor
    FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(ULuaScript, ScriptContent))
    {
        // Script content changed in editor
        // We could do syntax validation here
    }
}
#endif

void ULuaScript::Serialize(FArchive& Ar)
{
    Super::Serialize(Ar);

    // Add version checks if needed for future compatibility
}
