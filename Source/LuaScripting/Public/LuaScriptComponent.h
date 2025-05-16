#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LuaScript.h"
#include "LuaScriptComponent.generated.h"

/**
 * Component that allows an actor to run Lua scripts
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LUASCRIPTING_API ULuaScriptComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULuaScriptComponent();

    /** Script asset to run */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
    ULuaScript* ScriptAsset;

    /** Raw script content (alternative to script asset) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua", meta = (MultiLine = true))
    FString ScriptContent;

    /** Should this script run automatically on BeginPlay */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
    bool bAutoRun;

    /** Should this script call tick() function every frame */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua")
    bool bCallTickFunction;

    /** Garbage collection frequency (how many frames between GC steps) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lua|Advanced", meta = (ClampMin = "1", UIMin = "1"))
    int32 GCInterval;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    /**
     * Execute the component's script
     * @param ErrorMessage Error message if execution fails
     * @return True if script executed successfully
     */
    UFUNCTION(BlueprintCallable, Category = "Lua")
    bool ExecuteScript(FString& ErrorMessage);

    /**
     * Call a specific function in the script
     * @param FunctionName Name of the function to call
     * @param ErrorMessage Error message if execution fails
     * @return True if function called successfully
     */
    UFUNCTION(BlueprintCallable, Category = "Lua")
    bool CallFunction(const FString& FunctionName, FString& ErrorMessage);

    /**
     * Hot reload the script (useful during development)
     * @param ErrorMessage Error message if reload fails
     * @return True if script reloaded successfully
     */
    UFUNCTION(BlueprintCallable, Category = "Lua|Development")
    bool HotReloadScript(FString& ErrorMessage);

private:
    /** State to track if the script has been initialized */
    bool bScriptInitialized;

    /** The Lua state for this component */
    struct lua_State* ComponentLuaState;

    /** Frame counter for garbage collection */
    int32 GCCounter;

    /** Initialize the Lua environment for this component */
    bool InitializeLuaEnvironment(FString& ErrorMessage);

    /** Cleanup the Lua environment for this component */
    void CleanupLuaEnvironment();

    /** Determine script content to execute */
    FString DetermineScriptContent() const;

    /** Load and execute the script content */
    bool LoadAndExecuteScript(const FString& ContentToExecute, FString& ErrorMessage);

    /** Create a snapshot of script global state for hot reloading */
    FString PreserveScriptState() const;

    /** Restore script state after hot reloading */
    void RestoreScriptState(const FString& StateVars);
};