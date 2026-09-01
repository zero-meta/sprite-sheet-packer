cmake_minimum_required(VERSION 3.21)

project(SpriteSheetPackerCLI VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Xml)

qt_add_executable(sprite-sheet-packer
    SpriteSheetPacker/main.cpp
    SpriteSheetPacker/command-line.cpp
    SpriteSheetPacker/DataExporter.cpp
    SpriteSheetPacker/PublishSpriteSheet.cpp
    SpriteSheetPacker/SpriteAtlas.cpp
    SpriteSheetPacker/SpritePackerProjectFile.cpp
    SpriteSheetPacker/PolygonImage.cpp
    SpriteSheetPacker/algorithm/polypack2d.cpp
    SpriteSheetPacker/TPSParser/TPSParser.cpp
    SpriteSheetPacker/3rdparty/qtplist-master/PListSerializer.cpp
    SpriteSheetPacker/3rdparty/clipper/clipper.cpp
    SpriteSheetPacker/3rdparty/poly2tri/common/shapes.cc
    SpriteSheetPacker/3rdparty/poly2tri/sweep/advancing_front.cc
    SpriteSheetPacker/3rdparty/poly2tri/sweep/cdt.cc
    SpriteSheetPacker/3rdparty/poly2tri/sweep/sweep_context.cc
    SpriteSheetPacker/3rdparty/poly2tri/sweep/sweep.cc
)

target_include_directories(sprite-sheet-packer PRIVATE
    SpriteSheetPacker
    SpriteSheetPacker/algorithm
    SpriteSheetPacker/TPSParser
    SpriteSheetPacker/3rdparty/qtplist-master
    SpriteSheetPacker/3rdparty/clipper
    SpriteSheetPacker/3rdparty/poly2tri
)

target_link_libraries(sprite-sheet-packer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Xml
)

if(APPLE)
    set_target_properties(sprite-sheet-packer PROPERTIES
        BUILD_WITH_INSTALL_RPATH ON
        INSTALL_RPATH "@executable_path/../lib"
    )
endif()

install(TARGETS sprite-sheet-packer RUNTIME DESTINATION bin)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME cli-version COMMAND sprite-sheet-packer --version)
    set_tests_properties(cli-version PROPERTIES PASS_REGULAR_EXPRESSION "sprite-sheet-packer ${PROJECT_VERSION}")

    add_test(
        NAME cli-smoke
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:sprite-sheet-packer>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/SpriteSheetPacker/res/icon-addFolder.png
            -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/cli-smoke
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_smoke.cmake
    )
endif()
