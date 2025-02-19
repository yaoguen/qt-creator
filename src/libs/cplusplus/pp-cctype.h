// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

/*
  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cplusplus/CPlusPlusForwardDeclarations.h>

#include <cctype>

namespace CPlusPlus {

inline bool CPLUSPLUS_EXPORT pp_isalpha (int ch)
{ return std::isalpha ((unsigned char) ch) != 0; }

inline bool CPLUSPLUS_EXPORT pp_isalnum (int ch)
{ return std::isalnum ((unsigned char) ch) != 0; }

inline bool CPLUSPLUS_EXPORT pp_isdigit (int ch)
{ return std::isdigit ((unsigned char) ch) != 0; }

inline bool CPLUSPLUS_EXPORT pp_isspace (int ch)
{ return std::isspace ((unsigned char) ch) != 0; }

} // namespace CPlusPlus
