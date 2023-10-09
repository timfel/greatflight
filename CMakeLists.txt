cmake_minimum_required(VERSION 3.0)
project(AceProject C)

# Lowercase project name for binaries and packaging
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

if(NOT AMIGA)
	message(SEND_ERROR "This project only compiles for Amiga")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DAMIGA -DACE_NO_BOB_WRAP_Y -Wall -Wextra -fomit-frame-pointer")
# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DACE_DEBUG -DGENERIC_MAIN_LOG_PATH=\\\"${PROJECT_NAME}.log\\\"")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DACE_DEBUG -DACE_DEBUG_UAE")

add_subdirectory(deps/ACE ace)
include_directories(deps/ace/include)

file(GLOB_RECURSE SOURCES src/*.c)
file(GLOB_RECURSE HEADERS src/*.h)

include_directories(${PROJECT_SOURCE_DIR}/src)

# Linux/other UNIX get a lower-case binary name
set(TARGET_NAME ${PROJECT_NAME_LOWER})

if(ELF2HUNK)
  # Add elf2hunk step for Bartman compiler
  set(GAME_LINKED ${TARGET_NAME}.elf) # Intermediate executable
  set(GAME_EXE ${TARGET_NAME}.exe) # Use this to launch the game
  add_executable(${GAME_LINKED} ${SOURCES} ${HEADERS})
  add_custom_command(
    TARGET ${GAME_LINKED} POST_BUILD
    COMMAND ${ELF2HUNK} ${GAME_LINKED} ${GAME_EXE}
  )
	FILE(
    WRITE ${PROJECT_BINARY_DIR}/disassemble.cmake
    "EXECUTE_PROCESS(COMMAND ${OBJDUMP} --disassemble -S ${GAME_LINKED} OUTPUT_FILE ${TARGET_NAME}.s)"
  )
  add_custom_command(
    TARGET ${GAME_LINKED} POST_BUILD
    COMMAND ${CMAKE_COMMAND} ARGS -P ${PROJECT_BINARY_DIR}/disassemble.cmake
  )
else()
  # Just produce the executable with Bebbo compiler
  SET(GAME_LINKED ${TARGET_NAME})
  SET(GAME_EXE ${TARGET_NAME})
  add_executable(${GAME_LINKED} ${SOURCES} ${HEADERS})
endif()

target_link_libraries(${GAME_LINKED} ace)

# resources
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/resources/maps)

## Helper functions
function(png2gpl SOURCE DESTINATION)
	add_custom_command(
		OUTPUT ${DESTINATION}
		COMMAND python3 ${PROJECT_SOURCE_DIR}/tools/png2gpl.py ${SOURCE} ${DESTINATION}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${SOURCE}
	)
endfunction()

function(tileize DESTINATION TILESIZEX TILESIZEY PALETTE)
	add_custom_command(
		OUTPUT ${DESTINATION}
		COMMAND python3 ${PROJECT_SOURCE_DIR}/tools/tileize.py ${TILESIZEX} ${TILESIZEY} ${PALETTE} ${DESTINATION} ${ARGN}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${ARGN}
	)
endfunction()

function(palettize DESTINATION PALETTE SOURCE)
	add_custom_command(
		OUTPUT ${DESTINATION}
		COMMAND python3 ${PROJECT_SOURCE_DIR}/tools/tileize.py ${PALETTE} ${DESTINATION} ${SOURCE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${ARGN}
	)
endfunction()

function(map2bin SOURCE DESTINATION)
	add_custom_command(
		OUTPUT ${DESTINATION}
		COMMAND python3 ${PROJECT_SOURCE_DIR}/tools/map2bin.py ${SOURCE} ${DESTINATION}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${SOURCE}
	)
  target_sources(${GAME_LINKED} PUBLIC ${DESTINATION})
endfunction()

function(font2pngs SOURCE DESTINATION)
	add_custom_command(
		OUTPUT ${DESTINATION}
		COMMAND python3 ${PROJECT_SOURCE_DIR}/tools/map2bin.py ${SOURCE} ${DESTINATION}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${SOURCE}
	)
  target_sources(${GAME_LINKED} PUBLIC ${DESTINATION})
endfunction()

# Map

map2bin(${PROJECT_SOURCE_DIR}/resources/maps/example.map ${PROJECT_BINARY_DIR}/resources/maps/example.map)

# Viewport

png2gpl(${PROJECT_SOURCE_DIR}/resources/Palettes/Viewport.png ${PROJECT_BINARY_DIR}/tmp/Palettes/Viewport.gpl)
convertPalette(${GAME_LINKED} ${PROJECT_BINARY_DIR}/tmp/Palettes/Viewport.gpl ${PROJECT_BINARY_DIR}/resources/palettes/for.plt)

tileize(
  ${PROJECT_BINARY_DIR}/tmp/Tiles/Woodland/Woodland.png
  32 32 ${PROJECT_SOURCE_DIR}/resources/Palettes/Viewport.png
  # ground tiles
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Grass.png              # 00
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Bridge0.png            # 04
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Bridge1.png            # 08
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Bridge2.png            # 12
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Bridge3.png            # 16
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Tree3.png              # 20
  # harvestable tiles
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Tree2.png              # 24
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Tree1.png              # 28
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Tree0.png              # 32
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Water.png              # 36
  # partially walkable tiles
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Coast0.png             # 40, 41
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Coast1.png             # 44, 45, 47
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Coast2.png             # 48, 49, 50
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Coast3.png             # 52, 54
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Coast4.png             # 57, 59
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Coast5.png             # 60, 62, 63
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Coast6.png             # 66, 67
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Coast7.png             # 69, 70, 71
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Coast8.png             # 72
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Coast9.png             # 77
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Coast10.png            # 82
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Coast11.png            # 87
  # buildings
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Barracs.png            # 88
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Blacksmith.png         # 104
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Church.png             # 108
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Construction_big.png   # 114
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Construction_small.png # 130
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Farm.png               # 134
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Hall.png               # 138
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/lumbermill.png         # 154
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Stable.png             # 170
  ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Tower.png              # 186
  # gold mine                                                           # 190
  # 206 + is free for decorations or such things
)

convertTileset(
  INTERLEAVED
  TARGET ${GAME_LINKED}
  SOURCE ${PROJECT_BINARY_DIR}/tmp/Tiles/Woodland/Woodland.png
  TILE_PATHS ${PROJECT_BINARY_DIR}/tmp/Tiles/Woodland/Woodland.png
  DESTINATION ${PROJECT_BINARY_DIR}/resources/tilesets/for.bm
  SIZE 32
  PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/for.plt
)

# Human UI
png2gpl(${PROJECT_SOURCE_DIR}/resources/Palettes/HumanGUI.png ${PROJECT_BINARY_DIR}/tmp/Palettes/HumanGUI.gpl)
convertPalette(${GAME_LINKED} ${PROJECT_BINARY_DIR}/tmp/Palettes/HumanGUI.gpl ${PROJECT_BINARY_DIR}/resources/palettes/hgui.plt)

palettize(
  ${PROJECT_BINARY_DIR}/tmp/GUI/Bottom_GUI.png
  ${PROJECT_SOURCE_DIR}/resources/Palettes/HumanGUI.png
  ${PROJECT_SOURCE_DIR}/resources/GUI/Bottom_GUI.png
)

palettize(
  ${PROJECT_BINARY_DIR}/tmp/GUI/Top_GUI.png
  ${PROJECT_SOURCE_DIR}/resources/Palettes/HumanGUI.png
  ${PROJECT_SOURCE_DIR}/resources/GUI/Top_GUI.png
)

convertBitmaps(
  INTERLEAVED
  TARGET ${GAME_LINKED}
  PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/hgui.plt
  SOURCES ${PROJECT_BINARY_DIR}/tmp/GUI/Bottom_GUI.png
          ${PROJECT_BINARY_DIR}/tmp/GUI/Top_GUI.png
  DESTINATIONS ${PROJECT_BINARY_DIR}/resources/ui/bottompanel.bm
               ${PROJECT_BINARY_DIR}/resources/ui/toppanel.bm
)

palettize(
  ${PROJECT_BINARY_DIR}/tmp/Units/peon.png
  ${PROJECT_SOURCE_DIR}/resources/Palettes/Viewport.png
  ${PROJECT_SOURCE_DIR}/resources/Units/peon.png
)

convertTileset(
       INTERLEAVED
       TARGET ${GAME_LINKED}
       SOURCE ${PROJECT_BINARY_DIR}/tmp/Units/peon.png
       TILE_PATHS ${PROJECT_BINARY_DIR}/tmp/Units/peon.png
       DESTINATION ${PROJECT_BINARY_DIR}/tmp/Units/peon.tileset.png
       SIZE 16
       PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/for.plt
)

convertBitmaps(
       INTERLEAVED
       TARGET ${GAME_LINKED}
       PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/for.plt
       MASK_COLOR "#ff00dd"
       SOURCES ${PROJECT_BINARY_DIR}/tmp/Units/peon.tileset.png
       DESTINATIONS ${PROJECT_BINARY_DIR}/resources/units/peon.bm
       MASKS ${PROJECT_BINARY_DIR}/resources/units/peon.msk
)

convertFont(
       TARGET ${GAME_LINKED}
       FIRST_CHAR 29
       SOURCE ${PROJECT_SOURCE_DIR}/resources/GUI/font_uni54.png
       DESTINATION ${PROJECT_BINARY_DIR}/resources/ui/uni54.fnt
)
