#Generated by VisualGDB (http://visualgdb.com)
#DO NOT EDIT THIS FILE MANUALLY UNLESS YOU ABSOLUTELY NEED TO
#USE VISUALGDB PROJECT PROPERTIES DIALOG INSTEAD

BINARYDIR := Release

#Toolchain
CC := C:/Users/Philip/Desktop/MollenOS/Bin/Compiler/bin/clang.exe
CXX := C:/Users/Philip/Desktop/MollenOS/Bin/Compiler/bin/clang++.exe
LD := C:/Users/Philip/Desktop/MollenOS/Bin/Compiler/bin/lld.exe
AR := C:/Users/Philip/Desktop/MollenOS/Bin/Compiler/bin/llvm-ar.exe
AS := C:/Users/Philip/AppData/Local/nasm/nasm.exe
OBJCOPY := C:/Qt/Tools/mingw491_32/bin/objcopy.exe

#Additional flags
PREPROCESSOR_MACROS := NDEBUG=1 RELEASE=1
INCLUDE_DIRS := ../libc/include ../libcxx/include include
LIBRARY_DIRS := 
LIBRARY_NAMES := 
ADDITIONAL_LINKER_INPUTS := 
MACOS_FRAMEWORKS := 
LINUX_PACKAGES := 

CFLAGS := -O3 -target i686-none-elf -ffreestanding -fno-builtin -nostdlib -Wno-macro-redefined 
CXXFLAGS := -O3 -target i686-none-elf -nostdinc++ -ffreestanding -fno-builtin -nostdlib -Wno-macro-redefined 
ASFLAGS := -f elf32
LDFLAGS := -flavor gnu
COMMONFLAGS := 
LINKER_SCRIPT := 

START_GROUP := 
END_GROUP := 

#Additional options detected from testing the toolchain
USE_DEL_TO_CLEAN := 1
CP_NOT_AVAILABLE := 1
