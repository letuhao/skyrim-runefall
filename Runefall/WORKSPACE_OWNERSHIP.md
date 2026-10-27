# Runefall - workspace ownership

- Workspace root: D:\Works\source\skyrim-moddings
- Git repository: D:\Works\source\skyrim-moddings\skyrim-runefall (GitHub letuhao/skyrim-runefall, branch main)
- Project root inside the repo: D:\Works\source\skyrim-moddings\skyrim-runefall\Runefall
- Active version: Runefall 0.1.0 (POC base frozen for real-mod development)
- Created: 2026-10-26
- Parent version: 0.1.18
- Project owner: the user (mod author). Executing agent: DSH (DeepSeek Harness) lead, with teammates as recorded in task history.
- Authority: this folder is the only writable root for Runefall work.

Read-only evidence realms (never write):
- D:\Works\source\skyrim-moddings\_tools
- D:\Works\source\skyrim-moddings\rpg-mechanism
- D:\Works\source\skyrim-moddings\_context
- H:\Skyrim\MantrasOfMara (target MO2 list, pinned Skyrim SE 1.6.1170)
- H:\Skyrim\TwistedSkyrim (secondary MO2 list, pinned 1.6.1170)
- E:\SteamLibrary\steamapps\common\Skyrim Special Edition (1.7.104 build; deferred/unsupported for v1, D-062)

Runtime targets and architecture: see GAME-DESIGN.md section 1 and section 4.

Handoff rule: any new agent session records its application, model and reasoning mode here (or in STATE.md) together with the active version, last successful build and next required command. Ownership transfer changes this record, not the filesystem.

Version rule: before editing a release snapshot, copy it to a new version folder, update CURRENT.txt and VERSION.txt, and start the CHANGELOG entry first. Never overwrite a previous version.
