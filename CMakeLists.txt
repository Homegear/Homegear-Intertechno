cmake_minimum_required(VERSION 3.8)
project(homegear_intertechno)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        src/PhysicalInterfaces/Coc.cpp
        src/PhysicalInterfaces/Coc.h
        src/PhysicalInterfaces/Cul.cpp
        src/PhysicalInterfaces/Cul.h
        src/PhysicalInterfaces/Cunx.cpp
        src/PhysicalInterfaces/Cunx.h
        src/PhysicalInterfaces/IIntertechnoInterface.cpp
        src/PhysicalInterfaces/IIntertechnoInterface.h
        src/PhysicalInterfaces/TiCc1100.cpp
        src/PhysicalInterfaces/TiCc1100.h
        src/Factory.cpp
        src/Factory.h
        src/GD.cpp
        src/GD.h
        src/Interfaces.cpp
        src/Interfaces.h
        src/MyCentral.cpp
        src/MyCentral.h
        src/MyFamily.cpp
        src/MyFamily.h
        src/MyPacket.cpp
        src/MyPacket.h
        src/MyPeer.cpp
        src/MyPeer.h
        config.h)

add_custom_target(homegear COMMAND ../../makeAll.sh SOURCES ${SOURCE_FILES})

add_library(homegear_intertechno ${SOURCE_FILES})