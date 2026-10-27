# Runefall — credits and licenses

## Code and tooling

| Component | Use | License / attribution |
|---|---|---|
| CommonLibSSE-NG (alandtse fork v14.0.1) | native SKSE plugin framework | MIT |
| SKSE64 | script extender runtime | SKSE team license; not redistributed |
| Mutagen | plugin generation library | MIT |
| SKSE Menu Framework (Thiago099) | Mod Control Panel API/release | follow published license; release build required |
| PapyrusCompiler | compiling shipped PEX | Bethesda toolchain; PEX only in archive |
| nlohmann/json, spdlog, fmt | linked libraries | MIT/BSD-style; see vcpkg metadata |

## Game content

No Skyrim assets are redistributed. The mod only adds new ESL records and runtime behavior;
vanilla records are read, never overridden.

## AI assistance

Content-generation scripts, validation harnesses, documentation and parts of the implementation
were developed with AI coding assistance. Shipped binaries are built from this repository source;
artifact hashes are recorded in Runefall 0.1.0/VERSION.txt.