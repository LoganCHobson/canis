# Stages the runtime files Gradle packages into the APK and writes
# canis_files.txt, the list the Android runtime mirrors into internal storage.
# Each line is "<md5> <path>", so any changed asset changes the list and the
# next launch mirrors a fresh copy.
if(NOT IS_DIRECTORY "${ASSETS_DIR}")
    message(FATAL_ERROR "Android assets directory not found: '${ASSETS_DIR}'. Set CANIS_BUILD_ASSETS_DIRECTORY.")
endif()
if(NOT OUTPUT_DIR)
    message(FATAL_ERROR "CANIS_ANDROID_ASSETS_DIRECTORY is empty.")
endif()

file(REMOVE_RECURSE "${OUTPUT_DIR}")
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

file(COPY "${ASSETS_DIR}/" DESTINATION "${OUTPUT_DIR}/assets")
# Without a staged settings directory, package the whole folder that holds
# project.canis, as the desktop build copies it beside the executable.
if(NOT SETTINGS_DIR)
    get_filename_component(SETTINGS_DIR "${PROJECT_CONFIG}" DIRECTORY)
endif()
file(COPY "${SETTINGS_DIR}/" DESTINATION "${OUTPUT_DIR}/project_settings")

file(GLOB_RECURSE CANIS_ANDROID_FILES RELATIVE "${OUTPUT_DIR}" "${OUTPUT_DIR}/*")
list(SORT CANIS_ANDROID_FILES)

set(CANIS_ANDROID_MANIFEST "")
foreach(asset_file IN LISTS CANIS_ANDROID_FILES)
    file(MD5 "${OUTPUT_DIR}/${asset_file}" asset_hash)
    string(APPEND CANIS_ANDROID_MANIFEST "${asset_hash} ${asset_file}\n")
endforeach()
file(WRITE "${OUTPUT_DIR}/canis_files.txt" "${CANIS_ANDROID_MANIFEST}")
