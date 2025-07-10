cmake_minimum_required (VERSION 3.10)
project(OpenSiv3DSpineTest-deps)

# ファイル取得用。ビルド用途ではないので、取得完了後、VisualStudioでのビルド成功を確認したら
# outフォルダなどCMake関連の生成ファイルは手動で削除して下さい。

set(SIV3D_VERSION "0.6.15")
set(SIV3D_RESOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../App)
set(SIV3D_SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/OpenSiv3D_SDK_${SIV3D_VERSION})

set(SPINE_VERSION "4.1")
set(SPINE_RUNTIME_LANGUAGE "spine-cpp")
set(SPINE_RUNTIME_DIR ${CMAKE_CURRENT_LIST_DIR}/${SPINE_RUNTIME_LANGUAGE}-${SPINE_VERSION})

set(IMGUI_VERSION "1.92.0")
set(IMGUI_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/imgui-${IMGUI_VERSION})

set(TEMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/temp)

function(download_and_extract HOST_URL ARCHIVE_NAME)
	if (NOT EXISTS "${TEMP_DIR}/${ARCHIVE_NAME}.zip")
		file(DOWNLOAD "${HOST_URL}" "${TEMP_DIR}/${ARCHIVE_NAME}.zip" SHOW_PROGRESS)
	endif()

	if (NOT EXISTS "${TEMP_DIR}/${ARCHIVE_NAME}")
		file(ARCHIVE_EXTRACT INPUT "${TEMP_DIR}/${ARCHIVE_NAME}.zip" DESTINATION "${TEMP_DIR}")
	endif()
endfunction()

# OpenSiv3D 埋め込みファイル取得
if (NOT EXISTS "${SIV3D_RESOURCE_DIR}")
	set(SIV3D_RESOURCE_FOLDER "OpenSiv3D_${SIV3D_VERSION}")
	set(SIV3D_RESOURCE_URL "https://siv3d.jp/downloads/Siv3D/manual/${SIV3D_VERSION}/${SIV3D_RESOURCE_FOLDER}.zip")
	download_and_extract(${SIV3D_RESOURCE_URL} ${SIV3D_RESOURCE_FOLDER} )

	file(COPY "${TEMP_DIR}/App/dll" DESTINATION "${SIV3D_RESOURCE_DIR}")
	file(COPY "${TEMP_DIR}/App/engine" DESTINATION "${SIV3D_RESOURCE_DIR}")
	file(COPY "${TEMP_DIR}/App/example" DESTINATION "${SIV3D_RESOURCE_DIR}")
	file(COPY "${TEMP_DIR}/App/icon.ico" DESTINATION "${SIV3D_RESOURCE_DIR}")
	file(COPY "${TEMP_DIR}/App/Resource.rc" DESTINATION "${SIV3D_RESOURCE_DIR}")
endif()

# OpenSiv3D SDK取得
if (NOT EXISTS "${SIV3D_SDK_DIR}")
	set(SIV3D_SDK_FOLDER "OpenSiv3D_SDK_${SIV3D_VERSION}")
	set(SIV3D_SDK_URL "https://siv3d.jp/downloads/Siv3D/manual/${SIV3D_VERSION}/${SIV3D_SDK_FOLDER}.zip")
	download_and_extract(${SIV3D_SDK_URL} ${SIV3D_SDK_FOLDER})

	file(COPY "${TEMP_DIR}/${SIV3D_SDK_FOLDER}/addon" DESTINATION "${SIV3D_SDK_DIR}")
	file(COPY "${TEMP_DIR}/${SIV3D_SDK_FOLDER}/include" DESTINATION "${SIV3D_SDK_DIR}")
	file(COPY "${TEMP_DIR}/${SIV3D_SDK_FOLDER}/lib" DESTINATION "${SIV3D_SDK_DIR}")
endif()

# Spine ランタイム取得
if (NOT EXISTS "${SPINE_RUNTIME_DIR}")
	set (SPINE_RUNTIME_URL "https://github.com/EsotericSoftware/spine-runtimes/archive/refs/heads/${SPINE_VERSION}.zip")
	set (SPINE_FOLDER_NAME "spine-runtimes-${SPINE_VERSION}")
	download_and_extract("${SPINE_RUNTIME_URL}" "${SPINE_FOLDER_NAME}")

	file(COPY "${TEMP_DIR}/${SPINE_FOLDER_NAME}/${SPINE_RUNTIME_LANGUAGE}/${SPINE_RUNTIME_LANGUAGE}/src" DESTINATION "${SPINE_RUNTIME_DIR}")
	file(COPY "${TEMP_DIR}/${SPINE_FOLDER_NAME}/${SPINE_RUNTIME_LANGUAGE}/${SPINE_RUNTIME_LANGUAGE}/include" DESTINATION "${SPINE_RUNTIME_DIR}")
endif()

# Dear ImGui取得
if (NOT EXISTS "${IMGUI_SRC_DIR}")
	set(IMGUI_SRC_FOLDER "imgui-${IMGUI_VERSION}")
	set(IMGUI_SRC_URL "https://github.com/ocornut/imgui/archive/refs/tags/v${IMGUI_VERSION}.zip")
	download_and_extract(${IMGUI_SRC_URL} ${IMGUI_SRC_FOLDER})
	file(COPY "${TEMP_DIR}/${IMGUI_SRC_FOLDER}" DESTINATION "${CMAKE_CURRENT_LIST_DIR}")
endif()