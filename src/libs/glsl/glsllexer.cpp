// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "glsllexer.h"
#include "glslparser.h"
#include "glslengine.h"
#include <qbytearray.h>
#include <cctype>
#include <iostream>
#include <cstdio>

using namespace GLSL;

Lexer::Lexer(Engine *engine, const char *source, unsigned size)
    : _engine(engine),
      _source(source),
      _it(source),
      _size(size),
      _yychar('\n'),
      _lineno(0),
      _state(0),
      _variant(Variant_Mask & ~Variant_Reserved), // everything except reserved
      _scanKeywords(true),
      _scanComments(false)
{
}

Lexer::~Lexer()
{
}

void Lexer::yyinp()
{
    _yychar = (unsigned char) *_it++;
    if (_yychar == '\n')
        ++_lineno;
}

int Lexer::yylex(Token *tk)
{
    const char *pos = nullptr;
    int line = 0;
    _yyval.ptr = nullptr;
    const int kind = yylex_helper(&pos, &line);
    tk->kind = kind;
    tk->position = pos - _source;
    tk->length = _it - pos - 1;
    tk->line = line;
    tk->ptr = _yyval.ptr;
    return kind;
}

enum {
    State_normal,
    State_comment
};

int Lexer::yylex_helper(const char **position, int *line)
{
    again:
        while (std::isspace(_yychar))
      yyinp();

    *position = _it - 1;
    *line = _lineno;

    if (_yychar == 0)
        return Parser::EOF_SYMBOL;

    if (_state == State_comment) {
        while (_yychar) {
            if (_yychar == '*') {
                yyinp();
                if (_yychar == '/') {
                    yyinp();
                    _state = State_normal;
                    break;
                }
            } else {
                yyinp();
            }
        }
        return Parser::T_COMMENT;
    }

    const int ch = _yychar;
    yyinp();

    switch (ch) {
    case '#':
        for (; _yychar; yyinp()) {
            if (_yychar == '\n')
                break;
        }
        goto again;

        // one of `!', `!='
    case '!':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_NE_OP;
        }
        return Parser::T_BANG;

        // one of
        //  %
        //  %=
    case '%':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_MOD_ASSIGN;
        }
        return Parser::T_PERCENT;

        // one of
        // &
        // &&
        // &=
    case '&':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_AND_ASSIGN;
        } else if (_yychar == '&') {
            yyinp();
            return Parser::T_AND_OP;
        }
        return Parser::T_AMPERSAND;

        // (
    case '(':
        return Parser::T_LEFT_PAREN;

        // )
    case ')':
        return Parser::T_RIGHT_PAREN;

        // one of
        // *
        // *=
    case '*':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_MUL_ASSIGN;
        }
        return Parser::T_STAR;

        // one of
        // ++
        // +=
        // +
    case '+':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_ADD_ASSIGN;
        } else if (_yychar == '+') {
            yyinp();
            return Parser::T_INC_OP;
        }
        return Parser::T_PLUS;

        // ,
    case ',':
        return Parser::T_COMMA;

        // one of
        // -
        // --
        // -=
    case '-':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_SUB_ASSIGN;
        } else if (_yychar == '-') {
            yyinp();
            return Parser::T_DEC_OP;
        }
        return Parser::T_DASH;

        // one of
        // .
        // float constant
    case '.':
        if (std::isdigit(_yychar)) {
            const char *word = _it - 2;
            while (std::isalnum(_yychar)) {
                yyinp();
            }
            if (_engine)
                _yyval.string = _engine->number(word, _it - word - 1);
            return Parser::T_NUMBER;
        }
        return Parser::T_DOT;

        // one of
        // /
        // /=
        // comment
    case '/':
        if (_yychar == '/') {
            for (; _yychar; yyinp()) {
                if (_yychar == '\n')
                    break;
            }
            if (_scanComments)
                return Parser::T_COMMENT;
            goto again;
        } else if (_yychar == '*') {
            yyinp();
            while (_yychar) {
                if (_yychar == '*') {
                    yyinp();
                    if (_yychar == '/') {
                        yyinp();
                        if (_scanComments)
                            return Parser::T_COMMENT;
                        goto again;
                    }
                } else {
                    yyinp();
                }
            }
            if (_scanComments) {
                _state = State_comment;
                return Parser::T_COMMENT;
            }
            goto again;
        } else if (_yychar == '=') {
            yyinp();
            return Parser::T_DIV_ASSIGN;
        }
        return Parser::T_SLASH;

        // :
    case ':':
        return Parser::T_COLON;

        // ;
    case ';':
        return Parser::T_SEMICOLON;

        // one of
        // <
        // <=
        // <<
        // <<=
    case '<':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_LE_OP;
        } else if (_yychar == '<') {
            yyinp();
            if (_yychar == '=') {
                yyinp();
                return Parser::T_LEFT_ASSIGN;
            }
            return Parser::T_LEFT_OP;
        }
        return Parser::T_LEFT_ANGLE;

        // one of
        // =
        // ==
    case '=':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_EQ_OP;
        }
        return Parser::T_EQUAL;

        // one of
        // >
        // >=
        // >>=
        // >>
    case '>':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_GE_OP;
        } else if (_yychar == '>') {
            yyinp();
            if (_yychar == '=') {
                yyinp();
                return Parser::T_RIGHT_ASSIGN;
            }
            return Parser::T_RIGHT_OP;
        }
        return Parser::T_RIGHT_ANGLE;

        // ?
    case '?':
        return Parser::T_QUESTION;

        // [
    case '[':
        return Parser::T_LEFT_BRACKET;

        // ]
    case ']':
        return Parser::T_RIGHT_BRACKET;

        // one of
        // ^
        // ^=
    case '^':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_XOR_ASSIGN;
        } else if (_yychar == '^') {
            yyinp();
            return Parser::T_XOR_OP;
        }
        return Parser::T_CARET;

        // {
    case '{':
        return Parser::T_LEFT_BRACE;

        // one of
        // |
        // |=
        // ||
    case '|':
        if (_yychar == '=') {
            yyinp();
            return Parser::T_OR_ASSIGN;
        } else if (_yychar == '|') {
            yyinp();
            return Parser::T_OR_OP;
        }
        return Parser::T_VERTICAL_BAR;

        // }
    case '}':
        return Parser::T_RIGHT_BRACE;

        // ~
    case '~':
        return Parser::T_TILDE;

    default:
        if (std::isalpha(ch) || ch == '_') {
            const char *word = _it - 2;
            while (std::isalnum(_yychar) || _yychar == '_') {
                yyinp();
            }
            if (_scanKeywords) {
                const int k = findKeyword(word, _it - word - 1);

                if (k != Parser::T_IDENTIFIER)
                    return k;
            }
            if (_engine)
                _yyval.string = _engine->identifier(word, _it - word - 1);
            return Parser::T_IDENTIFIER;
        } else if (std::isdigit(ch)) {
            const char *word = _it - 2;
            while (std::isalnum(_yychar) || _yychar == '.') {
                yyinp();
            }
            if (_engine)
                _yyval.string = _engine->number(word, _it - word - 1);
            return Parser::T_NUMBER;
        }

    } // switch

    return Parser::T_ERROR;
}

int Lexer::findKeyword(const char *word, int length) const
{
    int t = classify(word, length);
    if (!(t & Variant_Mask))
        return t;
    if ((_variant & t & Variant_Mask) == 0) {
        // Return a "reserved word" token if this keyword is not permitted
        // in the current language variant so that the syntax highlighter
        // can warn the user about the word.
        if (!_scanKeywords)
            return Parser::T_RESERVED;
    }
    return t & ~Variant_Mask;
}

void Lexer::warning(int line, const QString &message)
{
    _engine->warning(line, message);
}

void Lexer::error(int line, const QString &message)
{
    _engine->error(line, message);
}
