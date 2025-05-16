# Lua Scripting API for Unreal Engine

This document provides a reference for the Lua scripting API available in Unreal Engine, based on functionality verified through testing.

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
- [Examples](#examples)

## Overview

The Lua scripting system allows you to create gameplay logic using Lua scripts. Scripts are attached to actors via the `LuaScriptComponent` and have access to both global objects and a suite of UE functions.

## Global References

Each script has access to these predefined global variables:

| Name | Type | Description |
|------|------|-------------|
| `self` | UObject | Reference to the actor that owns the script component |
| `component` | UObject | Reference to the script component itself |

Example:
```lua
-- Print references to see what objects we have access to
UE.Print("Self reference: " .. tostring(self))
UE.Print("Component reference: " .. tostring(component))
```

## UE Namespace

The `UE` namespace contains engine-specific functions organized into categories.

### Core Functions

| Function | Parameters | Return Type | Description |
|----------|------------|-------------|-------------|
| `UE.Print(message)` | String | None | Prints a message to the output log and on-screen |
| `UE.GetDeltaTime()` | None | Number | Returns the time in seconds since the last frame |
| `UE.GetWorld()` | None | UObject | Returns the current world object |

Example:
```lua
-- Print a message to the log
UE.Print("Hello from Lua!")

-- Get the current delta time
local dt = UE.GetDeltaTime()
UE.Print("Delta time: " .. tostring(dt))

-- Get the world object
local world = UE.GetWorld()
UE.Print("World: " .. tostring(world))
```

### Logging

| Function | Parameters | Return Type | Description |
|----------|------------|-------------|-------------|
| `UE.Log.Trace(message)` | String | None | Logs a message at trace level |
| `UE.Log.Warning(message)` | String | None | Logs a message at warning level |
| `UE.Log.Error(message)` | String | None | Logs a message at error level |

Example:
```lua
-- Log messages at different levels
UE.Log.Trace("This is a trace message")
UE.Log.Warning("This is a warning message")
UE.Log.Error("This is an error message")
```

### Actor Functions

| Function | Parameters | Return Type | Description |
|----------|------------|-------------|-------------|
| `UE.Actor.FindActor(name)` | String | UObject or nil | Finds an actor by name |
| `UE.Actor.SpawnActor(className, x, y, z)` | String, Number, Number, Number | UObject or nil | Spawns an actor of the specified class at the given location |
| `UE.Actor.DestroyActor(actor)` | UObject | Boolean | Destroys the specified actor |

Example:
```lua
-- Find an existing actor
local actor = UE.Actor.FindActor("BP_LuaScript")
if actor then
    UE.Print("Found actor: " .. tostring(actor))
end

-- Spawn a new actor
local newActor = UE.Actor.SpawnActor("StaticMeshActor", 100, 200, 300)
if newActor then
    UE.Print("Spawned actor: " .. tostring(newActor))
    
    -- Destroy the actor when done
    local success = UE.Actor.DestroyActor(newActor)
    UE.Print("Actor destroyed: " .. tostring(success))
end
```

### Events

| Function | Parameters | Return Type | Description |
|----------|------------|-------------|-------------|
| `UE.Event.Register(eventName, handlerFunction)` | String, Function | None | Registers a function to handle the specified event |
| `UE.Event.Trigger(eventName, ...)` | String, Any... | None | Triggers an event with optional parameters |
| `UE.Event.Unregister(eventName)` | String | None | Removes all handlers for the specified event |

Example:
```lua
-- Register an event handler
UE.Event.Register("DataChanged", function(key, value)
    UE.Print("Event received: " .. key .. " = " .. tostring(value))
    _G.myData[key] = value
end)

-- Trigger the event
UE.Event.Trigger("DataChanged", "health", 75)

-- Unregister when done
UE.Event.Unregister("DataChanged")
```

## Script Lifecycle

Scripts have predefined functions that are called at specific times:

| Function | Parameters | Description |
|----------|------------|-------------|
| `init()` | None | Called when the script is first loaded |
| `tick(deltaTime)` | Number | Called every frame if enabled in the component |

Example:
```lua
function init()
    UE.Print("Script initialized!")
    -- Initialize your script here
end

function tick(deltaTime)
    -- This runs every frame when enabled
    -- deltaTime contains the time since the last frame
end
```

## Data Types

### Tables

Lua tables are used extensively for data storage:

```lua
-- Store data in a table
_G.myActorData = {
    name = "MyActor",
    position = { x = 100, y = 200, z = 300 },
    isActive = true,
    health = 100
}

-- Access table data
UE.Print("Actor name: " .. _G.myActorData.name)
UE.Print("Position: x=" .. _G.myActorData.position.x)
```

### Global Storage

The `_G` table provides global storage accessible from anywhere in your script:

```lua
-- Store global data
_G.playerScore = 0

-- Increment score
function addPoints(points)
    _G.playerScore = _G.playerScore + points
    UE.Print("Score: " .. _G.playerScore)
end
```

## Examples

### Minimal Script

```lua
function init()
    UE.Print("=== Script Started ===")
    UE.Print("Self: " .. tostring(self))
    UE.Print("Component: " .. tostring(component))
end

function tick(deltaTime)
    -- Empty tick function
end
```

### Actor Spawner

```lua
function init()
    UE.Print("=== Actor Spawner ===")
    
    -- Initialize storage
    _G.spawnedActors = {}
    
    -- Register event handler
    UE.Event.Register("SpawnActor", handleSpawn)
    UE.Event.Register("ClearActors", handleClear)
    
    UE.Print("Ready to spawn actors. Trigger 'SpawnActor' event to spawn.")
end

function handleSpawn(x, y, z)
    -- Default position if not provided
    x = x or 0
    y = y or 0
    z = z or 100
    
    -- Spawn the actor
    local actor = UE.Actor.SpawnActor("StaticMeshActor", x, y, z)
    
    if actor then
        table.insert(_G.spawnedActors, actor)
        UE.Print("Actor spawned: " .. tostring(actor))
        UE.Print("Total actors: " .. #_G.spawnedActors)
    else
        UE.Log.Error("Failed to spawn actor")
    end
end

function handleClear()
    UE.Print("Clearing " .. #_G.spawnedActors .. " actors...")
    
    for _, actor in ipairs(_G.spawnedActors) do
        UE.Actor.DestroyActor(actor)
    end
    
    _G.spawnedActors = {}
    UE.Print("All actors cleared")
end

-- Custom function that can be called from Blueprint
function spawnActorAtPosition(x, y, z)
    handleSpawn(x, y, z)
    return #_G.spawnedActors
end

-- Custom function to clear all actors
function clearAllActors()
    handleClear()
    return #_G.spawnedActors
end
```

### Event System Demo

```lua
function init()
    UE.Print("=== Event System Demo ===")
    
    -- Set up game state
    _G.gameState = {
        playerHealth = 100,
        score = 0,
        level = 1
    }
    
    -- Register event handlers
    UE.Event.Register("PlayerDamaged", function(amount)
        _G.gameState.playerHealth = _G.gameState.playerHealth - amount
        UE.Print("Player took " .. amount .. " damage. Health: " .. _G.gameState.playerHealth)
        
        if _G.gameState.playerHealth <= 0 then
            UE.Event.Trigger("GameOver")
        end
    end)
    
    UE.Event.Register("ScorePoints", function(points)
        _G.gameState.score = _G.gameState.score + points
        UE.Print("Scored " .. points .. " points. Total: " .. _G.gameState.score)
    end)
    
    UE.Event.Register("GameOver", function()
        UE.Print("GAME OVER! Final score: " .. _G.gameState.score)
    end)
    
    UE.Print("Event system ready. Use UE.Event.Trigger() to trigger events.")
end

-- These functions can be called from Blueprint
function damagePlayer(amount)
    UE.Event.Trigger("PlayerDamaged", amount)
    return _G.gameState.playerHealth
end

function addScore(points)
    UE.Event.Trigger("ScorePoints", points)
    return _G.gameState.score
end
```
