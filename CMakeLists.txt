cmake_minimum_required(VERSION 3.8)
project(dnd-inspect)

set(CMAKE_CXX_STANDARD 11)

link_directories("/develop/lib")
link_libraries(be)

set(SOURCE_FILES
        "main.cpp"
        "App.h"
        "App.cpp"
        "MainWindow.h"
        "MainWindow.cpp"
        "DragSourceView.h"
        "DragSourceView.cpp"
        "DropTargetView.h"
        "DropTargetView.cpp"
        "DragSourceList.h"
        "DragSourceList.cpp"
        )

add_executable(dnd-inspect ${SOURCE_FILES})


