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

## 3) Partial / aspirational status (jasno: co NIE jest gotowym runtime)

Poniższe są **Partial** albo **Aspirational** — nie ma dowodu w aktualnym CMake, testach i runtime, że istnieją jako kompletne systemy produkcyjne:

- **Partial**: produkcyjnie nazwany renderer **DirectX 11** (`D3D11Renderer`) ma prywatną ścieżkę inicjalizacji urządzenia/swapchain/RTV/depth oraz standalone smoke, który potrafi opcjonalnie narysować minimalny depth-tested `RenderFrame` Box i podstawowe `RenderFrame.debugLines` z widokiem/projekcją opartą o `RenderFrame.camera` (z fallbackiem na stałą kamerę dla pustych/domyślnych ramek), ale pełny backend rysujący komendy `RenderFrame` nadal nie jest zaimplementowany.
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
- **Partial/Aspirational (Planned runtime values; NOT active)**:
  - renderer: `d3d11` (`D3D11Renderer` private init/depth path plus tiny standalone-smoke Box/debug-line drawing subset with `RenderFrame.camera`-based view/projection and fallback camera; runtime selection/full primitive drawing not implemented)
  - fizyka: `physx`
- **Implemented (Current CLI surface)**:
  - `--renderer <raylib>` (planned value `d3d11` fails with a clear error)
  - `--physics <custom>` (planned value `physx` fails with a clear error)

Uwaga (explicit): **CLI flags do not mean runtime d3d11/physx are implemented**. To jest tylko plumbing pod przyszłe prace.

Backend prep status update: istnieją teraz backend-neutralne interfejsy `IPlatform` i `IInputReader`.
Obecnie jedynymi implementacjami runtime są `RaylibPlatform` i `RaylibInputReader`.
To nadal **nie oznacza**, że runtime D3D11 ani PhysX są zaimplementowane.

Backend prep status update: istnieje teraz backend-neutralny szkielet `RenderFrame` / listy komend renderu.
Obecny renderer runtime nadal jest raylibowy i nadal używa istniejących klas `WorldRenderer`, `HudRenderer` i `DebugRenderer`.
Runtime D3D11 nadal **nie jest zaimplementowany**.
Przyszłe prace nad rendererem powinny konsumować `RenderFrame`, zamiast kopiować raylibowe natychmiastowe wywołania typu `DrawCubeV`, `DrawSphere`, `DrawModelEx` albo stos `rlPushMatrix`.

Backend prep status update: istnieje teraz częściowa ścieżka ekstrakcji `WorldRenderList` -> `RenderFrame` używana tylko w testach.
To nadal jest eksperymentalna/data-only ścieżka graniczna; aktywny renderer runtime pozostaje raylibowy.
Runtime D3D11 nadal **nie jest zaimplementowany**.
Ta ekstrakcja jest krokiem pośrednim pod przyszły backend renderera, nie zamianą obecnego renderowania.

Backend prep status update: `WorldRenderList` został przeniesiony do małego, backend-neutralnego nagłówka `include/bs3d/render/WorldRenderList.h`.
`RenderExtraction` nie zależy już od `GameRenderers.h`.
Runtime renderer nadal jest raylibowy.
Runtime D3D11 nadal **nie jest zaimplementowany**.

Backend prep status update: istnieje teraz backend-neutralny `RenderFrameBuilder` (`include/bs3d/render/RenderFrameBuilder.h`).
`RenderFrameBuilder` akumuluje komendy prymitywów wg bucketu i buduje `RenderFrame` w produkcyjnej kolejności renderowania.
Jest helperem kontraktowym/testowym — **nie jest render grafem i sam nic nie rysuje**.
Aktywne renderowanie runtime nadal jest raylibowe.
Runtime D3D11 nadal **nie jest zaimplementowany**.

Backend prep status update: istnieje teraz `NullRenderer` (`include/bs3d/render/NullRenderer.h`) jako backend-neutralny no-op/testowy renderer implementujący `IRenderer`.
`NullRenderer` nie otwiera okna, nie używa GPU i zapisuje statystyki oraz wyniki walidacji każdego skonsumowanego `RenderFrame`.
Istnieje szkielet `RendererFactory` (`include/bs3d/render/RendererFactory.h`) jako przyszły seam do tworzenia rendererów.
Aktualnie działające ścieżki fabryki to `NullRenderer` (przez `useNullRenderer = true`) oraz eksperymentalna ścieżka `D3D11Renderer` (przez `allowExperimentalD3D11Renderer = true`, tylko dla testów/narzędzi, zwraca niezainicjalizowany renderer bez device/window/swapchain).
Implementacja fabryki w `src/render/RendererFactory.cpp` (nie inline w nagłówku).
Raylib `IRenderer` adapter **nie jest zaimplementowany** — legacy runtime nadal używa `WorldRenderer`/`HudRenderer`/`DebugRenderer`.
Runtime D3D11 nadal **nie jest zaimplementowany**.
Aktywne renderowanie runtime nadal jest raylibowe.

Backend prep status update: istnieje teraz Windows-only, produkcyjnie nazwany `D3D11Renderer` (`src/render_d3d11/D3D11Renderer.h/.cpp`) z pierwszą prywatną ścieżką inicjalizacji D3D11.
`D3D11Renderer` implementuje `IRenderer`, zwraca nazwę backendu `d3d11`, waliduje i podsumowuje przekazany `RenderFrame`, oraz zapisuje ostatnie statystyki/wynik walidacji do inspekcji testowej.
Może teraz prywatnie utworzyć `ID3D11Device`, `ID3D11DeviceContext`, `IDXGISwapChain`, `ID3D11RenderTargetView`, depth texture, `ID3D11DepthStencilView`, `ID3D11DepthStencilState` oraz minimalne zasoby pipeline'u Box/debug-lines: in-memory shader, input layout, vertex/index buffer, dynamic line vertex buffer i dynamic constant buffer.
Po inicjalizacji `renderFrame()` czyści render target i depth buffer, rysuje depth-tested wspierane komendy `RenderPrimitiveKind::Box` z bucketów Opaque/Vehicle/Debug, rysuje podstawowe depth-tested `RenderFrame.debugLines`, a następnie presentuje swapchain.
Ten path jest weryfikowany tylko przez standalone `bs3d_d3d11_renderer_smoke --box` / `--two-boxes` / `--debug-lines`; nie konsumuje realnych danych runtime i nie ma mesh/textures/shader-file/material pipeline.
Nie jest podłączony do `GameApp`, `RendererFactory` ani CLI wyboru backendu.
Aktywne renderowanie runtime nadal jest raylibowe.
Standalone `bs3d_d3d11_boot` pozostaje osobną ścieżką hardcoded cube spike.
Standalone `bs3d_d3d11_renderer_smoke` istnieje jako smoke init/clear/depth/present oraz opcjonalny minimalny Box/debug-line smoke dla `D3D11Renderer`.

Backend prep status update: `D3D11Renderer` smoke renderuje teraz Box i debug-line prymitywy używając widoku/projekcji opartej o `RenderFrame.camera`.
Jeśli `RenderFrame.camera` jest domyślna/nieprawidłowa (fovy ≤ 0, pozycja = cel, up zerowy), renderer stosuje wbudowaną fallback kamerę (Z-offset + 65° FOV).
Standalone `bs3d_d3d11_renderer_smoke --camera` ćwiczy ścieżkę kamery z nie-domyślną `RenderCamera`.
Nie jest to pełny system kamery, integracja runtime, ani podłączenie do `GameApp`.
Runtime renderer nadal jest raylibowy.
Runtime D3D11 nadal **nie jest zaimplementowany**.

Backend prep status update: `D3D11Renderer` smoke może teraz konsumować `RenderFrame` zbudowany przez `RenderFrameBuilder` (`--builder-frame`).
Builder tworzy ramkę z co najmniej 2 Box primitives w obsługiwanych bucketach (Opaque, Vehicle), co najmniej 3 debug lines i waliduje ramkę przed wysłaniem.
To ćwiczy ścieżkę kontraktu renderu: `RenderFrameBuilder` → `RenderFrame` → `D3D11Renderer` smoke, bez ręcznego konstruowania `RenderFrame`.
Runtime renderer nadal jest raylibowy.
`GameApp` nadal nie jest podłączony do D3D11.
`RendererFactory` nie jest podłączony do D3D11.

Backend prep status update: `RendererBackendKind` wymienia teraz `D3D11` obok `Raylib`.
`rendererBackendName(D3D11)` zwraca `"d3d11"`.
`RendererFactory` ma teraz eksperymentalną ścieżkę D3D11 dla testów/narzędzi, aktywowaną przez `allowExperimentalD3D11Renderer = true`.
Zwrócony `D3D11Renderer` jest niezainicjalizowany — fabryka nie tworzy device/window/swapchain.
Bez jawnego opt-in (`allowExperimentalD3D11Renderer = false`) ścieżka D3D11 zwraca czytelny błąd "experimental and not active for runtime".
Implementacja fabryki znajduje się w `src/render/RendererFactory.cpp` (nie jest już inline w nagłówku).
Nagłówek `RendererFactory.h` pozostaje backend-neutralny — nie inkluduje D3D11Renderer.h, windows.h, d3d11.h, raylib.h.
`--renderer d3d11` w `main.cpp` nadal **nie jest aktywny** — CLI gry nadal odrzuca backend `d3d11`.
Runtime renderer nadal jest raylibowy.
`GameApp` nadal nie jest podłączony do D3D11.

Backend prep status update: `bs3d_d3d11_renderer_smoke --factory` ćwiczy teraz ścieżkę `RendererFactory` → `D3D11Renderer`.
Smoke z `--factory` tworzy renderer przez `createRenderer(D3D11 + allowExperimentalD3D11Renderer)`, weryfikuje `result.ok()`, `backendName()`, `dynamic_cast` do `D3D11Renderer*`, a następnie inicjalizuje i renderuje ramki.
Bez `--factory` smoke konstruuje `D3D11Renderer` bezpośrednio na stosie — obie ścieżki są zachowane.
`--renderer d3d11` nadal **nie jest aktywny** w grze.
Runtime renderer nadal jest raylibowy.

Backend prep status update: `D3D11Renderer` smoke wspiera teraz szerszy zestaw bucketów Box: Opaque, Vehicle, Decal, Glass, Translucent, Debug.
Bucket'y Glass i Translucent używają minimalnego alpha blendingu przez osobny `ID3D11BlendState`; pozostałe bucket'y renderują jako opaque.
Nadal pomijane: Sky, Ground, Hud, oraz nieobsługiwane typy prymitywów (Mesh, Sphere, CylinderX, QuadPanel).
Brak systemu materiałów, sortowania przezroczystych, wsparcia tekstur.
Runtime renderer nadal jest raylibowy.
`GameApp` nadal nie jest podłączony do D3D11.

Backend prep status update: `GameApp` ma teraz tryb `--renderframe-shadow` (dev/test-only).
Po włączeniu, co klatkę (lub co 120 klatek) `GameApp` buduje shadow `RenderFrame` z `WorldRenderList` przez `RenderFrameBuilder` + `RenderExtraction`, waliduje go przez `validateRenderFrame()`, podsumowuje przez `summarizeRenderFrame()`, i podaje do `NullRenderer`.
Nie używa D3D11Renderer, nie zmienia wizualnego wyjścia, nie modyfikuje zachowania gameplay.
Shadow frame zawiera tylko fallback boxy z live `WorldRenderList`; pomija HUD, cząstki, pojazdy i postacie.
Runtime renderer nadal jest raylibowy.
`--renderer d3d11` nadal **nie jest aktywny** w grze.
`RendererFactory`/`BackendKinds` pozostają w ścieżce testowej/narzędziowej.

Backend prep status update: istnieje teraz `--d3d11-shadow-window` (dev-only, Windows-only).
Po włączeniu, `GameApp` otwiera osobne okno Win32 (640x360, "Blok 13 D3D11 Shadow"), inicjalizuje `D3D11Renderer` przez `RendererFactory`, i renderuje shadow `RenderFrame` w tym bocznym oknie.
Główne renderowanie raylib pozostaje bez zmian; D3D11 nie zastępuje `GameApp` renderingu.
Sidecar renderuje tylko wspierany podzbiór `RenderFrame` (Box + debug lines) z danych `WorldRenderList`.
`D3D11ShadowSidecar` jest prywatnym helperem w `src/game/`; nie wystawia typów D3D11/Win32 w publicznych nagłówkach.
Zamknięcie okna sidecara (X) wyłącza sidecar bez zamykania głównego okna raylib — game kontynuuje normalne działanie.
`isCloseRequested()` / `hasError()` umożliwiają inspekcję stanu sidecara.
Diagnostyka D3D11 shadow zawiera teraz pokrycie rysowania D3D11: `drawnBoxes`, `skippedUnsupportedKinds`, `skippedUnsupportedBuckets`, `skippedPrimitives`, `drawnDebugLines`, `skippedDebugLines`.
Pokrycie D3D11 jest raportowane tylko gdy sidecar jest zainicjalizowany; przy niezainicjalizowanym rendererze wszystkie liczniki są zero.
`tools/d3d11_shadow_smoke.ps1` automatyzuje weryfikację obu ścieżek shadow sidecar (z i bez okna D3D11).
Na non-Windows flaga jest akceptowana ale sidecar zwraca błąd "Windows-only" i kontynuuje działanie gry.
`--d3d11-shadow-diagnostics` rozszerza log o szczegółowe statystyki bucketa (opaque/vehicle/decal/glass/translucent/debug), status inicjalizacji sidecara, renderCalls i lastFrameValid.
Diagnostics loguje PO sidecar.submit(), więc sidecarCalls/lastFrameValid odzwierciedlają właśnie wysłaną ramkę.
Log zawiera też liczniki built/submitted shadow frames.
Interwał budowania shadow frame jest konfigurowalny przez `--renderframe-shadow-interval <count>` (domyślnie 120, minimum 1).
Runtime renderer nadal jest raylibowy.
`--renderer d3d11` nadal **nie jest aktywny** w grze.

Backend prep status update: `D3D11Renderer` smoke może teraz konsumować `RenderFrame` zbudowany przez `WorldRenderList`-style extraction (`--extraction-frame`).
Smoke tworzy lokalne `WorldObject`/`WorldAssetDefinition`, konstruuje `WorldRenderList`, przepuszcza dane przez `RenderExtraction::addWorldRenderListFallbackBoxes(RenderFrameBuilder&)` do `RenderFrameBuilder`, a następnie do `D3D11Renderer`.
Ścieżka: WorldRenderList-style data → RenderExtraction → RenderFrameBuilder → RenderFrame → D3D11Renderer smoke.
Runtime renderer nadal jest raylibowy.
`GameApp` nadal nie jest podłączony do D3D11.
`RendererFactory`/`BackendKinds` nadal nie są podłączone.

Backend prep status update: istnieje teraz backend-neutralny interfejs `IRenderer`, który konsumuje `RenderFrame`.
Testowy `RecordingRenderer` istnieje tylko w testach i zapisuje wyłącznie liczniki komend.
Aktywne renderowanie runtime nadal przechodzi przez raylibowe `WorldRenderer`, `HudRenderer` i `DebugRenderer`.
Runtime D3D11 nadal **nie jest zaimplementowany**.
Przyszłe backendy renderera powinny implementować `IRenderer` i konsumować `RenderFrame`.

Backend prep status update: istnieją teraz backend-neutralne helpery statystyk i walidacji `RenderFrame`.
Są używane w testach do sprawdzania liczby komend i kolejności bucketów bez GPU ani runtime renderera.
Aktywne renderowanie runtime nadal jest raylibowe.
Runtime D3D11 nadal **nie jest zaimplementowany**.
Przyszłe backendy renderera powinny walidować i podsumowywać `RenderFrame` przed rysowaniem.

Backend spike status update: istnieje teraz samodzielny, Windows-only spike `bs3d_d3d11_boot`.
Ten target tworzy okno Win32, inicjalizuje Direct3D 11, czyści, rysuje hardcoded indexed colored cube, presentuje swapchain przez krótki przebieg i kończy działanie.
`bs3d_d3d11_boot` potwierdza teraz pierwszą ścieżkę rysowania D3D11: shader, input layout, vertex buffer, index buffer i draw call dla hardcoded colored cube.
`bs3d_d3d11_boot` potwierdza też minimalną ścieżkę constant-buffer update i model/view/projection transform w shaderze dla tego standalone spike.
`bs3d_d3d11_boot` potwierdza teraz minimalną ścieżkę depth buffer + depth-stencil state (D24_UNORM_S8_UINT, DepthEnable/LESS, clear depth co klatkę).
`bs3d_d3d11_boot` potwierdza teraz minimalną ścieżkę indexed geometry: vertex buffer + index buffer (uint16_t, TRIANGLELIST) dla hardcoded colored cube.
Nie jest podłączony do runtime renderingu, `GameApp`, `IRenderer` ani `RendererFactory`.
Nie jest współdzielony jako publiczne API silnika z produkcyjnym szkieletem `D3D11Renderer`.
Aktywne renderowanie runtime nadal jest raylibowe.

Backend spike status update: `bs3d_d3d11_boot` został zrefaktoryzowany z jednego monolitycznego pliku na lokalne moduły spike (D3D11BootMain, D3D11BootWindow, D3D11BootDevice, D3D11BootPipeline).
Wszystkie typy D3D11 pozostają lokalne w `src/d3d11_spike/` — żaden nie jest wyeksponowany przez publiczne nagłówki silnika.
Zachowanie spike'a jest identyczne jak przed refaktoryzacją.
Aktywne renderowanie runtime nadal jest raylibowe.
`D3D11Renderer` istnieje jako osobny produkcyjnie nazwany renderer z prywatną ścieżką init/clear/present, bez współdzielenia klas spike'a jako publicznego API.

## 7) First safe implementation pass (pierwszy bezpieczny pass – rekomendacja)

Następny pass w kodzie (nie w ramach tej dokumentacji) powinien być mały, odwracalny i nie zmieniać zachowania:

- **Recommended (Safe, prep-only)**: wprowadzić wybór backendu jako konfigurację/opcje (np. struktura `BackendOptions`) i „boundary interfaces” typu `IPlatform` / `IInputReader` / `IRenderer` / `IPhysicsWorld`, ale z jedyną realną implementacją opartą o raylib + custom collision.
- **Do not do yet**: nie podłączać runtime D3D11 do `GameApp`, nie rysować prymitywów `RenderFrame` przez D3D11, ani nie dodawać PhysX w tym kroku.
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

- add DirectX 11 device/window/drawing integration outside explicitly scoped spike/skeleton work;
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
- new docs do not claim runtime D3D11/PhysX exists;
- any planned backend value is clearly marked as planned/not implemented.

## 11) Backend matrix

| Renderer | Physics | Status |
|---|---|---|
| raylib | custom WorldCollision | Implemented / current baseline |
| d3d11 | custom WorldCollision | Standalone boot spike draws; production `D3D11Renderer` can init/clear/depth/present and draw a tiny depth-tested Box/debug-line subset with `RenderFrame.camera`-based view/projection (with fallback camera for default frames) in smoke while recording `RenderFrame` stats/validation; runtime renderer NOT implemented |
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