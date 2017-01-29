set(CMAKE_C_COMPILER "/usr/local/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/local/bin/clang++")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 2.8.4)
project(ClangEx)

find_package(Boost 1.58 COMPONENTS system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

set(LLVM_PATH /home/bmuscede/Applications/LLVM-Build)
link_directories(${LLVM_PATH}/lib)
include_directories(${LLVM_PATH}/include)

add_definitions(
        -D__STDC_LIMIT_MACROS
        -D__STDC_CONSTANT_MACROS
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -std=c++11")

set(SOURCE_FILES main.cpp Walker/ASTWalker.cpp Walker/ASTWalker.h Graph/TAGraph.cpp Graph/TAGraph.h Graph/ClangNode.cpp
        Graph/ClangNode.h Graph/ClangEdge.cpp Graph/ClangEdge.h File/FileParse.cpp File/FileParse.h File/ClangArgParse.cpp
        File/ClangArgParse.h Walker/PartialWalker.cpp Walker/PartialWalker.h Walker/BlobWalker.cpp Walker/BlobWalker.h TupleAttribute/TAProcessor.cpp TupleAttribute/TAProcessor.h)
add_executable(ClangEx ${SOURCE_FILES})

target_link_libraries(ClangEx
        clangFrontend
        clangSerialization
        clangDriver
        clangParse
        clangSema
        clangAnalysis
        clangAST
        clangASTMatchers
        clangBasic
        clangEdit
        clangLex
        clangTooling
        )

target_link_libraries(ClangEx
        LLVMLTO
        LLVMPasses
        LLVMObjCARCOpts
        LLVMSymbolize
        LLVMDebugInfoPDB
        LLVMDebugInfoDWARF
        LLVMMIRParser
        LLVMCoverage
        LLVMTableGen
        LLVMOrcJIT
        LLVMXCoreDisassembler
        LLVMXCoreCodeGen
        LLVMXCoreDesc
        LLVMXCoreInfo
        LLVMXCoreAsmPrinter
        LLVMSystemZDisassembler
        LLVMSystemZCodeGen
        LLVMSystemZAsmParser
        LLVMSystemZDesc
        LLVMSystemZInfo
        LLVMSystemZAsmPrinter
        LLVMSparcDisassembler
        LLVMSparcCodeGen
        LLVMSparcAsmParser
        LLVMSparcDesc
        LLVMSparcInfo
        LLVMSparcAsmPrinter
        LLVMPowerPCDisassembler
        LLVMPowerPCCodeGen
        LLVMPowerPCAsmParser
        LLVMPowerPCDesc
        LLVMPowerPCInfo
        LLVMPowerPCAsmPrinter
        LLVMNVPTXCodeGen
        LLVMNVPTXDesc
        LLVMNVPTXInfo
        LLVMNVPTXAsmPrinter
        LLVMMSP430CodeGen
        LLVMMSP430Desc
        LLVMMSP430Info
        LLVMMSP430AsmPrinter
        LLVMMipsDisassembler
        LLVMMipsCodeGen
        LLVMMipsAsmParser
        LLVMMipsDesc
        LLVMMipsInfo
        LLVMMipsAsmPrinter
        LLVMLanaiDisassembler
        LLVMLanaiCodeGen
        LLVMLanaiAsmParser
        LLVMLanaiDesc
        LLVMLanaiInstPrinter
        LLVMLanaiInfo
        LLVMHexagonDisassembler
        LLVMHexagonCodeGen
        LLVMHexagonAsmParser
        LLVMHexagonDesc
        LLVMHexagonInfo
        LLVMBPFCodeGen
        LLVMBPFDesc
        LLVMBPFInfo
        LLVMBPFAsmPrinter
        LLVMARMDisassembler
        LLVMARMCodeGen
        LLVMARMAsmParser
        LLVMARMDesc
        LLVMARMInfo
        LLVMARMAsmPrinter
        LLVMAMDGPUDisassembler
        LLVMAMDGPUCodeGen
        LLVMAMDGPUAsmParser
        LLVMAMDGPUDesc
        LLVMAMDGPUInfo
        LLVMAMDGPUAsmPrinter
        LLVMAMDGPUUtils
        LLVMAArch64Disassembler
        LLVMAArch64CodeGen
        LLVMGlobalISel
        LLVMAArch64AsmParser
        LLVMAArch64Desc
        LLVMAArch64Info
        LLVMAArch64AsmPrinter
        LLVMAArch64Utils
        LLVMObjectYAML
        LLVMLibDriver
        LLVMOption
        LLVMX86Disassembler
        LLVMX86AsmParser
        LLVMX86CodeGen
        LLVMSelectionDAG
        LLVMAsmPrinter
        LLVMDebugInfoCodeView
        LLVMDebugInfoMSF
        LLVMX86Desc
        LLVMMCDisassembler
        LLVMX86Info
        LLVMX86AsmPrinter
        LLVMX86Utils
        LLVMMCJIT
        LLVMLineEditor
        LLVMInterpreter
        LLVMExecutionEngine
        LLVMRuntimeDyld
        LLVMCodeGen
        LLVMTarget
        LLVMBitWriter
        LLVMCoroutines
        LLVMipo
        LLVMInstrumentation
        LLVMVectorize
        LLVMScalarOpts
        LLVMObject
        LLVMMCParser
        LLVMMC
        LLVMLinker
        LLVMIRReader
        LLVMBitReader
        LLVMAsmParser
        LLVMInstCombine
        LLVMTransformUtils
        LLVMAnalysis
        LLVMProfileData
        LLVMCore
        LLVMSupport
        LLVMDemangle
        )

include(FindCurses)
target_link_libraries(ClangEx
        pthread
        z
        dl
        crypto
        ${CURSES_LIBRARIES}
        )

target_link_libraries(ClangEx
        ${Boost_LIBRARIES}
        )

add_custom_command(TARGET ClangEx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/include $<TARGET_FILE_DIR:ClangEx>/include)