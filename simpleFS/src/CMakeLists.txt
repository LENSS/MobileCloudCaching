cmake_minimum_required(VERSION 2.6)
project(simpleFS)

add_executable(simpleFS fUtils.c main.c)

INSTALL(TARGETS simpleFS RUNTIME DESTINATION /usr/bin/)
