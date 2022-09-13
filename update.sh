#!/bin/bash
# Copy precompiled static libraries for pochivm here
rm -rf pochi_libs_and_includes/lib/*
cp PochiVM/build/release/pochivm/libpochivm.a pochi_libs_and_includes/lib/
cp PochiVM/build/release/runtime/libruntime.a pochi_libs_and_includes/lib/
cp PochiVM/build/release/runtime/libruntime_bc.a pochi_libs_and_includes/lib/
cp PochiVM/build/release/runtime/libruntime_bc_validator.a pochi_libs_and_includes/lib/
# Copy updated source
rm -rf pochi_libs_and_includes/include/*
cp -r PochiVM/fastinterp pochi_libs_and_includes/include/fastinterp
cp -r PochiVM/__generated__/release/generated pochi_libs_and_includes/include/generated
cp -r PochiVM/gtest pochi_libs_and_includes/include/gtest
cp -r PochiVM/pochivm pochi_libs_and_includes/include/pochivm
cp -r PochiVM/runtime pochi_libs_and_includes/include/runtime
cp -r PochiVM/test_util_helper.h pochi_libs_and_includes/include/test_util_helper.h
