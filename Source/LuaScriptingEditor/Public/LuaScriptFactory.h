#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "LuaScriptFactory.generated.h"

/**
 * Factory for creating Lua script assets
 */
UCLASS()
class LUASCRIPTINGEDITOR_API ULuaScriptFactory : public UFactory
{
    GENERATED_BODY()

public:
    ULuaScriptFactory();

    // UFactory interface
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    virtual UObject* FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) override;
    virtual bool CanCreateNew() const override;
    virtual bool ShouldShowInNewMenu() const override;
    virtual bool FactoryCanImport(const FString& Filename) override;
    virtual FText GetDisplayName() const override;
    // End of UFactory interface
};