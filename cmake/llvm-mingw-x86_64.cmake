# Cross-compile toolchain: Linux host -> 64-bit Windows using llvm-mingw
# (clang + recent mingw-w64 headers, needed for JUCE 8's Direct2D 1.3 usage).
# Pass -DLLVM_MINGW_ROOT=/path/to/llvm-mingw or set the LLVM_MINGW_ROOT env var.
if(NOT DEFINED LLVM_MINGW_ROOT)
    set(LLVM_MINGW_ROOT "$ENV{LLVM_MINGW_ROOT}")
endif()
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER   "${LLVM_MINGW_ROOT}/bin/x86_64-w64-mingw32-clang")
set(CMAKE_CXX_COMPILER "${LLVM_MINGW_ROOT}/bin/x86_64-w64-mingw32-clang++")
set(CMAKE_RC_COMPILER  "${LLVM_MINGW_ROOT}/bin/x86_64-w64-mingw32-windres")
set(CMAKE_FIND_ROOT_PATH "${LLVM_MINGW_ROOT}/x86_64-w64-mingw32")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# Recent libc++ trimmed transitive includes that JUCE 8.0.4 relies on
set(CMAKE_CXX_FLAGS_INIT "-include type_traits -include cstring -isystem ${CMAKE_CURRENT_LIST_DIR}/mingw-shims")
set(CMAKE_C_FLAGS_INIT "-isystem ${CMAKE_CURRENT_LIST_DIR}/mingw-shims")
# Self-contained binaries: no MinGW runtime DLLs needed on the target machine
set(CMAKE_EXE_LINKER_FLAGS_INIT    "-static")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-static")
