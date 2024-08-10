set(NAME "unformatter_sample")

file(GLOB SRCS "*.cpp")

add_executable(${NAME} ${SRCS})
target_link_libraries(${NAME} PRIVATE ${UNFORMATTER})
