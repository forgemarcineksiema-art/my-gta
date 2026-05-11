# Backend modernization grounding (anti-hallucination)

Status: LIVE (for modernization prep)
Last verified against implementation evidence: 2026-05-11

Cel tego dokumentu: uziemić przyszłe prace modernizacyjne backendu tak, żeby nie „dopisywać” systemów, które nie istnieją, i żeby nie psuć aktualnego workflow build/test/smoke.

## 1) Truth hierarchy (hierarchia prawdy)

Jeśli pojawia się konflikt, obowiązuje kolejność:

- **Implemented (Operational truth)**: kod, `CMakeLists.txt`, `CMakePresets.json`, testy/CTest, walidatory, skrypty uruchomieniowe i faktyczne zachowanie runtime (smoke).
- **Implemented (Workflow truth)**: `DEVELOPMENT.md`, `docs/README.md`, `docs/current-state.md` (podporządkowane temu, co da się uruchomić i zweryfikować).
- **Partial/Experimental/Aspirational**: reszta dokumentów — kierunek, intencje, backlog. Nie nadpisują implementacji.

Reguła: **nie ufaj starym design docom ponad evidence z implementacji**. Jeśli coś nie ma śladu w kodzie/CMake/CTest/walidatorach/skryptach, traktuj to jako niezaimplementowane.

Etykiety statusu używane w tym repo:
- **Implemented**: działa dziś i ma wsparcie w kodzie/CMake/testach/walidatorach/skryptach.
- **Partial**: istnieje tylko dla wycinka/slice.
- **Experimental**: narzędzia/tryby rozwojowe, niestabilne lub dev-only.
- **Aspirational**: kierunek/plan, nie obietnica istniejącej implementacji.
- **Deprecated**: stare zachowanie/workflow, którego nie należy rozwijać.

## 2) Current implemented baseline (co jest zaimplementowane dzisiaj)

- **Implemented**: renderer runtime oparty o **raylib 6.0** (pobierany przez CMake `FetchContent` z tagu `6.0`).
- **Implemented**: **C++17 + CMake** (standard wymuszony w `CMakeLists.txt`).
- **Implemented**: biblioteka `bs3d_core` (rdzeń gameplay/fundamenty).
- **Implemented (Partial slice)**: grywalny „intro slice” uruchamiany przez `blokowa_satyra` (wg `docs/current-state.md` + target w CMake).
- **Implemented**: własna fizyka/kolizje **`WorldCollision`** (custom, jako obecna prawda runtime).
- **Implemented**: **`PlayerMotor`** (ruch postaci).
- **Implemented**: **`CameraRig`** (kamera jako system produkcyjny).
- **Implemented**: **`VehicleController`** (jazda/sterowanie pojazdem w obecnym wycinku).
- **Implemented**: **`ControlContext`** (kontekstowe mapowanie wejścia; architektura: RawInput -> ControlContext -> InputState).
- **Implemented**: gate’y jakości przez **walidatory + CTest**:
  - `bs3d_core_tests`, `bs3d_game_support_tests`
  - walidatory Python (assets/world contract/editor overlay/object outcomes/mission) uruchamiane w CTest, gdy dostępny jest Python.
- **Implemented**: workflow presetów CMake i oficjalne foldery build (m.in. `build/ci-core`, `build/ci`) zgodnie z `CMakePresets.json`.

## 3) Aspirational / not implemented yet (jasno: NIE jest zaimplementowane)

Poniższe są **Aspirational (Not implemented)** — nie ma dowodu w aktualnym CMake, testach i runtime, że to istnieje:

- **Aspirational**: backend renderera **DirectX 11**.
- **Aspirational**: backend fizyki **PhysX**.
- **Aspirational**: backend fizyki **Jolt**.
- **Aspirational**: render backend **DX12/Vulkan**.
- **Aspirational**: pełny „Euphoria-like” active ragdoll.
- **Aspirational**: pełny open world (poza aktualnym slice).
- **Aspirational**: finalny pipeline materiałów/shaderów (obecnie pipeline jest pragmatyczny i „art kit”, nie AAA).

## 4) Protected systems (nie ruszać w trakcie przygotowań backendu)

W pracach „backend prep / modernization” tych systemów **nie przepisujemy** i nie zmieniamy zachowania gameplay (tylko adaptory/wrappers na granicach):

- **Implemented (Protected)**: `PlayerMotor`
- **Implemented (Protected)**: `CameraRig`
- **Implemented (Protected)**: `VehicleController`
- **Implemented (Protected)**: `MissionController`
- **Implemented (Protected)**: `PrzypalSystem`
- **Implemented (Protected)**: `WorldEventLedger`
- **Implemented (Protected)**: `ControlContext`
- **Implemented (Protected)**: `SaveGame`
- **Implemented (Protected)**: walidatory (Python) i gate’y CTest
- **Implemented (Protected)**: workflow `--data-root` oraz uruchamianie przez skrypty (`tools/run_dev.ps1`, `tools/run_release_smoke.ps1`, `tools/ci_smoke.ps1`)

## 5) Known weak spots (rzeczy, których nie warto portować 1:1)

To są realne „punkty zaczepienia” do modernizacji, ale **nie kopiujemy ich 1:1** do przyszłego backendu:

- **Implemented (Weak spot)**: `WorldModelCache` wystawiający typy `raylib` (np. `Model`) poza granice renderera.
- **Implemented (Weak spot)**: `GameRenderers.cpp` z natychmiastowym/immediate-mode rysowaniem przez raylib.
- **Implemented (Weak spot)**: `GameApp.cpp` jako monolityczna orkiestracja runtime (duża odpowiedzialność w jednym miejscu).
- **Implemented (Weak spot)**: użycie `raylib::Camera3D` (lub struktury `Camera3D`) jako elementu snapshotu renderu w runtime, zamiast neutralnego opisu kamery.
- **Implemented (Weak spot)**: ładowanie assetów/modeli bezpośrednio przez raylib w runtime (silne sprzężenie I/O + render backend).
- **Implemented (Weak spot)**: statyczne wywołania `WorldRenderer` (trudniejsze do testowania/abstrakcji).

## 6) Backend modernization direction (kierunek, bez zmiany zachowania dziś)

Kierunek modernizacji ma umożliwić w przyszłości wybór backendów, ale **dzisiejsze zachowanie pozostaje bez zmian**.

- **Implemented (Supported values today)**:
  - renderer: `raylib`
  - fizyka: `custom` (`WorldCollision`)
- **Aspirational (Planned values; NOT implemented)**:
  - renderer: `d3d11`
  - fizyka: `physx`
- **Implemented (Current CLI surface)**:
  - `--renderer <raylib>` (planned value `d3d11` fails with a clear error)
  - `--physics <custom>` (planned value `physx` fails with a clear error)

Uwaga (explicit): **CLI flags do not mean d3d11/physx are implemented**. To jest tylko plumbing pod przyszłe prace.

Backend prep status update: istnieją teraz backend-neutralne interfejsy `IPlatform` i `IInputReader`.
Obecnie jedynymi implementacjami runtime są `RaylibPlatform` i `RaylibInputReader`.
To nadal **nie oznacza**, że D3D11 ani PhysX są zaimplementowane.

Backend prep status update: istnieje teraz backend-neutralny szkielet `RenderFrame` / listy komend renderu.
Obecny renderer runtime nadal jest raylibowy i nadal używa istniejących klas `WorldRenderer`, `HudRenderer` i `DebugRenderer`.
D3D11 nadal **nie jest zaimplementowany**.
Przyszłe prace nad rendererem powinny konsumować `RenderFrame`, zamiast kopiować raylibowe natychmiastowe wywołania typu `DrawCubeV`, `DrawSphere`, `DrawModelEx` albo stos `rlPushMatrix`.

Backend prep status update: istnieje teraz częściowa ścieżka ekstrakcji `WorldRenderList` -> `RenderFrame` używana tylko w testach.
To nadal jest eksperymentalna/data-only ścieżka graniczna; aktywny renderer runtime pozostaje raylibowy.
D3D11 nadal **nie jest zaimplementowany**.
Ta ekstrakcja jest krokiem pośrednim pod przyszły backend renderera, nie zamianą obecnego renderowania.

Backend prep status update: `WorldRenderList` został przeniesiony do małego, backend-neutralnego nagłówka `include/bs3d/render/WorldRenderList.h`.
`RenderExtraction` nie zależy już od `GameRenderers.h`.
Runtime renderer nadal jest raylibowy.
D3D11 nadal **nie jest zaimplementowany**.

Backend prep status update: istnieje teraz backend-neutralny `RenderFrameBuilder` (`include/bs3d/render/RenderFrameBuilder.h`).
`RenderFrameBuilder` akumuluje komendy prymitywów wg bucketu i buduje `RenderFrame` w produkcyjnej kolejności renderowania.
Jest helperem kontraktowym/testowym — **nie jest render grafem i sam nic nie rysuje**.
Aktywne renderowanie runtime nadal jest raylibowe.
D3D11 nadal **nie jest zaimplementowany**.

Backend prep status update: istnieje teraz `NullRenderer` (`include/bs3d/render/NullRenderer.h`) jako backend-neutralny no-op/testowy renderer implementujący `IRenderer`.
`NullRenderer` nie otwiera okna, nie używa GPU i zapisuje statystyki oraz wyniki walidacji każdego skonsumowanego `RenderFrame`.
Istnieje szkielet `RendererFactory` (`include/bs3d/render/RendererFactory.h`) jako przyszły seam do tworzenia rendererów.
Aktualnie jedyna działająca ścieżka fabryki to `NullRenderer` (przez `useNullRenderer = true`).
Raylib `IRenderer` adapter **nie jest zaimplementowany** — legacy runtime nadal używa `WorldRenderer`/`HudRenderer`/`DebugRenderer`.
D3D11 nadal **nie jest zaimplementowany**.
Aktywne renderowanie runtime nadal jest raylibowe.

Backend prep status update: istnieje teraz backend-neutralny interfejs `IRenderer`, który konsumuje `RenderFrame`.
Testowy `RecordingRenderer` istnieje tylko w testach i zapisuje wyłącznie liczniki komend.
Aktywne renderowanie runtime nadal przechodzi przez raylibowe `WorldRenderer`, `HudRenderer` i `DebugRenderer`.
D3D11 nadal **nie jest zaimplementowany**.
Przyszłe backendy renderera powinny implementować `IRenderer` i konsumować `RenderFrame`.

Backend prep status update: istnieją teraz backend-neutralne helpery statystyk i walidacji `RenderFrame`.
Są używane w testach do sprawdzania liczby komend i kolejności bucketów bez GPU ani runtime renderera.
Aktywne renderowanie runtime nadal jest raylibowe.
D3D11 nadal **nie jest zaimplementowany**.
Przyszłe backendy renderera powinny walidować i podsumowywać `RenderFrame` przed rysowaniem.

Backend spike status update: istnieje teraz samodzielny, Windows-only spike `bs3d_d3d11_boot`.
Ten target tworzy okno Win32, inicjalizuje Direct3D 11, czyści, rysuje hardcoded indexed colored cube, presentuje swapchain przez krótki przebieg i kończy działanie.
`bs3d_d3d11_boot` potwierdza teraz pierwszą ścieżkę rysowania D3D11: shader, input layout, vertex buffer, index buffer i draw call dla hardcoded colored cube.
`bs3d_d3d11_boot` potwierdza też minimalną ścieżkę constant-buffer update i model/view/projection transform w shaderze dla tego standalone spike.
`bs3d_d3d11_boot` potwierdza teraz minimalną ścieżkę depth buffer + depth-stencil state (D24_UNORM_S8_UINT, DepthEnable/LESS, clear depth co klatkę).
`bs3d_d3d11_boot` potwierdza teraz minimalną ścieżkę indexed geometry: vertex buffer + index buffer (uint16_t, TRIANGLELIST) dla hardcoded colored cube.
Nie jest podłączony do runtime renderingu, `GameApp`, `IRenderer` ani `RendererFactory`.
`D3D11Renderer` nadal **nie jest zaimplementowany**.
Aktywne renderowanie runtime nadal jest raylibowe.

Backend spike status update: `bs3d_d3d11_boot` został zrefaktoryzowany z jednego monolitycznego pliku na lokalne moduły spike (D3D11BootMain, D3D11BootWindow, D3D11BootDevice, D3D11BootPipeline).
Wszystkie typy D3D11 pozostają lokalne w `src/d3d11_spike/` — żaden nie jest wyeksponowany przez publiczne nagłówki silnika.
Zachowanie spike'a jest identyczne jak przed refaktoryzacją.
Aktywne renderowanie runtime nadal jest raylibowe.
`D3D11Renderer` nadal **nie jest zaimplementowany**.

## 7) First safe implementation pass (pierwszy bezpieczny pass – rekomendacja)

Następny pass w kodzie (nie w ramach tej dokumentacji) powinien być mały, odwracalny i nie zmieniać zachowania:

- **Recommended (Safe, prep-only)**: wprowadzić wybór backendu jako konfigurację/opcje (np. struktura `BackendOptions`) i „boundary interfaces” typu `IPlatform` / `IInputReader` / `IRenderer` / `IPhysicsWorld`, ale z jedyną realną implementacją opartą o raylib + custom collision.
- **Do not do yet**: nie dodawać D3D11 ani PhysX w tym kroku.
- **Must preserve**: zachować wszystkie testy, walidatory i cały workflow smoke/build (`DEVELOPMENT.md`).

## 8) Verification commands (komendy weryfikacji)

Te komendy są częścią prawdy operacyjnej i muszą dalej działać:

```powershell
cmake --preset ci-core
cmake --build --preset ci-core
ctest --preset ci-core
cmake --preset ci
cmake --build --preset ci
ctest --preset ci
.\tools\ci_smoke.ps1 -Preset ci
```

## 9) Forbidden first-pass changes

During backend modernization prep, do not:

- add DirectX 11 code;
- add PhysX/Jolt code;
- remove raylib;
- rewrite GameApp.cpp wholesale;
- rewrite GameRenderers.cpp wholesale;
- rewrite PlayerMotor, CameraRig, VehicleController, MissionController;
- change asset_manifest.txt format;
- change save/load format;
- change mission flow;
- change camera/movement/vehicle tuning;
- introduce a new ECS;
- introduce a render graph;
- introduce PBR/material pipeline;
- silently change run scripts or CMake presets.

## 10) Success criteria for first backend prep pass

A first backend-prep code pass is successful only if:

- gameplay behavior is unchanged;
- raylib remains the only active renderer;
- custom WorldCollision remains the only active physics backend;
- all existing CI/core/game tests still pass;
- smoke workflow still launches the same playable slice;
- new abstractions do not expose raylib types in public backend-neutral headers;
- new docs do not claim D3D11/PhysX exists;
- any planned backend value is clearly marked as planned/not implemented.

## 11) Backend matrix

| Renderer | Physics | Status |
|---|---|---|
| raylib | custom WorldCollision | Implemented / current baseline |
| d3d11 | custom WorldCollision | Standalone boot spike only; production renderer NOT implemented |
| raylib | physx | Aspirational / planned (NOT implemented) |
| d3d11 | physx | Aspirational / long-term target (NOT implemented) |

## 12) Do not port raylib mental model into DX11

Future D3D11 work must not recreate raylib immediate-mode patterns such as:

- D3D11DrawCubeV
- D3D11DrawSphere
- D3D11DrawModelEx
- push/pop matrix transform stack
- renderer-global static draw calls

The target direction is:
game/runtime state -> render extraction -> RenderFrame/RenderCommandList -> backend renderer.