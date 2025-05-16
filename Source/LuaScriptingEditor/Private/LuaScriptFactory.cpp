#include "LuaScriptFactory.h"
#include "LuaScript.h"
#include "Misc/FileHelper.h"

ULuaScriptFactory::ULuaScriptFactory()
{
    // Set factory properties
    bCreateNew = true;
    bEditAfterNew = true;
    bEditorImport = true;
    SupportedClass = ULuaScript::StaticClass();

    // Add supported formats for importing
    Formats.Add(TEXT("lua;Lua Script"));
    Formats.Add(TEXT("txt;Text File"));
}

UObject* ULuaScriptFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    // Create a new Lua script asset
    ULuaScript* NewScript = NewObject<ULuaScript>(InParent, InClass, InName, Flags);
    return NewScript;
}

UObject* ULuaScriptFactory::FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn)
{
    // Convert the binary buffer to a string
    FString ScriptContent;
    int32 Size = BufferEnd - Buffer;
    
    // Check if the file is UTF-8, UTF-16 LE, or UTF-16 BE
    const uint8* BinaryBuffer = Buffer;
    if (Size >= 2)
    {
        // Check for UTF-16 LE BOM (FF FE)
        if (BinaryBuffer[0] == 0xFF && BinaryBuffer[1] == 0xFE)
        {
            // UTF-16 LE
            const UTF16CHAR* Src = reinterpret_cast<const UTF16CHAR*>(BinaryBuffer + 2);
            ScriptContent = FString(Size / 2 - 1, Src);
        }
        // Check for UTF-16 BE BOM (FE FF)
        else if (BinaryBuffer[0] == 0xFE && BinaryBuffer[1] == 0xFF)
        {
            // UTF-16 BE - we need to swap bytes
            const UTF16CHAR* Src = reinterpret_cast<const UTF16CHAR*>(BinaryBuffer + 2);
            ScriptContent = FString(Size / 2 - 1, Src);
            // Swap bytes logic would go here if needed
        }
        else if (Size >= 3 && BinaryBuffer[0] == 0xEF && BinaryBuffer[1] == 0xBB && BinaryBuffer[2] == 0xBF)
        {
            // UTF-8 with BOM
            ScriptContent = UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(BinaryBuffer + 3));
        }
        else
        {
            // Assume UTF-8 without BOM
            ScriptContent = UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(BinaryBuffer));
        }
    }
    else
    {
        // Just treat as ASCII if it's too short to have a BOM
        ScriptContent = UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(BinaryBuffer));
    }

    // Create a new Lua script asset and set its content
    ULuaScript* NewScript = NewObject<ULuaScript>(InParent, InClass, InName, Flags);
    NewScript->ScriptContent = ScriptContent;
    return NewScript;
}

bool ULuaScriptFactory::CanCreateNew() const
{
    return true;
}

bool ULuaScriptFactory::ShouldShowInNewMenu() const
{
    return true;
}

bool ULuaScriptFactory::FactoryCanImport(const FString& Filename)
{
    const FString Extension = FPaths::GetExtension(Filename).ToLower();
    return Extension == TEXT("lua") || Extension == TEXT("txt");
}

FText ULuaScriptFactory::GetDisplayName() const
{
    return FText::FromString(TEXT("Lua Script"));
}