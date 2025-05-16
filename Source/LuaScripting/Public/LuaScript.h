#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LuaScript.generated.h"

/**
 * Asset type for Lua scripts in Unreal Engine
 */
UCLASS(BlueprintType)
class LUASCRIPTING_API ULuaScript : public UObject
{
    GENERATED_BODY()

public:
    ULuaScript();

    /**
     * The Lua script content
     */
    UPROPERTY(EditAnywhere, Category = "Script")
    FString ScriptContent;

    /**
     * Execute this Lua script
     * @param ErrorMessage Error message if execution fails
     * @return True if script executed successfully
     */
    UFUNCTION(BlueprintCallable, Category = "Lua")
    bool Execute(FString& ErrorMessage);

#if WITH_EDITOR
    /** Called when the asset is imported or reimported */
    virtual void PostInitProperties() override;

    /** Called when a property is changed in the editor */
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    /** Called when the asset is loaded */
    virtual void Serialize(FArchive& Ar) override;
};