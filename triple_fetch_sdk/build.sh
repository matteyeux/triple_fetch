#!/bin/bash
`xcrun --sdk iphoneos -f clang` -isysroot `xcrun --sdk iphoneos --show-sdk-path` -arch arm64 -o hello_world remote_call.c remote_ports.c remote_memory.c task_ports.c hello_world.c
ldid -S hello_world
