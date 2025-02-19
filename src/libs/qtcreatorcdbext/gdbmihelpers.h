// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "common.h"
#include <vector>

struct SymbolGroupValueContext;

/* Various helpers to the extension commands to retrieve debuggee information
 * in suitable formats for the debugger engine. */

/* Helpers to retrieve threads and their top stack frame
 * in one roundtrip, conveniently in GDBMI format. */

struct StackFrame
{
    StackFrame(ULONG64 a = 0);
    void formatGDBMI(std::ostream &str, unsigned level = 0) const;
    std::wstring fileName() const;

    ULONG64 address;
    std::wstring function;
    std::wstring fullPathName;
    ULONG line = 0;
};

typedef std::vector<StackFrame> StackFrames;

bool getFrame(unsigned n, StackFrame *f, std::string *errorMessage);
bool getFrame(CIDebugSymbols *debugSymbols, CIDebugControl *debugControl,
              unsigned n, StackFrame *f, std::string *errorMessage);

struct Thread
{
    Thread(ULONG i = 0, ULONG sysId = 0);
    void formatGDBMI(std::ostream &str) const;

    ULONG id;
    ULONG systemId;
    std::string state;
    std::wstring name;
    StackFrame frame;
};

typedef std::vector<Thread> Threads;

// Get list of threads.
bool threadList(CIDebugSystemObjects *debugSystemObjects,
                CIDebugSymbols *debugSymbols,
                CIDebugControl *debugControl,
                CIDebugAdvanced *debugAdvanced,
                Threads* threads, ULONG *currentThreadId,
                std::string *errorMessage);

// Convenience formatting as GDBMI
std::string gdbmiThreadList(CIDebugSystemObjects *debugSystemObjects,
                            CIDebugSymbols *debugSymbols,
                            CIDebugControl *debugControl,
                            CIDebugAdvanced *debugAdvanced,
                            std::string *errorMessage);

/* Helpers for retrieving module lists */

struct Module {
    std::string name;
    std::string image;
    bool deferred = false;
    ULONG64 base = 0;
    ULONG64 size = 0;
};

typedef std::vector<Module> Modules;

// Retrieve modules info
Modules getModules(CIDebugSymbols *syms, std::string *errorMessage);

// Format modules as GDBMI
std::string gdbmiModules(CIDebugSymbols *syms, bool humanReadable, std::string *errorMessage);

//  Helper for breakpoints. Return the memory range of a breakpoint
// as pair(address, size). size is !=0 for data breakpoints.
std::pair<ULONG64, ULONG> breakPointMemoryRange(IDebugBreakpoint *bp);

// Format breakpoints as GDBMI
std::string gdbmiBreakpoints(CIDebugControl *ctrl,
                             CIDebugSymbols *symbols /* = 0 */,
                             CIDebugDataSpaces *dataSpaces /* = 0 */,
                             bool humanReadable,
                             unsigned verbose,
                             std::string *errorMessage);

/* Helpers for registers */
struct Register
{
    Register();

    std::wstring name;
    std::wstring description;
    std::wstring type;
    int size = 0;
    bool subRegister = false;
    bool pseudoRegister = false;
    DEBUG_VALUE value;
};

typedef std::vector<Register> Registers;

// Description of a DEBUG_VALUE type field
const wchar_t *valueType(ULONG type);
// Helper to format a DEBUG_VALUE
void formatDebugValue(std::ostream &str, const DEBUG_VALUE &dv, CIDebugControl *ctl = 0);

enum RegisterFlags {
    IncludePseudoRegisters = 0x1,
    IncludeSubRegisters = 0x2
};

// Retrieve current register snapshot using RegisterFlags
Registers getRegisters(CIDebugRegisters *regs,
                       unsigned flags,
                       std::string *errorMessage);

// Format current register snapshot using RegisterFlags as GDBMI
// This is not exactly using the actual GDB formatting as this is
// to verbose (names and values separated)
std::string gdbmiRegisters(CIDebugRegisters *regs,
                           CIDebugControl *control,
                           bool humanReadable,
                           unsigned flags,
                           std::string *errorMessage);

std::string memoryToHex(CIDebugDataSpaces *ds, ULONG64 address, ULONG length,
                        std::string *errorMessage = 0);
// Stack helpers
StackFrames getStackTrace(CIDebugControl *debugControl, CIDebugSymbols *debugSymbols,
                                 unsigned maxFrames, bool *incomplete, std::string *errorMessage);

std::string gdbmiStack(CIDebugControl *debugControl, CIDebugSymbols *debugSymbols,
                       unsigned maxFrames, bool humanReadable,
                       std::string *errorMessage);

// Find the widget of the application at (x,y) by calling QApplication::widgetAt().
// Return a string of "Qualified_ClassName:Address"
std::string widgetAt(const SymbolGroupValueContext &ctx,
                     int x, int y, std::string *errorMessage);

bool evaluateExpression(CIDebugControl *control, const std::string expression,
                        ULONG desiredType, DEBUG_VALUE *v, std::string *errorMessage);

bool evaluateInt64Expression(CIDebugControl *control, const std::string expression,
                             LONG64 *, std::string *errorMessage);
