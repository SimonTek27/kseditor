# Qt-free SimulatorApp executable — sources under src/simulator + engine natives.
# Included from the root CMakeLists.txt only when KSIMULATOR_QT_FREE is ON,
# after src/engine (ksengine) has been added.

if(NOT KSIMULATOR_QT_FREE)
	return()
endif()

find_package(Vulkan REQUIRED)

set(KSIM_SOURCES
	${CMAKE_SOURCE_DIR}/src/simulator/SimulatorApp.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/SimulationLoop.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/InputManager.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/CameraController.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/SimulatorAudio.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/AudioMixer.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/AudioBankManager.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/SoundsIniParser.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/NativeRenderer.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/ShadowSystem.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/DashboardOverlay.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/SetupGarage.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/TelemetryOverlay.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/TrackLoader.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/AIController.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/RaceSessionManager.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/MultiCarManager.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/PostProcessing.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/ReplayRecorder.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/GameMenuOverlay.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/NetworkManager.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/NetworkLowLevel.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/UdpTelemetryListener.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/TrackAudioManager.cpp
	${CMAKE_SOURCE_DIR}/src/simulator/WASAPIOutput.cpp
)

file(GLOB _ksim_ui_sources "${CMAKE_SOURCE_DIR}/src/simulator/ui/*.cpp")
list(APPEND KSIM_SOURCES ${_ksim_ui_sources})

set(_ksim_existing "")
foreach(f ${KSIM_SOURCES})
	if(EXISTS "${f}")
		list(APPEND _ksim_existing ${f})
	else()
		message(STATUS "SimulatorApp: skip missing ${f}")
	endif()
endforeach()

add_executable(SimulatorApp WIN32 ${_ksim_existing})
set_target_properties(SimulatorApp PROPERTIES
	AUTOMOC OFF
	AUTOUIC OFF
	AUTORCC OFF
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_compile_definitions(SimulatorApp PRIVATE
	KSENGINE_QT_FREE=1
	HAS_VEHICLE_SIM=1
	HAS_FFB=1
	KSASSETTOCORSA_EXPORTS
)
target_include_directories(SimulatorApp PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/simulator
)
target_link_libraries(SimulatorApp PRIVATE ksengine ${Vulkan_LIBRARIES})
if(HAS_MIKKTSPACE)
	target_link_libraries(SimulatorApp PRIVATE mikktspace)
endif()
if(WIN32)
	target_link_libraries(SimulatorApp PRIVATE dinput8 dxguid xinput ole32 user32 gdi32 ws2_32 winmm)
endif()

# ---------------------------------------------------------------------------
# SPIR-V shaders: glslc -> ${CMAKE_BINARY_DIR}/shaders, copied next to the exe
# ---------------------------------------------------------------------------
find_program(KS_GLSLC glslc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
if(KS_GLSLC)
	set(_ksim_shader_sources
		${CMAKE_SOURCE_DIR}/src/engine/Graphics/shaders/ksShadow.vert
		${CMAKE_SOURCE_DIR}/src/engine/Graphics/shaders/ksShadow.frag
		${CMAKE_SOURCE_DIR}/src/simulator/shaders/native_forward.vert
		${CMAKE_SOURCE_DIR}/src/simulator/shaders/native_forward.frag
		${CMAKE_SOURCE_DIR}/src/simulator/shaders/gbuffer.vert
		${CMAKE_SOURCE_DIR}/src/simulator/shaders/gbuffer.frag
		${CMAKE_SOURCE_DIR}/src/simulator/shaders/deferred_lighting.vert
		${CMAKE_SOURCE_DIR}/src/simulator/shaders/deferred_lighting.frag
		${CMAKE_SOURCE_DIR}/src/simulator/shaders/taa.frag
		${CMAKE_SOURCE_DIR}/src/simulator/ui/shaders/ui.vert.glsl
		${CMAKE_SOURCE_DIR}/src/simulator/ui/shaders/ui.frag.glsl
	)
	set(_ksim_spv_files "")
	foreach(_shader ${_ksim_shader_sources})
		if(EXISTS "${_shader}")
			get_filename_component(_name "${_shader}" NAME)
			string(REGEX REPLACE "\\.glsl$" "" _name "${_name}")
			# .glsl files carry no stage in the extension: pass it explicitly.
			set(_stage "")
			if(_shader MATCHES "\\.glsl$")
				string(REGEX REPLACE "^(.*)([./])([a-z]+)\\.glsl$" "\\3" _stage "${_shader}")
				set(_stage "-fshader-stage=${_stage}")
			endif()
			set(_spv "${CMAKE_BINARY_DIR}/shaders/${_name}.spv")
			add_custom_command(OUTPUT "${_spv}"
				COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders"
				COMMAND ${KS_GLSLC} -O ${_stage} -o "${_spv}" "${_shader}"
				DEPENDS "${_shader}"
				COMMENT "glslc ${_name}.spv"
				VERBATIM)
			list(APPEND _ksim_spv_files "${_spv}")
		endif()
	endforeach()
	add_custom_target(ks_shaders DEPENDS ${_ksim_spv_files})
	add_dependencies(SimulatorApp ks_shaders)
	add_custom_command(TARGET SimulatorApp POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:SimulatorApp>/shaders"
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/shaders" "$<TARGET_FILE_DIR:SimulatorApp>/shaders"
		COMMENT "Installing .spv shaders next to SimulatorApp.exe")
else()
	message(STATUS "SimulatorApp: glslc not found, shaders will not be built")
endif()

message(STATUS "SimulatorApp: Qt-free runtime (src/simulator, render=src/engine)")
