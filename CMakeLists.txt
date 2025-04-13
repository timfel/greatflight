cmake_minimum_required(VERSION 3.22)
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
file(GLOB HAMCONVERT ${PROJECT_SOURCE_DIR}/tools/ham_convert/ham_convert*.jar)
function(hamconvert SOURCE DESTINATION MODE)
  get_filename_component(SOURCEFILE_STEM ${SOURCE} NAME_WLE)
  get_filename_component(SOURCEFILE_DIRECTORY ${SOURCE} DIRECTORY)

  add_custom_command(
    OUTPUT ${DESTINATION}
    COMMAND java -jar ${HAMCONVERT} ${SOURCE} ${MODE} VERBATIM
    COMMAND ${CMAKE_COMMAND} -E rename ${SOURCEFILE_DIRECTORY}/${SOURCEFILE_STEM}_indexed_output.png ${DESTINATION}
    COMMAND ${CMAKE_COMMAND} -E remove ${SOURCEFILE_DIRECTORY}/${SOURCEFILE_STEM}_indexed_output.png
    COMMAND ${CMAKE_COMMAND} -E remove ${SOURCEFILE_DIRECTORY}/${SOURCEFILE_STEM}_output.png
    COMMAND ${CMAKE_COMMAND} -E remove ${SOURCEFILE_DIRECTORY}/${SOURCEFILE_STEM}_output.iff
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${SOURCE}
  )
endfunction()

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

# Menu

hamconvert(${PROJECT_SOURCE_DIR}/resources/GUI/menu_background.png ${PROJECT_BINARY_DIR}/tmp/GUI/menu_background.png ehb dither black_bkd)
png2gpl(${PROJECT_BINARY_DIR}/tmp/GUI/menu_background.png ${PROJECT_BINARY_DIR}/tmp/palettes/menu.gpl)
convertPalette(${GAME_LINKED} ${PROJECT_BINARY_DIR}/tmp/palettes/menu.gpl ${PROJECT_BINARY_DIR}/resources/palettes/menu.plt)
convertBitmaps(
       INTERLEAVED EHB
       TARGET ${GAME_LINKED}
       PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/menu.plt
       SOURCES ${PROJECT_BINARY_DIR}/tmp/GUI/menu_background.png
       DESTINATIONS ${PROJECT_BINARY_DIR}/resources/ui/menu_background.bm
)

png2gpl(${PROJECT_SOURCE_DIR}/resources/GUI/mouse.png ${PROJECT_BINARY_DIR}/tmp/palettes/mouse.gpl)
convertPalette(${GAME_LINKED} ${PROJECT_BINARY_DIR}/tmp/palettes/mouse.gpl ${PROJECT_BINARY_DIR}/tmp/palettes/mouse.plt)
convertBitmaps(
       INTERLEAVED
       TARGET ${GAME_LINKED}
       PALETTE ${PROJECT_BINARY_DIR}/tmp/palettes/mouse.plt
       SOURCES ${PROJECT_SOURCE_DIR}/resources/GUI/mouse.png
       DESTINATIONS ${PROJECT_BINARY_DIR}/resources/ui/mouse.bm
)

# Map

map2bin(${PROJECT_SOURCE_DIR}/resources/maps/example.map ${PROJECT_BINARY_DIR}/resources/maps/example.map)

# Viewport

png2gpl(${PROJECT_SOURCE_DIR}/resources/Tiles/woodland.png ${PROJECT_BINARY_DIR}/tmp/palettes/woodland.gpl)
convertPalette(${GAME_LINKED} ${PROJECT_BINARY_DIR}/tmp/palettes/woodland.gpl ${PROJECT_BINARY_DIR}/resources/palettes/woodland.plt)
convertTileset(
  INTERLEAVED
  TARGET ${GAME_LINKED}
  SIZE 16
  SOURCE ${PROJECT_SOURCE_DIR}/resources/Tiles/woodland.png
  DESTINATION ${PROJECT_BINARY_DIR}/resources/tilesets/woodland.bm
  PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/woodland.plt
  TILE_PATHS ${PROJECT_SOURCE_DIR}/resources/Tiles/woodland.png
)

# convertPalette(${GAME_LINKED} ${PROJECT_SOURCE_DIR}/resources/Palettes/Woodland.gpl ${PROJECT_BINARY_DIR}/resources/palettes/for.plt)

# convertTileset(
#   INTERLEAVED
#   TARGET ${GAME_LINKED}
#   SOURCE ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Woodland.png
#   TILE_PATHS ${PROJECT_SOURCE_DIR}/resources/Tiles/Woodland/Woodland.png
#   DESTINATION ${PROJECT_BINARY_DIR}/resources/tilesets/for.bm
#   SIZE 32
#   PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/for.plt
# )

# convertBitmaps(
#        INTERLEAVED
#        TARGET ${GAME_LINKED}
#        PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/for.plt
#        MASK_COLOR "#000000"
#        SOURCES ${PROJECT_SOURCE_DIR}/resources/Tiles/fow.png
#        DESTINATIONS ${PROJECT_BINARY_DIR}/resources/tilesets/fow.bm
#        MASKS ${PROJECT_BINARY_DIR}/resources/tilesets/fow.msk
# )

# Human UI

hamconvert(${PROJECT_SOURCE_DIR}/resources/GUI/Top_GUI.png ${PROJECT_BINARY_DIR}/tmp/GUI/Top_GUI.png ocs4 dither)
png2gpl(${PROJECT_BINARY_DIR}/tmp/GUI/Top_GUI.png ${PROJECT_BINARY_DIR}/tmp/palettes/Top_GUI.gpl)
convertPalette(${GAME_LINKED} ${PROJECT_BINARY_DIR}/tmp/palettes/Top_GUI.gpl ${PROJECT_BINARY_DIR}/resources/palettes/top.plt)
convertBitmaps(
       INTERLEAVED
       TARGET ${GAME_LINKED}
       PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/top.plt
       SOURCES ${PROJECT_BINARY_DIR}/tmp/GUI/Top_GUI.png
       DESTINATIONS ${PROJECT_BINARY_DIR}/resources/ui/top.bm
)

hamconvert(${PROJECT_SOURCE_DIR}/resources/GUI/Bottom_GUI.png ${PROJECT_BINARY_DIR}/tmp/GUI/Bottom_GUI.png ocs4 dither)
png2gpl(${PROJECT_BINARY_DIR}/tmp/GUI/Bottom_GUI.png ${PROJECT_BINARY_DIR}/tmp/palettes/Bottom_GUI.gpl)
convertPalette(${GAME_LINKED} ${PROJECT_BINARY_DIR}/tmp/palettes/Bottom_GUI.gpl ${PROJECT_BINARY_DIR}/resources/palettes/bottom.plt)
convertBitmaps(
       INTERLEAVED
       TARGET ${GAME_LINKED}
       PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/bottom.plt
       SOURCES ${PROJECT_BINARY_DIR}/tmp/GUI/Bottom_GUI.png
       DESTINATIONS ${PROJECT_BINARY_DIR}/resources/ui/bottom.bm
)

convertPalette(${GAME_LINKED} ${PROJECT_SOURCE_DIR}/resources/Palettes/HumanGUI.gpl ${PROJECT_BINARY_DIR}/resources/palettes/hgui.plt)

# convertBitmaps(
#   INTERLEAVED
#   TARGET ${GAME_LINKED}
#   PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/hgui.plt
#   SOURCES ${PROJECT_SOURCE_DIR}/resources/GUI/Bottom_GUI.png
#           ${PROJECT_SOURCE_DIR}/resources/GUI/Top_GUI.png
#   DESTINATIONS ${PROJECT_BINARY_DIR}/resources/ui/bottompanel.bm
#                ${PROJECT_BINARY_DIR}/resources/ui/toppanel.bm
# )

# convertTileset(
#   INTERLEAVED
#   TARGET ${GAME_LINKED}
#   SOURCE ${PROJECT_SOURCE_DIR}/resources/GUI/icons.png
#   TILE_PATHS ${PROJECT_SOURCE_DIR}/resources/GUI/icons.png
#   DESTINATION ${PROJECT_BINARY_DIR}/resources/ui/icons.bm
#   SIZE 32
#   HEIGHT 18
#   PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/hgui.plt
# )

# convertTileset(
#        INTERLEAVED
#        TARGET ${GAME_LINKED}
#        SOURCE ${PROJECT_SOURCE_DIR}/resources/Units/peasant.png
#        TILE_PATHS ${PROJECT_SOURCE_DIR}/resources/Units/peasant.png
#        DESTINATION ${PROJECT_BINARY_DIR}/tmp/Units/peasant.tileset.png
#        SIZE 16
#        PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/for.plt
# )

# convertBitmaps(
#        INTERLEAVED
#        TARGET ${GAME_LINKED}
#        PALETTE ${PROJECT_BINARY_DIR}/resources/palettes/for.plt
#        MASK_COLOR "#000000"
#        SOURCES ${PROJECT_BINARY_DIR}/tmp/Units/peasant.tileset.png
#        DESTINATIONS ${PROJECT_BINARY_DIR}/resources/units/peasant.bm
#        MASKS ${PROJECT_BINARY_DIR}/resources/units/peasant.msk
# )

convertFont(
       TARGET ${GAME_LINKED}
       FIRST_CHAR 29
       SOURCE ${PROJECT_SOURCE_DIR}/resources/GUI/font_uni54.png
       DESTINATION ${PROJECT_BINARY_DIR}/resources/ui/uni54.fnt
)
