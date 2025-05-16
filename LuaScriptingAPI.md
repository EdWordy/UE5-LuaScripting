# Lua Scripting API for Unreal Engine

This document provides a complete reference for the Lua scripting API available in your Unreal Engine project.

## Table of Contents
- [Overview](#overview)
- [Global References](#global-references)
- [UE Namespace](#ue-namespace)
  - [Core Functions](#core-functions)
  - [Logging](#logging)
  - [Actor Functions](#actor-functions)
  - [Events](#events)
- [Script Lifecycle](#script-lifecycle)
- [Data Types](#data-types)
- [Best Practices](#best-practices)
- [Examples](#examples)

## Overview

The Lua scripting system allows you to write gameplay logic using Lua scripts. Scripts can be attached to actors in the level using the `LuaScriptComponent`. 

Each script runs in its own Lua environment with access to a set of UE4-specific functions and objects.

## Global References

The following global variables are available in every script:

| Name | Type | Description |
|------|------|-------------|
| `self` | AActor | Reference to the actor that owns the script component |
| `component` | ULuaScriptComponent | Reference to the script component itself |

## UE Namespace

The `UE` namespace contains all engine-specific functions.

### Core Functions

| Function | Return Type | Description |
|----------|-------------|-------------|
| `UE.Print(message)` | None | Prints a message to the output log and screen |
| `UE.GetDeltaTime()` | Number | Returns the time in seconds since the last frame |
| `UE.GetWorld()` | UWorld | Returns the current world object |

### Logging

| Function | Return Type | Description |
|----------|-------------|-------------|
| `UE.Log.Trace(message)` | None | Logs a trace message |
| `UE.Log.Warning(message)` | None | Logs a warning message |
| `UE.Log.Error(message)` | None | Logs an error message |

### Actor Functions

| Function | Return Type | Description |
|----------|-------------|-------------|
| `UE.Actor.FindActor(name)` | AActor | Finds an actor by name |
| `UE.Actor.SpawnActor(className, x, y, z)` | AActor | Spawns a new actor at the specified location |
| `UE.Actor.DestroyActor(actor)` | Boolean | Destroys the given actor, returns true if successful |

### Events

| Function | Return Type | Description |
|----------|-------------|-------------|
| `UE.Event.Register(eventName, handlerFunction)` | None | Registers a handler function for the specified event |
| `UE.Event.Trigger(eventName, ...)` | None | Triggers an event with optional parameters |
| `UE.Event.Unregister(eventName)` | None | Unregisters all handlers for the specified event |

## Script Lifecycle

Each script has specific lifecycle functions that get called at specific times:

| Function | Description |
|----------|-------------|
| `init()` | Called when the script is first loaded, before any gameplay begins |
| `tick(deltaTime)` | Called every frame if `bCallTickFunction` is set to true |

Optional custom functions:
- You can define any custom functions that can be called from Blueprint using `CallFunction(functionName)`

## Data Types

### Vectors/Locations

Vectors are represented as tables with X, Y, and Z components:

```lua
local location = {
    X = 100,
    Y = 200,
    Z = 300
}
```

### Rotators

Rotators are represented as tables with Pitch, Yaw, and Roll components:

```lua
local rotation = {
    Pitch = 0,
    Yaw = 90,
    Roll = 0
}
```

## Best Practices

1. **Use Lua's strengths**: Take advantage of Lua's tables, functions as first-class values, and closures
2. **Avoid excessive object spawning**: Spawning objects is expensive - reuse or pool them when possible
3. **Clean up after yourself**: Make sure to unregister events when you're done with them
4. **Use the global storage wisely**: Use `_G` for storing persistent data, but be careful of name conflicts
5. **Keep scripts modular**: Split large scripts into multiple logical components

## Examples

### Basic Script

```lua
function init()
    UE.Print("Script initialized!")
end

function tick(deltaTime)
    -- Code that runs every frame
end
```

### Spawning and Managing Actors

```lua
function init()
    -- Spawn 5 cubes in a circle
    local radius = 200
    self.cubes = {}
    
    for i = 1, 5 do
        local angle = (i-1) * (2 * math.pi / 5)
        local x = radius * math.cos(angle)
        local y = radius * math.sin(angle)
        local cube = UE.Actor.SpawnActor("StaticMeshActor", x, y, 100)
        table.insert(self.cubes, cube)
    end
end

function cleanup()
    -- Destroy all spawned cubes
    for _, cube in ipairs(self.cubes) do
        UE.Actor.DestroyActor(cube)
    end
    self.cubes = {}
end
```

### Using the Event System

```lua
function init()
    -- Register event handlers
    UE.Event.Register("PlayerDamaged", handlePlayerDamaged)
    UE.Event.Register("GameOver", handleGameOver)
    
    -- Game state
    _G.gameState = {
        playerHealth = 100,
        score = 0
    }
end

function handlePlayerDamaged(damage)
    _G.gameState.playerHealth = _G.gameState.playerHealth - damage
    UE.Print("Player health: " .. _G.gameState.playerHealth)
    
    if _G.gameState.playerHealth <= 0 then
        UE.Event.Trigger("GameOver")
    end
end

function handleGameOver()
    UE.Print("Game Over! Final score: " .. _G.gameState.score)
    -- Clean up
    UE.Event.Unregister("PlayerDamaged")
    UE.Event.Unregister("GameOver")
end

-- Can be called from Blueprint
function damagePlayer(amount)
    UE.Event.Trigger("PlayerDamaged", amount)
    return _G.gameState.playerHealth
end
```

### Finding and Interacting with Actors

```lua
function init()
    -- Find the player actor
    self.player = UE.Actor.FindActor("PlayerCharacter")
    if not self.player then
        UE.Log.Error("Could not find player character!")
        return
    end
    
    UE.Print("Found player: " .. tostring(self.player))
end
```
