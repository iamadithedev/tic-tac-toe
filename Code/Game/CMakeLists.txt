project(TicTacToe)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE Common Assets Platform Graphics Resources Components Editor Physics Math)
target_sources(${PROJECT_NAME}        PRIVATE main.cpp board.cpp item.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build")