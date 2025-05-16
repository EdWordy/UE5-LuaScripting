# Lua Scripting Plugin API Documentation

This document provides detailed API documentation for the Lua Scripting Plugin for Unreal Engine 5.

## Table of Contents
- [Accessing UE from Lua](#accessing-ue-from-lua)
- [Core API](#core-api)
- [Math API](#math-api)
- [Logging API](#logging-api)
- [Actor API](#actor-api)
- [Global Variables](#global-variables)
- [Lifecycle Functions](#lifecycle-functions)
- [UObject Property Access](#uobject-property-access)
- [C++ Integration API](#c-integration-api)

---

## Accessing UE from Lua

All Unreal Engine functionality is accessible through the `UE` namespace in Lua. This keeps the global namespace clean and makes it clear which functions are provided by the engine.

```lua
-- Example of using the UE namespace
UE.Print("Hello from Lua!")
local position = UE.Math.Vector(100, 200, 300)
```

---

## Core API

### UE.Print(message)
Prints a message to the Unreal Engine log and on-screen display.

**Parameters:**
- `message` (string): The message to print

**Example:**
```lua
UE.Print("Hello from Lua!")
UE.Print("Current value: " .. someVariable)
```

### UE.GetDeltaTime()
Gets the time in seconds since the last frame.

**Returns:**
- (number): Time in seconds

**Example:**
```lua
local dt = UE.GetDeltaTime()
UE.Print("Frame time: " .. dt .. " seconds")
```

### UE.GetTotalElapsedTime()
Gets the total time in seconds since the game started.

**Returns:**
- (number): Total time in seconds

**Example:**
```lua
local totalTime = UE.GetTotalElapsedTime()
UE.Print("Game has been running for " .. totalTime .. " seconds")
```

---

## Math API

### UE.Math.Vector(x, y, z)
Creates a 3D vector.

**Parameters:**
- `x` (number): X coordinate
- `y` (number): Y coordinate
- `z` (number): Z coordinate

**Returns:**
- (table): Vector object with X, Y, Z properties

**Example:**
```lua
local position = UE.Math.Vector(100, 200, 300)
UE.Print("Position: " .. position.X .. ", " .. position.Y .. ", " .. position.Z)
```

### UE.Math.Rotation(pitch, yaw, roll)
Creates a rotation.

**Parameters:**
- `pitch` (number): Pitch in degrees
- `yaw` (number): Yaw in degrees
- `roll` (number): Roll in degrees

**Returns:**
- (table): Rotation object with Pitch, Yaw, Roll properties

**Example:**
```lua
local rotation = UE.Math.Rotation(0, 90, 0)
UE.Print("Rotation: " .. rotation.Pitch .. ", " .. rotation.Yaw .. ", " .. rotation.Roll)
```

---

## Logging API

### UE.Log.Trace(message)
Logs a trace message to the Unreal Engine log.

**Parameters:**
- `message` (string): The message to log

**Example:**
```lua
UE.Log.Trace("Detailed information for debugging")
```

### UE.Log.Warning(message)
Logs a warning message to the Unreal Engine log with yellow color.

**Parameters:**
- `message` (string): The warning message to log

**Example:**
```lua
UE.Log.Warning("Something unusual happened")
```

### UE.Log.Error(message)
Logs an error message to the Unreal Engine log with red color.

**Parameters:**
- `message` (string): The error message to log

**Example:**
```lua
UE.Log.Error("Something went wrong!")
```

---

## Actor API

### UE.Actor.Find(name)
Finds an actor in the world by name.

**Parameters:**
- `name` (string): The name of the actor to find

**Returns:**
- (userdata): The actor if found, nil otherwise

**Example:**
```lua
local player = UE.Actor.Find("PlayerPawn")
if player then
    UE.Print("Found player at position: " .. player.ActorLocation.X .. ", " .. player.ActorLocation.Y .. ", " .. player.ActorLocation.Z)
end
```

### UE.Actor.Spawn(className, location, rotation)
Spawns a new actor in the world.

**Parameters:**
- `className` (string): The class name of the actor to spawn (e.g., "Blueprint'/Game/Blueprints/MyActor.MyActor_C'")
- `location` (table, optional): Vector position where to spawn the actor
- `rotation` (table, optional): Rotation to apply to the spawned actor

**Returns:**
- (userdata): The spawned actor if successful, nil otherwise

**Example:**
```lua
local location = UE.Math.Vector(100, 200, 300)
local rotation = UE.Math.Rotation(0, 90, 0)
local actor = UE.Actor.Spawn("Blueprint'/Game/Blueprints/Cube.Cube_C'", location, rotation)

if actor then
    UE.Print("Actor spawned successfully")
end
```

### UE.Actor.Destroy(actor)
Destroys an actor in the world.

**Parameters:**
- `actor` (userdata): The actor to destroy

**Returns:**
- (boolean): True if successful

**Example:**
```lua
local actor = UE.Actor.Find("Temp_Cube")
if actor then
    local success = UE.Actor.Destroy(actor)
    UE.Print("Actor destroyed: " .. tostring(success))
end
```

---

## Global Variables

### self
Reference to the actor that owns the script component. Only available in script components, not in standalone scripts.

**Example:**
```lua
if self then
    UE.Print("My name is: " .. self.ActorLabel)
    self.ActorLocation = UE.Math.Vector(100, 200, 300)
end
```

### component
Reference to the Lua script component. Only available in script components, not in standalone scripts.

**Example:**
```lua
if component then
    UE.Print("Component name: " .. component.GetName())
end
```

---

## Lifecycle Functions

These functions are automatically called by the engine if defined in your script.

### init()
Called when the script is first loaded/initialized. Ideal for setup code.

**Example:**
```lua
function init()
    UE.Print("Script initialized!")
    
    -- Store initial state
    startPosition = nil
    if self then
        startPosition = {
            X = self.ActorLocation.X,
            Y = self.ActorLocation.Y,
            Z = self.ActorLocation.Z
        }
    end
end
```

### tick(deltaTime)
Called every frame if the "Call Tick Function" option is enabled on the script component.

**Parameters:**
- `deltaTime` (number): Time in seconds since the last frame

**Example:**
```lua
function tick(deltaTime)
    if self then
        local currentRotation = self.ActorRotation
        local newYaw = currentRotation.Yaw + (45 * deltaTime) -- Rotate 45 degrees per second
        self.ActorRotation = UE.Math.Rotation(currentRotation.Pitch, newYaw, currentRotation.Roll)
    end
end
```

---

## UObject Property Access

You can access UObject properties directly using dot notation.

### Reading Properties

**Example:**
```lua
if self then
    local location = self.ActorLocation
    local rotation = self.ActorRotation
    local scale = self.ActorScale
    
    UE.Print("Actor position: " .. location.X .. ", " .. location.Y .. ", " .. location.Z)
end
```

### Writing Properties

**Example:**
```lua
if self then
    -- Set actor location
    self.ActorLocation = UE.Math.Vector(100, 200, 300)
    
    -- Set actor rotation
    self.ActorRotation = UE.Math.Rotation(0, 90, 0)
    
    -- Set a custom property (if defined on the actor)
    self.Health = 100
end
```

### Calling Functions

**Example:**
```lua
if self then
    -- Call a function on the actor
    self.Jump()
    
    -- Call a function with parameters (simplified syntax)
    self.SetActorHiddenInGame(true)
end
```

---

## C++ Integration API

The following C++ functions are available for integrating Lua scripts into your game code.

### Executing Scripts from C++

```cpp
#include "LuaStateManager.h"

// Execute a script from a string
bool ExecuteScriptFromString(const FString& ScriptContent)
{
    FString ErrorMessage;
    return FLuaStateManager::Get().ExecuteString(ScriptContent, ErrorMessage);
}

// Execute a script from a file
bool ExecuteScriptFromFile(const FString& FilePath)
{
    FString ErrorMessage;
    return FLuaStateManager::Get().ExecuteFile(FilePath, ErrorMessage);
}
```

### Calling Lua Functions from C++

```cpp
#include "LuaStateManager.h"

// Get a Lua function and call it
bool CallLuaFunction(const FString& FunctionName, float Parameter)
{
    lua_State* L = FLuaStateManager::Get().GetLuaState();
    
    // Get the function from the global table
    lua_getglobal(L, TCHAR_TO_UTF8(*FunctionName));
    
    if (!lua_isfunction(L, -1))
    {
        UE_LOG(LogTemp, Warning, TEXT("Lua function '%s' not found"), *FunctionName);
        lua_pop(L, 1);
        return false;
    }
    
    // Push the parameter
    lua_pushnumber(L, Parameter);
    
    // Call the function (1 parameter, 1 result)
    if (lua_pcall(L, 1, 1, 0) != LUA_OK)
    {
        FString ErrorMessage = UTF8_TO_TCHAR(lua_tostring(L, -1));
        UE_LOG(LogTemp, Error, TEXT("Error calling Lua function: %s"), *ErrorMessage);
        lua_pop(L, 1);
        return false;
    }
    
    // Get the result
    bool Result = lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    
    return Result;
}
```

### Using LuaScriptComponent from C++

```cpp
#include "LuaScriptComponent.h"

// Set script content and execute it
void SetupLuaComponent(AActor* Actor)
{
    // Add a Lua script component to an actor
    ULuaScriptComponent* LuaComponent = Cast<ULuaScriptComponent>(
        Actor->AddComponentByClass(ULuaScriptComponent::StaticClass(), false, FTransform::Identity, false)
    );
    
    if (LuaComponent)
    {
        // Set script content directly
        LuaComponent->ScriptContent = R"(
            function init()
                UE.Print("Hello from " .. self.ActorLabel)
            end
            
            function tick(deltaTime)
                local rot = self.ActorRotation
                rot.Yaw = rot.Yaw + 45 * deltaTime
                self.ActorRotation = rot
            end
        )";
        
        // Configure component
        LuaComponent->bAutoRun = true;
        LuaComponent->bCallTickFunction = true;
        
        // Execute script manually if needed
        FString ErrorMessage;
        LuaComponent->ExecuteScript(ErrorMessage);
    }
}
```

---

## Advanced Examples

### Creating a Simple AI Patrol Behavior

```lua
-- Simple patrol behavior using Lua

-- Configuration
local patrolPoints = {}
local currentPointIndex = 1
local moveSpeed = 100.0
local rotationSpeed = 2.0
local waitTime = 2.0
local waitCounter = 0.0
local isWaiting = false

function init()
    if not self then return end
    
    UE.Print("Initializing patrol behavior for " .. self.ActorLabel)
    
    -- Define patrol points (could also be loaded from actor properties)
    local startPos = self.ActorLocation
    patrolPoints = {
        {X = startPos.X + 300, Y = startPos.Y, Z = startPos.Z},
        {X = startPos.X + 300, Y = startPos.Y + 300, Z = startPos.Z},
        {X = startPos.X, Y = startPos.Y + 300, Z = startPos.Z},
        {X = startPos.X, Y = startPos.Y, Z = startPos.Z}
    }
    
    UE.Print("Patrol ready with " .. #patrolPoints .. " points")
end

function tick(deltaTime)
    if not self then return end
    
    if isWaiting then
        -- Wait at current point
        waitCounter = waitCounter + deltaTime
        if waitCounter >= waitTime then
            waitCounter = 0.0
            isWaiting = false
            
            -- Move to the next patrol point
            currentPointIndex = currentPointIndex + 1
            if currentPointIndex > #patrolPoints then
                currentPointIndex = 1
            end
            
            UE.Print("Moving to point " .. currentPointIndex)
        end
    else
        -- Move towards the current patrol point
        local targetPoint = patrolPoints[currentPointIndex]
        local currentPos = self.ActorLocation
        
        -- Convert patrol point to vector
        local targetPos = UE.Math.Vector(targetPoint.X, targetPoint.Y, targetPoint.Z)
        
        -- Calculate direction and distance
        local direction = {
            X = targetPoint.X - currentPos.X,
            Y = targetPoint.Y - currentPos.Y,
            Z = targetPoint.Z - currentPos.Z
        }
        local distance = math.sqrt(direction.X^2 + direction.Y^2 + direction.Z^2)
        
        -- Normalize direction
        if distance > 0 then
            direction.X = direction.X / distance
            direction.Y = direction.Y / distance
            direction.Z = direction.Z / distance
        end
        
        -- Check if we reached the target point
        if distance < 50 then
            -- Arrived at point, start waiting
            isWaiting = true
            UE.Print("Reached point " .. currentPointIndex .. ", waiting...")
            return
        end
        
        -- Move towards target
        local moveStep = moveSpeed * deltaTime
        local newPos = UE.Math.Vector(
            currentPos.X + direction.X * moveStep,
            currentPos.Y + direction.Y * moveStep,
            currentPos.Z
        )
        
        -- Update actor position
        self.ActorLocation = newPos
        
        -- Calculate rotation to face movement direction
        local targetYaw = math.atan2(direction.Y, direction.X) * (180 / math.pi)
        local currentRotation = self.ActorRotation
        
        -- Smoothly rotate towards the target direction
        local newYaw = currentRotation.Yaw
        local yawDiff = targetYaw - newYaw
        
        -- Normalize angle to -180 to 180 range
        while yawDiff > 180 do yawDiff = yawDiff - 360 end
        while yawDiff < -180 do yawDiff = yawDiff + 360 end
        
        newYaw = newYaw + yawDiff * rotationSpeed * deltaTime
        
        -- Update actor rotation
        self.ActorRotation = UE.Math.Rotation(currentRotation.Pitch, newYaw, currentRotation.Roll)
    end
end
```

### Interacting with Other Actors

```lua
-- Script demonstrating how to interact with other actors

local targetActor = nil
local interactionRange = 300.0
local followSpeed = 150.0

function init()
    if not self then return end
    
    UE.Print("Looking for interaction target...")
    targetActor = UE.Actor.Find("TargetCube")
    
    if targetActor then
        UE.Print("Found target: " .. targetActor.ActorLabel)
    else
        UE.Log.Warning("Target not found!")
    end
end

function tick(deltaTime)
    if not self or not targetActor then return end
    
    -- Get positions
    local myPos = self.ActorLocation
    local targetPos = targetActor.ActorLocation
    
    -- Calculate distance
    local dx = targetPos.X - myPos.X
    local dy = targetPos.Y - myPos.Y
    local dz = targetPos.Z - myPos.Z
    local distance = math.sqrt(dx*dx + dy*dy + dz*dz)
    
    -- Check if within interaction range
    if distance <= interactionRange then
        -- We're close enough to interact
        interactWithTarget(deltaTime)
    else
        -- Move towards the target
        followTarget(deltaTime, dx, dy, dz, distance)
    end
end

function followTarget(deltaTime, dx, dy, dz, distance)
    -- Normalize direction
    local dirX = dx / distance
    local dirY = dy / distance
    local dirZ = dz / distance
    
    -- Move towards target
    local moveStep = followSpeed * deltaTime
    local myPos = self.ActorLocation
    local newPos = UE.Math.Vector(
        myPos.X + dirX * moveStep,
        myPos.Y + dirY * moveStep,
        myPos.Z + dirZ * moveStep
    )
    
    -- Update position
    self.ActorLocation = newPos
    
    -- Update rotation to face target
    local targetYaw = math.atan2(dirY, dirX) * (180 / math.pi)
    self.ActorRotation = UE.Math.Rotation(0, targetYaw, 0)
    
    -- Visualization
    UE.Print("Following target at distance: " .. string.format("%.2f", distance))
end

function interactWithTarget(deltaTime)
    -- Orbit around the target when in range
    local targetPos = targetActor.ActorLocation
    local time = UE.GetTotalElapsedTime()
    
    -- Calculate orbit position
    local angle = time * 0.5 -- orbit speed
    local orbitRadius = interactionRange * 0.5
    local orbitX = targetPos.X + math.cos(angle) * orbitRadius
    local orbitY = targetPos.Y + math.sin(angle) * orbitRadius
    
    -- Update position
    self.ActorLocation = UE.Math.Vector(orbitX, orbitY, targetPos.Z)
    
    -- Look at target
    local dx = targetPos.X - orbitX
    local dy = targetPos.Y - orbitY
    local targetYaw = math.atan2(dy, dx) * (180 / math.pi)
    self.ActorRotation = UE.Math.Rotation(0, targetYaw, 0)
    
    -- Maybe trigger an effect on the target
    if math.floor(time) % 3 == 0 and math.floor(time * 10) % 10 == 0 then
        -- Custom function on the target if it exists (example only)
        if targetActor.TriggerEffect then
            targetActor.TriggerEffect()
        end
    end
end
```

This document provides a comprehensive reference for the Lua Scripting Plugin API. Use it as a guide when developing Lua scripts for your Unreal Engine projects.
