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
else
    echo -n "Would you like to configure your project? (y/n): "
    read configure
    if [ $configure == 'y' ]; then
        cmake ..
    fi

    compiledb make all

    echo -n "Would you like to run the project? (y/n): "
    read execute
    if [ $execute == 'y' ]; then
        ./Sandbox/SandboxApp
    fi

    mv compile_commands.json .. && cd ..
fi
