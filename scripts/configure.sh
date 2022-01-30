#!/bin/bash

source common.sh

cmake -B $BUILD_DIR -S $REPO_ROOT \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

ln -f -s $BUILD_DIR/compile_commands.json $REPO_ROOT

