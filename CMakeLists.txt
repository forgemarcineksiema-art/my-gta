cmake_minimum_required(VERSION 3.25)

project(BlokowaSatyra3D VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BS3D_BUILD_GAME "Build the raylib desktop game executable" ON)
option(BS3D_ENABLE_DEV_TOOLS "Enable in-game QA/dev shortcuts and overlays" OFF)

function(bs3d_enable_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(bs3d_core
    src/core/CharacterAction.cpp
    src/core/CharacterPhysicalReaction.cpp
    src/core/CharacterStatusPolicy.cpp
    src/core/CharacterVaultPolicy.cpp
    src/core/CharacterWorldHitPolicy.cpp
    src/core/CameraRig.cpp
    src/core/ChaseSystem.cpp
    src/core/ControlContext.cpp
    src/core/DialogueSystem.cpp
    src/core/DriveRouteGuide.cpp
    src/core/FixedTimestep.cpp
    src/core/GameFeedback.cpp
    src/core/InteractionResolver.cpp
    src/core/MissionController.cpp
    src/core/MissionTriggerSystem.cpp
    src/core/NpcReactionSystem.cpp
    src/core/ParagonMission.cpp
    src/core/ParagonMissionSpec.cpp
    src/core/PlayerMotor.cpp
    src/core/PlayerPresentation.cpp
    src/core/PlayerController.cpp
    src/core/PrzypalSystem.cpp
    src/core/RenderInterpolation.cpp
    src/core/Scene.cpp
    src/core/SaveGame.cpp
    src/core/VehicleController.cpp
    src/core/WorldEventLedger.cpp
    src/core/WorldRewirPressure.cpp
    src/core/WorldServiceState.cpp
    src/core/WorldInteraction.cpp
    src/core/WorldCollision.cpp
)

target_include_directories(bs3d_core PUBLIC include)

if(WIN32)
    add_library(bs3d_renderer_d3d11 STATIC
        src/render_d3d11/D3D11Renderer.cpp
        src/render_d3d11/D3D11MeshCache.cpp
        src/render_d3d11/D3D11MeshUploadAdapter.cpp
    )

    target_link_libraries(bs3d_renderer_d3d11 PUBLIC bs3d_core PRIVATE d3d11 dxgi d3dcompiler)
    target_include_directories(bs3d_renderer_d3d11 PUBLIC include PRIVATE src/render_d3d11 src/render)
    bs3d_enable_warnings(bs3d_renderer_d3d11)
endif()

add_executable(bs3d_tests
    tests/core_tests.cpp
)

target_link_libraries(bs3d_tests PRIVATE bs3d_core)
bs3d_enable_warnings(bs3d_core)
bs3d_enable_warnings(bs3d_tests)

enable_testing()
add_test(NAME bs3d_core_tests COMMAND bs3d_tests)

add_executable(bs3d_render_tests
    tests/render_frame_tests.cpp
    src/game/RenderExtraction.cpp
    src/render/CpuMeshData.cpp
    src/render/MeshRegistry.cpp
    src/render/MaterialRegistry.cpp
    src/render/RendererFactory.cpp
    src/render/RenderFrameDump.cpp
)

target_link_libraries(bs3d_render_tests PRIVATE bs3d_core)
target_include_directories(bs3d_render_tests PRIVATE include src/game src/render)
if(WIN32)
    target_link_libraries(bs3d_render_tests PRIVATE bs3d_renderer_d3d11)
    target_include_directories(bs3d_render_tests PRIVATE src/render_d3d11)
    target_compile_definitions(bs3d_render_tests PRIVATE BS3D_HAS_D3D11_RENDERER=1)
endif()
bs3d_enable_warnings(bs3d_render_tests)
add_test(NAME bs3d_render_frame_tests COMMAND bs3d_render_tests)

if(WIN32)
    add_executable(bs3d_d3d11_boot
        src/d3d11_spike/D3D11BootMain.cpp
        src/d3d11_spike/D3D11BootWindow.cpp
        src/d3d11_spike/D3D11BootDevice.cpp
        src/d3d11_spike/D3D11BootPipeline.cpp
    )

    target_link_libraries(bs3d_d3d11_boot PRIVATE d3d11 dxgi d3dcompiler)
    bs3d_enable_warnings(bs3d_d3d11_boot)

    add_executable(bs3d_d3d11_renderer_smoke
        src/render_d3d11/D3D11RendererSmoke.cpp
        src/game/RenderExtraction.cpp
        src/render/RendererFactory.cpp
        src/render/RenderFrameDump.cpp
    )

    target_link_libraries(bs3d_d3d11_renderer_smoke PRIVATE bs3d_renderer_d3d11)
    target_include_directories(bs3d_d3d11_renderer_smoke PRIVATE include src/render_d3d11 src/game src/render)
    bs3d_enable_warnings(bs3d_d3d11_renderer_smoke)

    add_executable(bs3d_d3d11_game_shell
        src/render_d3d11/D3D11GameShell.cpp
        src/render/RenderFrameDump.cpp
    )

    target_link_libraries(bs3d_d3d11_game_shell PRIVATE bs3d_renderer_d3d11)
    target_include_directories(bs3d_d3d11_game_shell PRIVATE include src/render_d3d11 src/render)
    bs3d_enable_warnings(bs3d_d3d11_game_shell)
endif()

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME bs3d_asset_validation
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_assets.py ${CMAKE_SOURCE_DIR}/data/assets --quiet
    )
    add_test(NAME bs3d_asset_validator_unit_tests
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/test_validate_assets.py
    )
    add_test(NAME bs3d_editor_overlay_validation
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_editor_overlay.py
                ${CMAKE_SOURCE_DIR}/data/world/block13_editor_overlay.json
                --asset-root ${CMAKE_SOURCE_DIR}/data/assets
    )
    add_test(NAME bs3d_world_contract_validation
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_world_contract.py
                ${CMAKE_SOURCE_DIR}/data
                --asset-root ${CMAKE_SOURCE_DIR}/data/assets
                --quiet
    )
    add_test(NAME bs3d_world_contract_validator_unit_tests
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/test_validate_world_contract.py
    )
    add_test(NAME bs3d_object_outcome_validation
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_object_outcomes.py
                ${CMAKE_SOURCE_DIR}/data/world/object_outcome_catalog.json
                --runtime-policy ${CMAKE_SOURCE_DIR}/src/game/WorldObjectInteraction.cpp
                --quiet
    )
    add_test(NAME bs3d_object_outcome_validator_unit_tests
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/test_validate_object_outcomes.py
    )
    add_test(NAME bs3d_mission_validation
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_mission.py
                ${CMAKE_SOURCE_DIR}/data/mission_driving_errand.json
                --mission-localization ${CMAKE_SOURCE_DIR}/data/world/mission_localization_pl.json
                --outcome-catalog ${CMAKE_SOURCE_DIR}/data/world/object_outcome_catalog.json
                --runtime-policy ${CMAKE_SOURCE_DIR}/src/game/WorldDataLoader.cpp
                --quiet
    )
    add_test(NAME bs3d_mission_validator_unit_tests
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/test_validate_mission.py
    )
else()
    message(STATUS "Python3 interpreter not found; skipping Python validation CTests")
endif()

if(BS3D_BUILD_GAME)
    include(FetchContent)

    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    set(PLATFORM "Desktop" CACHE STRING "" FORCE)

    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
        URL_HASH SHA256=2b3ee1e2120c7a0796b33062c7e9a694dd8a8caa56a96319ac8c8ecf54a90d0b
    )
    FetchContent_MakeAvailable(raylib)

    if(BS3D_ENABLE_DEV_TOOLS)
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.92.7
        )
        FetchContent_MakeAvailable(imgui)

        FetchContent_Declare(
            rlimgui
            GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
            GIT_TAG Raylib_6_0
        )
        FetchContent_MakeAvailable(rlimgui)

        add_library(bs3d_imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${rlimgui_SOURCE_DIR}/rlImGui.cpp
        )
        target_include_directories(bs3d_imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${rlimgui_SOURCE_DIR}
        )
        target_link_libraries(bs3d_imgui PUBLIC raylib)
    endif()

    add_library(bs3d_game_support
        src/game/IntroLevelBuilder.cpp
        src/game/IntroLevelCoreLayout.cpp
        src/game/IntroLevelDressing.cpp
        src/game/IntroLevelGameplayData.cpp
        src/game/IntroLevelGroundTruthDressing.cpp
        src/game/IntroLevelIdentityDressing.cpp
        src/game/IntroLevelPresentation.cpp
        src/game/IntroLevelPresentationRender.cpp
        src/game/MissionRuntimeBridge.cpp
        src/game/MissionOutcomeTrigger.cpp
        src/game/PropSimulationSystem.cpp
        src/game/RaylibInputReader.cpp
        src/game/RaylibPlatform.cpp
        src/game/RenderExtraction.cpp
        src/game/RuntimeMapEditor.cpp
        src/game/GameHudPainter.cpp
        src/game/GameHudLayout.cpp
        src/game/GameHudTextMetrics.cpp
        src/game/GameHudRenderers.cpp
        src/game/GameRenderers.cpp
        src/game/RuntimeAudio.cpp
        src/game/VehicleArtModel.cpp
        src/game/VehicleExitResolver.cpp
        src/game/VisualBaselineDebug.cpp
        src/game/WorldAssetRegistry.cpp
        src/game/WorldEventEmitterBridge.cpp
        src/game/WorldGlassRendering.cpp
        src/game/WorldObjectInteraction.cpp
        src/game/ChaseAiRuntime.cpp
        src/game/ChaseVehicleRuntime.cpp
        src/game/CharacterArtModel.cpp
        src/game/CharacterDebugGeometry.cpp
        src/game/CharacterPosePreview.cpp
        src/game/DevTools.cpp
        src/game/EditorOverlayApply.cpp
        src/game/EditorOverlayCodec.cpp
        src/game/WorldDataLoader.cpp
    )

    target_link_libraries(bs3d_game_support PUBLIC bs3d_core raylib)
    target_include_directories(bs3d_game_support PUBLIC include src/game)
    target_compile_definitions(bs3d_game_support PUBLIC BS3D_ENABLE_DEV_TOOLS=$<BOOL:${BS3D_ENABLE_DEV_TOOLS}>)
    if(BS3D_ENABLE_DEV_TOOLS)
        target_sources(bs3d_game_support PRIVATE src/game/RuntimeMapEditorImGui.cpp)
        target_link_libraries(bs3d_game_support PUBLIC bs3d_imgui)
    endif()
    bs3d_enable_warnings(bs3d_game_support)

    add_executable(bs3d_game_tests
        tests/game_support_tests.cpp
    )

    target_link_libraries(bs3d_game_tests PRIVATE bs3d_game_support)
    bs3d_enable_warnings(bs3d_game_tests)
    add_test(NAME bs3d_game_support_tests COMMAND bs3d_game_tests)
    set_tests_properties(bs3d_game_support_tests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_executable(blokowa_satyra
        src/game/GameApp.cpp
        src/game/main.cpp
        src/render/RenderFrameDump.cpp
    )

    target_link_libraries(blokowa_satyra PRIVATE bs3d_core bs3d_game_support raylib)
    target_include_directories(blokowa_satyra PRIVATE include src/render)

    if(WIN32)
        target_sources(blokowa_satyra PRIVATE
            src/game/D3D11ShadowSidecar.cpp
            src/render/RendererFactory.cpp
        )
        target_link_libraries(blokowa_satyra PRIVATE bs3d_renderer_d3d11)
        target_include_directories(blokowa_satyra PRIVATE src/render_d3d11)
    endif()

    target_compile_definitions(blokowa_satyra PRIVATE BS3D_ENABLE_DEV_TOOLS=$<BOOL:${BS3D_ENABLE_DEV_TOOLS}>)
    bs3d_enable_warnings(blokowa_satyra)
    add_custom_command(TARGET blokowa_satyra POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/data
                $<TARGET_FILE_DIR:blokowa_satyra>/data
    )
endif()
