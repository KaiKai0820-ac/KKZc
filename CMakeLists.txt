cmake_minimum_required(VERSION 3.24)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(FkkZc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# nlohmann/json (header-only)
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

# libcurl (Schannel TLS on Windows)
set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_TESTS ON CACHE BOOL "" FORCE)
FetchContent_Declare(curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG        curl-8_7_1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(curl)

# wxWidgets (static)
set(wxBUILD_SHARED OFF CACHE BOOL "" FORCE)
set(wxUSE_GUI ON CACHE BOOL "" FORCE)
set(wxBUILD_CXX_STANDARD 20 CACHE STRING "" FORCE)
FetchContent_Declare(wxWidgets
    GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
    GIT_TAG        v3.2.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(wxWidgets)

# Application
add_executable(FkkZc WIN32
    src/main.cpp
    src/model/LinkStore.cpp
    src/api/QuarkApi.cpp
    src/worker/TransferWorker.cpp
    src/gui/MainFrame.cpp
    src/gui/VirtualLinkList.cpp
    src/gui/QuarkDirDialog.cpp
)

target_include_directories(FkkZc PRIVATE src)
target_link_libraries(FkkZc PRIVATE
    nlohmann_json::nlohmann_json
    CURL::libcurl
    wx::core wx::base
)

if(WIN32)
    target_link_libraries(FkkZc PRIVATE ws2_32 crypt32)
    target_compile_options(FkkZc PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()
