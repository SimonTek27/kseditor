cmake_minimum_required(VERSION 3.22)
project(kseditor_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOTEST_TARGETS_WATCH "")

if(MSVC)
    set(QT6_PATH "C:/Qt/6.11.1/msvc2022_64" CACHE PATH "Qt6 path")
else()
    set(QT6_PATH "C:/Qt/6.11.1/mingw_64" CACHE PATH "Qt6 path")
endif()
list(PREPEND CMAKE_PREFIX_PATH "${QT6_PATH}")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../external/vcpkg/installed/x64-windows")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../external/vcpkg/installed/x64-windows")
endif()

find_package(Qt6 COMPONENTS Core Test Network Widgets Multimedia OpenGL OpenGLWidgets Quick Qml QuickWidgets Quick3D Charts 3DCore 3DExtras 3DRender WebSockets REQUIRED)
find_package(Vulkan QUIET)
find_package(Python3 COMPONENTS Development QUIET)
include(CTest)

# Locate the prebuilt kseditor_lib produced by the main project build.
find_library(KSEDITOR_LIB
    NAMES kseditor_lib.lib libkseditor_lib.a
    HINTS
        "${CMAKE_CURRENT_SOURCE_DIR}/../build/Release"
        "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib/Release"
        "${CMAKE_CURRENT_SOURCE_DIR}/../build"
        "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Release"
    NO_DEFAULT_PATH
)
if(KSEDITOR_LIB)
    message(STATUS "Found kseditor_lib: ${KSEDITOR_LIB}")
else()
    message(WARNING "kseditor_lib not found - build the main project (cmake --build build) first")
endif()

set(VCPKG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/vcpkg/installed/x64-windows")
if(EXISTS "${VCPKG_DIR}/share/cgal/CGALConfig.cmake")
    list(APPEND CMAKE_PREFIX_PATH "${VCPKG_DIR}")
    set(CGAL_DIR "${VCPKG_DIR}/share/cgal")
    find_package(CGAL QUIET)
    if(CGAL_FOUND)
        add_compile_definitions(HAS_CGAL=1)
        include_directories("${VCPKG_DIR}/include")
        message(STATUS "CGAL found for test targets: ${CGAL_VERSION}")
    else()
        add_compile_definitions(HAS_CGAL=0)
        message(STATUS "CGAL not found for test targets")
    endif()
else()
    add_compile_definitions(HAS_CGAL=0)
    message(STATUS "vcpkg/CGAL not found, boolean ops tests will be limited")
endif()

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/../src")
set(COMMON "${SRC}/core")
set(MODULES "${SRC}/modules")
set(PHYSICS "${MODULES}/PhysicsEditor")

set(KS_TEST_INCLUDES
    ${SRC} ${COMMON} ${MODULES}
    ${SRC}/core ${SRC}/core/Config
    ${SRC}/core/FileFormat ${SRC}/core/sys ${SRC}/core/tools ${SRC}/core/editor
    ${SRC}/core/material ${SRC}/core/mesh ${SRC}/core/network ${SRC}/core/modmanager
    ${SRC}/core/assets ${SRC}/core/FfbEditor ${SRC}/core/Graphics
    ${SRC}/core/ppfiltersEditor ${SRC}/core/formatTools ${SRC}/core/Scripting
    ${SRC}/core/AIEditor ${SRC}/core/animation ${SRC}/core/workshop
    ${PHYSICS} ${PHYSICS}/SetupEditor
    ${MODULES}/soundEditor ${MODULES}/modellingEditor
    ${MODULES}/displayEditor ${MODULES}/LicensePlatesEditor
    ${MODULES}/fontEditor ${MODULES}/ppfiltersEditor
    ${MODULES}/FfbEditor ${MODULES}/ShowroomEditor
    ${MODULES}/AIEditor ${MODULES}/formatTools
    ${MODULES}/ServerConfigEditor
)

# Bullet Physics 3.25+ - use libraries from main build
if(HAS_BULLET)
    message(STATUS "Bullet found for tests (from main build)")
    add_compile_definitions(HAS_BULLET=1)
else()
    set(HAS_BULLET FALSE)
    message(STATUS "Bullet not found for tests")
    add_compile_definitions(HAS_BULLET=0)
endif()

# VHACD (convex decomposition, from Bullet Extras) - matches main build detection
set(VHACD_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/../external/bullet/Extras/VHACD/public/VHACD.h")
set(VHACD_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../build/vhacd/Release/VHACD.lib")
if(EXISTS "${VHACD_HEADER}" AND EXISTS "${VHACD_LIB}")
    set(HAS_VHACD TRUE)
    add_compile_definitions(HAS_VHACD=1)
    message(STATUS "VHACD found for tests: ${VHACD_LIB}")
else()
    set(HAS_VHACD FALSE)
    message(STATUS "VHACD not found for tests")
    add_compile_definitions(HAS_VHACD=0)
endif()

macro(ks_add_test TARGET_NAME)
    add_executable(${TARGET_NAME} ${ARGN})
    target_link_libraries(${TARGET_NAME} PRIVATE Qt6::Core Qt6::Test Qt6::Network Qt6::Widgets Qt6::Multimedia Qt6::OpenGL ${KSEDITOR_LIB})
    target_include_directories(${TARGET_NAME} PRIVATE ${KS_TEST_INCLUDES})
    if(WIN32)
        target_compile_definitions(${TARGET_NAME} PRIVATE WINVER=0x0A00 _WIN32_WINNT=0x0A00)
    endif()
    add_test(NAME ${TARGET_NAME}
             COMMAND ${TARGET_NAME} -v2 -o ${CMAKE_BINARY_DIR}/${TARGET_NAME}.xml,xunitxml)
    set_tests_properties(${TARGET_NAME} PROPERTIES TIMEOUT 60)
endmacro()

ks_add_test(test_boolean_ops  unit/test_BooleanOps.cpp
    ${MODULES}/modellingEditor/BooleanOps.cpp
)
if(CGAL_FOUND)
    target_link_libraries(test_boolean_ops PRIVATE CGAL::CGAL)
endif()
ks_add_test(test_all_physics unit/test_PhysicsSimulator.cpp
    ${PHYSICS}/PhysicsSimulator.cpp
    ${PHYSICS}/PhysicsProfiler.cpp
    ${PHYSICS}/tire_PacejkaTireModel.cpp
    ${PHYSICS}/eng_EngineModel.cpp
    ${PHYSICS}/dt_DifferentialModel.cpp
    ${PHYSICS}/mech_BrakeThermalModel.cpp
    ${PHYSICS}/mech_SuspensionModel.cpp
    ${PHYSICS}/aero_AeroModel.cpp
    ${PHYSICS}/PhysicsEngine.cpp
    ${PHYSICS}/ers_HybridSystem.cpp
)
ks_add_test(test_all_ai      unit/test_AllAI.cpp
    ${COMMON}/AIEditor/AITelemetryTrainer.cpp
    ${COMMON}/AIEditor/AiBehaviorModel.cpp
    ${COMMON}/AIEditor/MultiCarAI.cpp
)
ks_add_test(test_ai_behavior  unit/test_AI.cpp
    ${COMMON}/AIEditor/AiBehaviorModel.cpp
)
ks_add_test(test_telemetry_feedback unit/test_TelemetryFeedback.cpp
    ${PHYSICS}/telemetry/TelemetryFeedbackBridge.cpp
    ${PHYSICS}/telemetry/TelemetryQmlBridge.cpp
    ${PHYSICS}/PhysicsSimulator.cpp
    ${PHYSICS}/PhysicsProfiler.cpp
    ${PHYSICS}/tire_PacejkaTireModel.cpp
    ${PHYSICS}/eng_EngineModel.cpp
    ${PHYSICS}/dt_DifferentialModel.cpp
    ${PHYSICS}/mech_BrakeThermalModel.cpp
    ${PHYSICS}/mech_SuspensionModel.cpp
    ${PHYSICS}/aero_AeroModel.cpp
    ${PHYSICS}/PhysicsEngine.cpp
    ${PHYSICS}/ers_HybridSystem.cpp
)
ks_add_test(test_physics_profiler unit/test_PhysicsProfiler.cpp
    ${PHYSICS}/PhysicsProfiler.cpp
)
ks_add_test(test_laptime_validation unit/test_LapTimeValidation.cpp
    ${PHYSICS}/LapTimeValidation.cpp
)
ks_add_test(test_license_plates unit/test_LicensePlates.cpp
    ${COMMON}/editor/EditorModule.cpp
    ${PHYSICS}/../LicensePlatesEditor/LicensePlateEditorModule.cpp
    ${PHYSICS}/../LicensePlatesEditor/QRCodeWriter.cpp
)
ks_add_test(test_display_editor unit/test_DisplayEditor.cpp
    ${MODULES}/displayEditor/DisplayEditor.cpp
    ${MODULES}/displayEditor/DisplayEditorQmlBridge.cpp
    ${MODULES}/displayEditor/LuaExpressionEvaluator.cpp
)
ks_add_test(test_ppfilter_color_grading unit/test_PPFilterColorGrading.cpp
    ${COMMON}/ppfiltersEditor/PPFilterColorGrading.cpp
)
ks_add_test(test_modmanager_deps unit/test_ModManagerDeps.cpp)
ks_add_test(test_modmanager_features unit/test_ModManagerFeatures.cpp
    ${COMMON}/modmanager/ModCollection.cpp
)
ks_add_test(test_font_generator unit/test_FontGenerator.cpp
    ${MODULES}/fontEditor/FontGenerator.cpp
)
ks_add_test(test_audio_effects_advanced unit/test_AudioEffectsAdvanced.cpp)
ks_add_test(test_content_repair unit/test_ContentRepair.cpp
    ${COMMON}/modmanager/KsContentRepair.cpp
    ${COMMON}/modmanager/ContentRepairTool.cpp
    ${COMMON}/sys/LogManager.cpp
    ${COMMON}/editor/EditorModule.cpp
    ${COMMON}/editor/EditorConfig.cpp
    ${COMMON}/assets/Paths.cpp
)
ks_add_test(test_csp_config unit/test_CspConfig.cpp
    ${COMMON}/Config/CspConfigParser.cpp
)
ks_add_test(test_workshop unit/test_Workshop.cpp
    ${COMMON}/workshop/WorkshopManager.cpp
    ${COMMON}/sys/LogManager.cpp
)
ks_add_test(test_physics_mesh unit/test_PhysicsMeshGenerator.cpp
    ${MODULES}/modellingEditor/PhysicsMeshGenerator.cpp
    ${COMMON}/mesh/MeshOperations.cpp
    ${MODULES}/modellingEditor/BooleanOps.cpp
)
if(CGAL_FOUND)
    target_link_libraries(test_physics_mesh PRIVATE CGAL::CGAL)
endif()
if(HAS_BULLET)
    target_link_libraries(test_physics_mesh PRIVATE
        BulletDynamics BulletCollision LinearMath
    )
    target_include_directories(test_physics_mesh PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../external/bullet/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/../external/bullet/Extras/VHACD/public"
    )
endif()
if(HAS_VHACD)
    target_link_libraries(test_physics_mesh PRIVATE "${VHACD_LIB}")
    target_include_directories(test_physics_mesh PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../external/bullet/Extras/VHACD/public"
    )
    # VHACD depends on Bullet (LinearMath / ConvexHullComputer)
    set(BULLET_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/bullet/Release")
    if(EXISTS "${BULLET_LIB_DIR}")
        target_link_libraries(test_physics_mesh PRIVATE
            "${BULLET_LIB_DIR}/BulletCollision.lib"
            "${BULLET_LIB_DIR}/BulletDynamics.lib"
            "${BULLET_LIB_DIR}/LinearMath.lib"
        )
    endif()
endif()
# Test subdivision - uses fallback when OpenSubdiv not available
ks_add_test(test_subdivision unit/test_SubdivisionSurface.cpp
    ${COMMON}/mesh/SubdivisionSurface.cpp
)
ks_add_test(test_texture_paint unit/test_TexturePaintSystem.cpp
    ${COMMON}/material/TexturePaintSystem.cpp
)
ks_add_test(test_lod_generator unit/test_LODGenerator.cpp
    ${COMMON}/mesh/MeshOperations.cpp
    ${MODULES}/modellingEditor/BooleanOps.cpp
)
if(CGAL_FOUND)
    target_link_libraries(test_lod_generator PRIVATE CGAL::CGAL)
endif()
ks_add_test(test_asset_dedup unit/test_AssetDedup.cpp
    ${COMMON}/assets/AssetManager.cpp
    ${COMMON}/assets/AssetFileWatcher.cpp
    ${COMMON}/assets/AssetSearchEngine.cpp
)
ks_add_test(test_sculpt_mode_minimal unit/test_SculptMode_minimal.cpp
)

ks_add_test(test_sculpt_mode unit/test_SculptMode.cpp
    ${COMMON}/mesh/SculptMode.cpp
)
ks_add_test(test_mesh_operations unit/test_MeshOperations.cpp
    ${COMMON}/mesh/MeshOperations.cpp
)
if(CGAL_FOUND)
    target_sources(test_mesh_operations PRIVATE ${MODULES}/modellingEditor/BooleanOps.cpp)
    target_link_libraries(test_mesh_operations PRIVATE CGAL::CGAL)
endif()
ks_add_test(test_advanced_sculpt unit/test_AdvancedSculpt.cpp
    ${COMMON}/mesh/AdvancedSculpt.cpp
    ${COMMON}/mesh/SculptMode.cpp
)
ks_add_test(test_animation_system unit/test_AnimationSystem.cpp
    ${COMMON}/animation/AnimationSystem.cpp
    ${COMMON}/editor/TimelineEditor.cpp
)
ks_add_test(test_material_system unit/test_MaterialSystem.cpp
    ${COMMON}/material/MaterialSystem.cpp
)
ks_add_test(test_theme_system unit/test_ThemeSystem.cpp
    ${COMMON}/tools/ThemeSystem.cpp
)
ks_add_test(test_undo_redo unit/test_UndoRedo.cpp
    ${COMMON}/sys/UndoStack.cpp
)
ks_add_test(test_validation_system unit/test_ValidationSystem.cpp
    ${COMMON}/tools/QualitySystem.cpp
)
ks_add_test(test_state_machine unit/test_StateMachine.cpp
    ${COMMON}/sys/StateMachine.cpp
)
ks_add_test(test_command_palette unit/test_CommandPalette.cpp
    ${MODULES}/modellingEditor/CommandPalette.cpp
)
ks_add_test(test_cache_manager unit/test_CacheManager.cpp
    ${COMMON}/tools/CacheManager.cpp
)
ks_add_test(test_settings_system unit/test_SettingsSystem.cpp
    ${COMMON}/sys/SettingsManager.cpp
)
ks_add_test(test_backup_system unit/test_BackupSystem.cpp
    ${COMMON}/tools/BackupSystem.cpp
)
ks_add_test(test_recent_files unit/test_RecentFilesManager.cpp
    ${COMMON}/assets/RecentFilesManager.cpp
)
ks_add_test(test_notification_system unit/test_NotificationSystem.cpp
    ${COMMON}/tools/NotificationSystem.cpp
)
ks_add_test(test_auto_save unit/test_AutoSave.cpp
    ${COMMON}/tools/AutoSave.cpp
)
ks_add_test(test_showroom_editor unit/test_ShowroomEditor.cpp
    ${MODULES}/ShowroomEditor/ShowroomEditorModule.cpp
    ${MODULES}/ShowroomEditor/ShowroomSystem.cpp
    ${MODULES}/ShowroomEditor/ShowroomViewport3D.cpp
    ${COMMON}/editor/EditorModule.cpp
    ${COMMON}/editor/EditorConfig.cpp
    ${COMMON}/sys/LogManager.cpp
)

# kseditor_lib is a monolithic static library, so any test executable may pull in
# CGAL (BooleanOps) or 7-Zip (archive) object files via symbol references. Link
# those dependencies to every test target to avoid unresolved-external link errors.
set(SEVENZIP_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib/Release")
set(SEVENZIP_LIBS
    "${SEVENZIP_LIB_DIR}/7zip_archive.lib"
    "${SEVENZIP_LIB_DIR}/7zip_crypto.lib"
    "${SEVENZIP_LIB_DIR}/7zip_compress.lib"
    "${SEVENZIP_LIB_DIR}/7zip_common.lib"
    "${SEVENZIP_LIB_DIR}/7zip_cpp_common.lib"
    "${SEVENZIP_LIB_DIR}/7zip_win.lib"
    "${SEVENZIP_LIB_DIR}/7zip_c.lib"
)
set(BULLET_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/bullet/Release")
get_property(ALL_TEST_TARGETS DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(_t ${ALL_TEST_TARGETS})
    if(_t MATCHES "^test_")
        if(CGAL_FOUND)
            target_link_libraries(${_t} PRIVATE CGAL::CGAL)
        endif()
        if(EXISTS "${SEVENZIP_LIB_DIR}/7zip_archive.lib")
            target_link_libraries(${_t} PRIVATE ${SEVENZIP_LIBS})
        endif()
        if(HAS_VHACD AND EXISTS "${VHACD_LIB}")
            target_link_libraries(${_t} PRIVATE "${VHACD_LIB}")
            target_include_directories(${_t} PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/../external/bullet/Extras/VHACD/public")
        endif()
        if(EXISTS "${BULLET_LIB_DIR}/BulletCollision.lib")
            target_link_libraries(${_t} PRIVATE
                "${BULLET_LIB_DIR}/BulletCollision.lib"
                "${BULLET_LIB_DIR}/BulletDynamics.lib"
                "${BULLET_LIB_DIR}/LinearMath.lib")
        endif()
        target_link_libraries(${_t} PRIVATE Qt6::Quick Qt6::Qml Qt6::QuickWidgets Qt6::Quick3D Qt6::Charts
            Qt6::3DCore Qt6::3DExtras Qt6::3DRender Qt6::WebSockets)
        if(Python3_FOUND)
            target_link_libraries(${_t} PRIVATE Python3::Python)
        endif()
        if(Vulkan_FOUND)
            target_link_libraries(${_t} PRIVATE Vulkan::Vulkan)
        endif()
        target_link_libraries(${_t} PRIVATE opengl32 dwmapi)
        if(MSVC)
            # 7-Zip static lib duplicates IID symbols already present in kseditor_lib
            # (same situation handled in the main build via /FORCE:MULTIPLE).
            target_link_options(${_t} PRIVATE "LINKER:/FORCE:MULTIPLE")
        endif()
    endif()
endforeach()
