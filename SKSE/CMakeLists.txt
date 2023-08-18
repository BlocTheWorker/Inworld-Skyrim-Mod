cmake_minimum_required(VERSION 3.21)

project(InworldSkyrim VERSION 0.0.1 LANGUAGES CXX)

if(DEFINED ENV{SKYRIM_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_FOLDER}/Data")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_FOLDER}/Data")
endif()

if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME}")
endif()
 
find_package(CommonLibSSE CONFIG REQUIRED)
add_commonlibsse_plugin(${PROJECT_NAME} SOURCES plugin.cpp) # <--- specifies plugin.cpp
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23) # <--- use C++23 standard 
target_precompile_headers(${PROJECT_NAME} PRIVATE PCH.h) # <--- PCH.h is required!

find_package(cpr CONFIG REQUIRED) 
target_link_libraries(${PROJECT_NAME} PUBLIC cpr::cpr)
 
find_package(nlohmann_json REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
 
find_package(websocketpp REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC websocketpp::websocketpp)

find_package(Boost REQUIRED COMPONENTS random)
target_link_libraries(${PROJECT_NAME} PUBLIC Boost::boost Boost::random)


if(DEFINED OUTPUT_FOLDER)
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")
    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()
endif()
