add_executable(fin
 main.cpp 
 drawing.cpp
 vector_tile.pb.cc
 tiles.cpp
)

include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

target_link_libraries(fin argparse ncurses z protobuf curses)

add_custom_command(TARGET fin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Add clangd to the src directory")

install(TARGETS fin DESTINATION bin)
