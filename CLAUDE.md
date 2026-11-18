# EndlessOcean

Horror game for the Month Of Horror Jam 3 (itch.io). Unreal Engine 5.8, C++ project,
started from the First Person template (with its Horror and Shooter variants).

## Project overview

### The jam

- **Jam:** Month Of Horror Jam 3 (itch.io). Theme: **Endless Ocean**.
- **Dates:** 18 November 2026 to **21 December 2026**. Everything must be playable and
  submitted by then, so favour working features over perfect ones as the deadline nears.

### The game

You're aboard a **sinking ship**. Avoid a **monster** that hunts you, **repair the ship**
and **escape**.

- **Scope starts small and grows with progress.** First build a minimal playable loop
  (move, hide from or evade the monster, fix something, escape). Only then add features as
  time allows. Design systems so they can be extended later without rewrites.
- When suggesting features, say how big they are and flag anything that puts the deadline at risk.

**Inspirations:** use these as reference points for tone, mechanics and design discussions.
The notes say what each game is known for; they don't commit us to any of it.

- **Alien: Isolation:** a single relentless stalker AI you can't kill; hiding, evading and sound-based tension.
- **SOMA:** an isolated underwater facility, oppressive atmosphere and environmental storytelling.
- **Resident Evil 7: Biohazard:** first-person survival horror, stalker encounters, scarce resources.
- **Subnautica:** the ocean as a vast, beautiful and threatening place; scavenging, crafting and repair.
- **Still Wakes the Deep:** a disaster at sea (an oil rig) with a monster, collapsing and
  flooding structures, no combat, and an ordinary worker trying to escape.

### The team

- **Three developers**, all coming back to Unreal after a long break.
- **Active, experienced developers with a lot of Unity (C#) experience.** They've used
  Blueprints before but are new to C++ in Unreal. They can follow complex code; what's new
  to them is C++ and Unreal's way of doing things.
- This is a **learning project**: the goal is to relearn the engine and learn C++ along the way.

### How Claude should help

- **Take on the heavy, detailed programming** (boilerplate, plumbing, editor tooling, tricky
  engine APIs) so the team can focus on design and core gameplay.
- **Don't hide the C++. Teach it.** When writing or changing code:
  - Explain what the code does and why it's structured that way, in plain language.
  - Relate C++ and Unreal concepts to their **Unity/C#** equivalents where possible (e.g.
    `AActor` ↔ `GameObject`, `UActorComponent` ↔ `MonoBehaviour`, `UPROPERTY(EditAnywhere)` ↔
    `[SerializeField]`, `BeginPlay`/`Tick` ↔ `Start`/`Update`, delegates ↔ C# events/`UnityEvent`,
    `UDataAsset` ↔ `ScriptableObject`, Blueprint subclasses ↔ prefabs). Call out where the
    comparison breaks down (e.g. memory management, headers and the build step, garbage collection
    of UObjects versus C#'s GC).
  - Explain Unreal-specific C++ (UCLASS/UPROPERTY macros, `.generated.h`, `TObjectPtr`,
    garbage collection, modules and Build.cs) the first time it comes up in a task.
  - Point out the key lines to read rather than dumping large blocks without explanation.
- **Write efficient, best-practice code. Don't simplify it for learners.** The goal is to
  learn how it should really be done. If the right solution needs advanced C++ or Unreal
  techniques (templates, move semantics, custom allocators, async tasks and so on), use them
  and explain them thoroughly rather than swapping in a simpler, worse version.
- **Say what to do in the editor.** After C++ work, explain how to use it from the editor
  and Blueprints (which Blueprint to make, which properties to set, where things show up).

## Working rules

These are the project owner's standing rules. They may be added to or changed later;
this section always wins over anything else in this file.

### Code

- **Write C++.** Gameplay systems, components and tools are C++. Blueprints are only for
  wiring, content and tuning, never for core logic.
- **Use Unreal best practice.** Follow the Epic coding standard, use `TObjectPtr` for
  UPROPERTY object pointers, forward-declare in headers and include in `.cpp` files, keep
  headers lean, prefer `const` correctness, and avoid Tick when a timer, delegate or event will do.
- **Build modular, reusable systems.** Prefer self-contained `UActorComponent`s and subsystems
  over logic packed into characters or game modes. Systems talk through delegates, interfaces
  and gameplay tags rather than hard references to each other's classes. Put shared
  configuration in `UDataAsset`/`UPrimaryDataAsset` or `UDeveloperSettings`, not hard-coded values.
- **Comment well.** Every class, public function and UPROPERTY gets a `/** */` doc comment
  (Unreal shows these as editor tooltips). Explain *why* in the implementation where it isn't
  obvious. Don't leave commented-out code.
- **Make everything easy to configure.** Expose tuning values with `UPROPERTY(EditAnywhere)`
  and give them a `Category`, sensible `ClampMin`/`ClampMax`/`UIMin`/`UIMax`, `Units`, and
  `EditCondition`/`EditConditionHides` so the Details panel only shows what's relevant.
  Add `UFUNCTION(CallInEditor)` buttons for useful in-editor actions.
- **Build custom editors and inspectors** where they make a system smoother to use:
  `IDetailCustomization` / `IPropertyTypeCustomization`, component visualizers, editor
  utility widgets, asset actions and validators. All editor-only code lives in a separate
  `EndlessOceanEditor` editor module (`Type: Editor` in the `.uproject`). Create that module
  the first time it's needed. Editor code never goes in the runtime module.
- **Structure files sensibly.** Give each new system its own folder,
  e.g. `Source/EndlessOcean/Oxygen/`, with related classes kept together. Add new folders
  to `PublicIncludePaths` in `EndlessOcean.Build.cs`. One main class per file, with
  the file named after the class without its prefix (`OxygenComponent.h` for `UOxygenComponent`).

### Editing vs creating

- **Creating new source files:** go ahead without asking.
- **Editing an existing source file** (`.h`, `.cpp`, `.Build.cs`, `.Target.cs`, editor
  Python scripts) that wasn't created in the current task: ask first. Say which file
  and what will change, and wait for a yes. This includes the template code in
  `Source/EndlessOcean/`, `Variant_Horror/` and `Variant_Shooter/`.

### Plans (paper trail)

Every plan is kept as a versioned Markdown file in the repo, so the team can look back at
what was planned and why decisions were made.

- **Where:** `Docs/Plans/<YYYY-MM-DD>-<short-topic>/`, one folder per plan, dated when the
  plan was started (e.g. `Docs/Plans/2026-11-18-oxygen-system/`).
- **Versions:** each version of a plan is its own file, `v1.md`, `v2.md`, `v3.md` and so on.
  Never overwrite or edit an earlier version; any change to the plan becomes a new file.
- **This covers plan mode and any other planning:** design breakdowns, feature scoping,
  architecture decisions and so on.
- **In plan mode** only the plan file can be written. As soon as the plan is approved and
  plan mode ends, save every version that was presented to the user, rejected ones included,
  as `v1.md`, `v2.md`, …, before starting the work.
- **Each file starts with a header:**
  ```markdown
  # <Plan title> (v<N>)
  - Date: <YYYY-MM-DD>
  - Status: Proposed | Rejected | Approved | Superseded by v<N+1> | Done
  - Changes from v<N-1>: <what changed and why, including the user's feedback>
  ```
  Then the plan itself. Record the decisions made and the alternatives considered.
- Update only the `Status` line of an earlier version (e.g. to `Superseded by v3`). That's the
  one allowed edit to an old version.
- Add a line for each new plan folder to the index in `Docs/Plans/README.md`.
- Commit plan files on the same branch as the work they describe.

### Git

- **Branch for each new line of development.** Before starting a feature or system, create
  a branch from an up-to-date `main` with a short, descriptive name (e.g. `oxygen-system`).
- **Commit and push the branch regularly**, at each working step, with clear messages.
  Build before committing C++ changes, and never commit code that doesn't compile.
- **Never commit to or push `main` unless the user explicitly says to.** When a branch's work
  is done, ask the user whether to merge it into `main`, and only merge on their yes.

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
