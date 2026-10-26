# Runefall plugin (Phase 0)

C++ SKSE plugin scaffold, template-derived from codepuncher/CommonLibSSE-NG-template.

Pinned versions:

- CommonLibSSE-NG: alandtse fork **v14.0.1** (commit 723cfda480ae, 2026-10-25). Active fork with
  1.7.x runtime support; the CharmedBaryon repository is not used.
- vcpkg: submodule at baseline ee12231b20c95013c6638d845d04c91559a1d1ff.
- Triplet: x64-windows-static-md.
- Toolchain: Visual Studio 2022 Enterprise (MSVC 14.44), CMake, Ninja, vcpkg.

Build:

    call "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Auxiliary\\Build\\vcvars64.bat"
    set "PATH=C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin;C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\Ninja;%PATH%"
    cd Runefall\\src\\plugin
    cmake --preset release-windows
    cmake --build --preset release-windows

Output: build/msvc/Runefall.dll (deploy preset copies to SKSE/Plugins).

Phase 0 scaffold proves: plugin load, logging, Papyrus interface registration, co-save interface
registration, messaging listener. Hooks, data loading, menu page and input follow the L4 checklist.
