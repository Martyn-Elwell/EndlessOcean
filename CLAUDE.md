# EndlessOcean

Horror game for the Month Of Horror Jam 3 (itch.io). Unreal Engine 5.8, C++ project,
started from the First Person template (with its Horror and Shooter variants).

## Engine

- Engine install: `C:\Program Files\Epic Games\UE_5.8`
- Single runtime module: `EndlessOcean` (`Source/EndlessOcean`)
- Enabled plugins: StateTree, GameplayStateTree, ModelingToolsEditorMode

## Building

Build the editor target from the project root (PowerShell):

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" EndlessOceanEditor Win64 Development -Project="$PWD\EndlessOcean.uproject" -WaitMutex
```

- Build after every C++ change and fix all compiler errors before reporting work as done.
- If the build fails with a Live Coding error, the editor is open with Live Coding on.
  Don't try to work around it: tell the user to close the editor or to press
  Ctrl+Alt+F11 in the editor to recompile.
- After adding, removing or moving source files, regenerate project files:
  ```powershell
  & "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="$PWD\EndlessOcean.uproject" -game -rocket -progress
  ```

## Source layout

- `Source/EndlessOcean/` holds the base character, player controller, game mode and camera manager.
- `Variant_Horror/` holds the horror character (stamina sprint, spotlight), game mode, controller and UI.
- `Variant_Shooter/` holds weapons, pickups, projectiles, AI (StateTree + EQS) and UI.
- Every subfolder is listed in `PublicIncludePaths` in `EndlessOcean.Build.cs`, so includes
  use bare file names (`#include "HorrorCharacter.h"`). A new subfolder must be added there.

## Code conventions

- Follow the Epic coding standard: `A` for actors, `U` for UObjects/components, `F` for structs,
  `E` for enums, `I` for interfaces, `b` prefix for bools. Tabs for indentation.
- Each module class uses `ENDLESSOCEAN_API`, and the `.generated.h` include comes last.
- Put gameplay logic in C++ and expose it to Blueprints with `UFUNCTION(BlueprintCallable)`,
  `UPROPERTY(EditAnywhere/BlueprintReadWrite)` and so on. Designers tune values in Blueprints.
- Use Enhanced Input (`UInputAction`) for new inputs, not legacy action mappings.
- New module dependencies go in `EndlessOcean.Build.cs`.

## Unreal MCP (editor access)

The project enables Epic's experimental built-in MCP server (`ModelContextProtocol` and
`AllToolsets` plugins, editor only). `.mcp.json` points Claude Code at it as `unreal-mcp`.

- It only works while the editor is open. It listens on `http://127.0.0.1:8000/mcp`
  (loopback only, no auth). Auto-start is off by default in 5.8, so
  `Config/DefaultEditorPerProjectUserSettings.ini` turns it on (`bAutoStartServer=True`).
  If the tools are missing, the editor is closed or the server didn't start: check the
  Output Log, or run `ModelContextProtocol.StartServer` in the editor console.
- Tool Search is on, so the server lists three meta-tools: `list_toolsets`, `describe_toolset`
  and `call_tool`. Find the right toolset and tool first, then use `call_tool`.
- Use MCP for Blueprint, asset, level, material, UMG and StateTree work that can't be done as text.
  Keep gameplay logic in C++.
- Tool calls run one at a time on the game thread. Don't send overlapping calls, and split
  long jobs into shorter calls (Claude Code times out after about 120 seconds).
- After a C++ change, MCP tools may be stale until `ModelContextProtocol.RefreshTools` runs
  and the client reconnects.

## What not to touch

- `Content/` holds binary `.uasset`/`.umap` files (Git LFS). They can't be read or edited as text.
  If a change needs Blueprint or asset edits, describe the steps for the user to do in the
  editor, or write an editor Python script for them to run.
- `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/` and the `.sln`/`.slnx` files
  are generated. Never edit them.
- `Config/Default*.ini` are tracked in git. Edits to them are real project changes.
