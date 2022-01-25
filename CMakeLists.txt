cmake_minimum_required(VERSION 3.10)

set(CORE_SRC )
set(PIPE_SRC )
file(GLOB CORE_SRC core/*.cc core/*.h)
file(GLOB PIPE_SRC pipe/*.cc pipe/*.h)

include_directories(pipe)
include_directories(core)

add_library(async-io ${CORE_SRC} ${PIPE_SRC})
target_include_directories(async-io PUBLIC ./)
target_link_libraries(async-io logger)

#add_executable(svrtest test-server.cc)
#target_link_libraries(svrtest async-io)




