# ================================================
#      Platform-independent LLVM + LLDB + Clang Setup
# ================================================

# Find LLVM using either LLVM_DIR, CMAKE_PREFIX_PATH or fallback to find_package
find_package(LLVM REQUIRED CONFIG)

if (NOT LLVM_FOUND)
  message(FATAL_ERROR "LLVM not found. Set LLVM_DIR or add to CMAKE_PREFIX_PATH.")
endif()

set(LLVM_INCLUDE_DIR "${LLVM_INCLUDE_DIRS}")
set(LLVM_LIB_DIR "${LLVM_LIBRARY_DIRS}")
set(LLDB_INCLUDE_DIR "${LLDB_INCLUDE_DIRS}")

list(APPEND CMAKE_PREFIX_PATH "${LLVM_DIR}")

message(STATUS "LLVM found at: ${LLVM_DIR}")
message(STATUS "LLVM Include: ${LLVM_INCLUDE_DIR}")
message(STATUS "LLVM Libs: ${LLVM_LIB_DIR}")

function(msg_lib_added lib_type imp_type NAME)
    message(STATUS "Added ${imp_type} ${lib_type} Library ${NAME}")
endfunction()

# Function: Adds an imported LLDB library target LLDB::lldb if not already present.
function(add_lldb_target_if_missing)
    if (NOT TARGET LLDB::lldb)
        # Candidate LLDB library files per platform
        set(LLDB_STATIC_CANDIDATES)
        set(LLDB_SHARED_CANDIDATES)

        if (WIN32)
            list(APPEND LLDB_STATIC_CANDIDATES "${LLVM_LIB_DIR}/liblldb.lib" "${LLVM_LIB_DIR}/lldb.lib")
            list(APPEND LLDB_SHARED_CANDIDATES "${LLVM_LIB_DIR}/liblldb.dll")
        elseif (APPLE)
            list(APPEND LLDB_STATIC_CANDIDATES "${LLVM_LIB_DIR}/liblldb.a")
            list(APPEND LLDB_SHARED_CANDIDATES "${LLVM_LIB_DIR}/liblldb.dylib")
        elseif (UNIX)
            list(APPEND LLDB_STATIC_CANDIDATES "${LLVM_LIB_DIR}/liblldb.a")
            list(APPEND LLDB_SHARED_CANDIDATES
                "${LLVM_LIB_DIR}/liblldb.so"
                "${LLVM_LIB_DIR}/liblldb.so.20.1.7"
                "${LLVM_LIB_DIR}/liblldb.so.20.1"
                "${LLVM_LIB_DIR}/liblldb.so.1"
            )
        endif()

        set(LLDB_LIB_FILE "")
        set(LLDB_LIB_TYPE "")

        # Prefer static library if available
        foreach(CANDIDATE IN LISTS LLDB_STATIC_CANDIDATES)
            if (EXISTS "${CANDIDATE}")
                set(LLDB_LIB_FILE "${CANDIDATE}")
                set(LLDB_LIB_TYPE STATIC)
                break()
            endif()
        endforeach()

        # If static not found, try shared
        if (NOT LLDB_LIB_FILE)
            foreach(CANDIDATE IN LISTS LLDB_SHARED_CANDIDATES)
                if (EXISTS "${CANDIDATE}")
                    set(LLDB_LIB_FILE "${CANDIDATE}")
                    set(LLDB_LIB_TYPE SHARED)
                    break()
                endif()
            endforeach()
        endif()

        if (NOT LLDB_LIB_FILE)
            message(FATAL_ERROR "No LLDB static or shared library found in ${LLVM_LIB_DIR}")
        endif()

        # Declare imported target
        add_library(LLDB::lldb ${LLDB_LIB_TYPE} IMPORTED GLOBAL)
        msg_lib_added(${LLDB_LIB_TYPE} IMPORTED "LLDB::lldb")

        if (MSVC OR CMAKE_GENERATOR MATCHES "Visual Studio")
            set_target_properties(LLDB::lldb PROPERTIES
                IMPORTED_CONFIGURATIONS "Debug;Release"
                IMPORTED_LOCATION_DEBUG "${LLDB_LIB_FILE}"
                IMPORTED_LOCATION_RELEASE "${LLDB_LIB_FILE}"
            )
        else()
            set_target_properties(LLDB::lldb PROPERTIES
                IMPORTED_LOCATION "${LLDB_LIB_FILE}"
            )
        endif()

        set_target_properties(LLDB::lldb PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LLDB_INCLUDE_DIR}"
        )

        # Platform-specific system dependencies for LLDB
        if (APPLE)
            target_link_libraries(LLDB::lldb INTERFACE "-framework Foundation" "-framework CoreFoundation")
        elseif (UNIX AND NOT ANDROID)
            target_link_libraries(LLDB::lldb INTERFACE pthread dl)
        elseif (WIN32)
            target_link_libraries(LLDB::lldb INTERFACE shlwapi psapi)
        endif()
    endif()
endfunction()

add_lldb_target_if_missing()

find_package(Clang REQUIRED CONFIG)

message(STATUS "Found Clang ${Clang_VERSION}")

# LLVM includes and definitions
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Map LLVM components
llvm_map_components_to_libnames(llvm_libs
    analysis
    bitreader
    bitwriter
    codegen
    core
    executionengine
    instcombine
    instrumentation
    interpreter
    ipo
    irreader
    linker
    lto
    mc
    mcjit
    objcarcopts
    option
    profiledata
    scalaropts
    support
    target
    transformutils
    vectorize
    passes
)

set(CLANG_LIBS
    clangBasic
    clangAPINotes
    clangLex
    clangParse
    clangAST
    clangDynamicASTMatchers
    clangASTMatchers
    clangCrossTU
    clangSema
    clangCodeGen
    clangAnalysis
    clangAnalysisFlowSensitive
    clangAnalysisFlowSensitiveModels
    clangEdit
    clangExtractAPI
    clangRewrite
    clangDriver
    clangSerialization
    clangRewriteFrontend
    clangFrontend
    clangFrontendTool
    clangToolingCore
    clangToolingInclusions
    clangToolingInclusionsStdlib
    clangToolingRefactoring
    clangToolingASTDiff
    clangToolingSyntax
    clangDependencyScanning
    clangTransformer
    clangTooling
    clangDirectoryWatcher
    clangIndex
    clangIndexSerialization
    clangInstallAPI
    clangStaticAnalyzerCore
    clangStaticAnalyzerCheckers
    clangStaticAnalyzerFrontend
    clangFormat
    clangInterpreter
    clangSupport
    clangHandleCXX
    clangHandleLLVM
    clangApplyReplacements
    clangReorderFields
    clangTidy
    clangTidyAndroidModule
    clangTidyAbseilModule
    clangTidyAlteraModule
    clangTidyBoostModule
    clangTidyBugproneModule
    clangTidyCERTModule
    clangTidyConcurrencyModule
    clangTidyCppCoreGuidelinesModule
    clangTidyDarwinModule
    clangTidyFuchsiaModule
    clangTidyGoogleModule
    clangTidyHICPPModule
    clangTidyLinuxKernelModule
    clangTidyLLVMModule
    clangTidyLLVMLibcModule
    clangTidyMiscModule
    clangTidyModernizeModule
    clangTidyMPIModule
    clangTidyObjCModule
    clangTidyOpenMPModule
    clangTidyPerformanceModule
    clangTidyPortabilityModule
    clangTidyReadabilityModule
    clangTidyZirconModule
    clangTidyPlugin
    clangTidyMain
    clangTidyUtils
    clangChangeNamespace
    clangDoc
    clangIncludeFixer
    clangIncludeFixerPlugin
    clangMove
    clangQuery
    clangIncludeCleaner
    clangdSupport
    clangDaemon
    clangDaemonTweaks
    clangdMain
    clangdRemoteIndex
)

# Merge Clang and LLDB libs
set(LLVM_LIBS
    ${llvm_libs}
    ${CLANG_LIBS}
    LLDB::lldb
)

add_library(LLVM_Interface INTERFACE)
target_link_libraries(LLVM_Interface INTERFACE ${LLVM_LIBS})