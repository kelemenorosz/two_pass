cmake_minimum_required(VERSION 3.20)

add_executable(
	main WIN32
	main.cpp
	application.cpp
	commandqueue.cpp
	game.cpp
	objects.cpp
	render.cpp
	sphereRender.cpp
	twoPassRender.cpp
	utils.cpp
	)
add_definitions(-DUNICODE)
set_property(TARGET main PROPERTY CXX_STANDARD 17)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/DirectX-Headers)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex)
target_link_libraries(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex/DirectXTex/Bin/Desktop_2022/x64/Debug/DirectXTex.lib)
install(TARGETS main DESTINATION ${CMAKE_SOURCE_DIR})

add_custom_target(shaders)
set(
	SHADER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/ComputeShader.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/PixelShader.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/TwoPassPixelShader.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/TwoPassPixelTextureShader.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/TwoPassVertexShader.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/VertexShader.hlsl
)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/shaders/ComputeShader.hlsl PROPERTIES ShaderType "cs")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/shaders/PixelShader.hlsl PROPERTIES ShaderType "ps")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/shaders/TwoPassPixelShader.hlsl PROPERTIES ShaderType "ps")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/shaders/TwoPassPixelTextureShader.hlsl PROPERTIES ShaderType "ps")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/shaders/TwoPassVertexShader.hlsl PROPERTIES ShaderType "vs")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/shaders/VertexShader.hlsl PROPERTIES ShaderType "vs")
set_source_files_properties(${SHADER_FILES} PROPERTIES ShaderModel "4_0")
foreach(FILE ${SHADER_FILES})
	get_filename_component(FILE_WE ${FILE} NAME_WE)
	get_source_file_property(shadertype ${FILE} ShaderType)
	get_source_file_property(shadermodel ${FILE} ShaderModel)
	add_custom_command(
		TARGET shaders
		COMMAND dxc.exe /nologo /Emain /T${shadertype}_${shadermodel} /Zi /Fo ${CMAKE_BINARY_DIR}/${FILE_WE}.cso /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE}
		MAIN_DEPENDENCY ${FILE}
		COMMENT "HLSL ${FILE}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
		VERBATIM
	)
endforeach(FILE)

add_dependencies(main shaders)

install(CODE "execute_process(COMMAND \${CMAKE_COMMAND} -E copy \${CMAKE_BINARY_DIR}/ComputeShader.cso \"${CMAKE_SOURCE_DIR}/\")")
install(CODE "execute_process(COMMAND \${CMAKE_COMMAND} -E copy \${CMAKE_BINARY_DIR}/PixelShader.cso \"${CMAKE_SOURCE_DIR}/\")")
install(CODE "execute_process(COMMAND \${CMAKE_COMMAND} -E copy \${CMAKE_BINARY_DIR}/TwoPassPixelShader.cso \"${CMAKE_SOURCE_DIR}/\")")
install(CODE "execute_process(COMMAND \${CMAKE_COMMAND} -E copy \${CMAKE_BINARY_DIR}/TwoPassPixelTextureShader.cso \"${CMAKE_SOURCE_DIR}/\")")
install(CODE "execute_process(COMMAND \${CMAKE_COMMAND} -E copy \${CMAKE_BINARY_DIR}/TwoPassVertexShader.cso \"${CMAKE_SOURCE_DIR}/\")")
install(CODE "execute_process(COMMAND \${CMAKE_COMMAND} -E copy \${CMAKE_BINARY_DIR}/VertexShader.cso \"${CMAKE_SOURCE_DIR}/\")")