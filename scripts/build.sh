#!/bin/bash

source common.sh

cmake --build $BUILD_DIR -j `nproc` --config Release

