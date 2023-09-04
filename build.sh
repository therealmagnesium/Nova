#!/bin/bash
cd build

if [ "$1" == "clean" ]; then
    make clean

    echo -n "Would you like to remove config files too? (y/n): "
    read removeConfig
    if [ $removeConfig == 'y' ]; then
        rm Makefile
        rm Nova/Makefile
        rm Sandbox/Makefile
    fi

    cd ..
elif [ "$1" == "run" ]; then
    cd Sandbox
    ./SandboxApp
    cd ../..
else
    if [ "$1" == "debug" ]; then
        cmake -DGLFW_BUILD_DOCS=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug ..
    elif [ "$1" == "release" ]; then
        cmake -DGLFW_BUILD_DOCS=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release ..
    fi

    mv compile_commands.json ..
    cp -r ../Sandbox/assets ./Sandbox

    make all

    echo -n "Would you like to run the project? (y/n): "
    read execute
    if [ $execute == 'y' ]; then
        cd Sandbox
        ./SandboxApp
    fi

    cd ../..
fi
