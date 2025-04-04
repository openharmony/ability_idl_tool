/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "parser/lexer.h"

#include <utility>
#include <cstdlib>
#include <limits>
#include "util/string_builder.h"

namespace OHOS {
namespace Idl {
Lexer::Lexer()
{
    InitializeKeywords();
}

Lexer::~Lexer()
{
    if (currentFile_ != nullptr) {
        currentFile_->Close();
    }
}

void Lexer::InitializeKeywords()
{
    keywords_[String("boolean")] = Token::BOOLEAN;
    keywords_[String("byte")] = Token::BYTE;
    keywords_[String("char")] = Token::CHAR;
    keywords_[String("double")] = Token::DOUBLE;
    keywords_[String("float")] = Token::FLOAT;
    keywords_[String("in")] = Token::IN;
    keywords_[String("inout")] = Token::INOUT;
    keywords_[String("int")] = Token::INTEGER;
    keywords_[String("interface")] = Token::INTERFACE;
    keywords_[String("List")] = Token::LIST;
    keywords_[String("long")] = Token::LONG;
    keywords_[String("Map")] = Token::MAP;
    keywords_[String("oneway")] = Token::ONEWAY;
    keywords_[String("out")] = Token::OUT;
    keywords_[String("sequenceable")] = Token::SEQUENCEABLE;
    keywords_[String("short")] = Token::SHORT;
    keywords_[String("String")] = Token::STRING;
    keywords_[String("cacheable")] = Token::CACHEABLE;
}

bool Lexer::OpenSourceFile(const String& filePath)
{
    currentFile_ = std::make_shared<File>(filePath, File::READ);
    if (!currentFile_->IsValid()) {
        return false;
    }

    return true;
}

Token Lexer::GetToken(bool skipComment)
{
    if (!havePeek_) {
        currentToken_ = ReadToken(skipComment);
    }
    havePeek_ = false;
    return currentToken_;
}

Token Lexer::PeekToken(bool skipComment)
{
    if (!havePeek_) {
        currentToken_ = ReadToken(skipComment);
        havePeek_ = true;
    }
    return currentToken_;
}

Token Lexer::ReadToken(bool skipComment)
{
    while (!currentFile_->IsEof()) {
        char c = currentFile_->GetChar();
        tokenLineNo_ = currentFile_->GetCharLineNumber();
        tokenColumnNo_ = currentFile_->GetCharColumnNumber();
        if (IsSpace(c)) {
            continue;
        } else if (IsAlphabet(c) || c == '_') {
            return ReadIdentifier(c);
        }
        switch (c) {
            case '<':
            case '>':
            case '{':
            case '}':
            case '[':
            case ']':
            case ',':
            case '(':
            case ')':
            case '.':
            case ';':
                currentToken_ = token_map_[c];
                return currentToken_;
            case '/':
                if (ReadTokenPeek(skipComment, c)) {
                    return currentToken_;
                } else {
                    continue;
                }
            default:
                currentToken_ = Token::UNKNOWN;
                return currentToken_;
        }
    }
    currentToken_ = Token::END_OF_FILE;
    return currentToken_;
}

bool Lexer::ReadTokenPeek(bool skipComment, char letter)
{
    if (currentFile_->PeekChar() == '/') {
        ReadLineComment(letter);
        if (!skipComment) {
            return true;
        }
        return false;
    } else if (currentFile_->PeekChar() == '*') {
        ReadBlockComment(letter);
        if (!skipComment) {
            return true;
        }
        return false;
    }
    currentToken_ = Token::UNKNOWN;
    return true;
}

Token Lexer::ReadIdentifier(char c)
{
    StringBuilder sb;

    sb.Append(c);
    while (!currentFile_->IsEof()) {
        c = currentFile_->PeekChar();
        if (IsAlphabet(c) || c == '_' || IsDecimalDigital(c) || c == '.') {
            c = currentFile_->GetChar();
            sb.Append(c);
            continue;
        }
        if (IsSpace(c)) {
            currentFile_->GetChar();
        }
        break;
    }
    String key = sb.ToString();
    auto it = keywords_.find(key);
    if (it == keywords_.end()) {
        identifier_ = key;
        currentToken_ = Token::IDENTIFIER;
    } else {
        currentToken_ = it->second;
    }
    return currentToken_;
}

Token Lexer::ReadLineComment(char c)
{
    StringBuilder sb;

    sb.Append(c);
    while (!currentFile_->IsEof()) {
        c = currentFile_->GetChar();
        if (c == '\n') {
            break;
        }
        sb.Append(c);
    }
    comment_ = sb.ToString();
    currentToken_ = Token::COMMENT_LINE;
    return currentToken_;
}

Token Lexer::ReadBlockComment(char c)
{
    StringBuilder sb;

    sb.Append(c);
    while (!currentFile_->IsEof()) {
        c = currentFile_->GetChar();
        sb.Append(c);
        if (c == '*' && currentFile_->PeekChar() == '/') {
            c = currentFile_->GetChar();
            sb.Append(c);
            break;
        }
    }
    comment_ = sb.ToString();
    currentToken_ = Token::COMMENT_BLOCK;
    return currentToken_;
}

void Lexer::SkipCurrentLine()
{
    while (!currentFile_->IsEof()) {
        char c = currentFile_->GetChar();
        if (c == '\n') {
            currentFile_->GetChar();
            return;
        }
    }
}

bool Lexer::SkipCurrentLine(char untilChar)
{
    while (!currentFile_->IsEof()) {
        int c = currentFile_->GetChar();
        if (c == untilChar) {
            return true;
        }
        if (c == '\n') {
            currentFile_->GetChar();
            return false;
        }
    }
    return true;
}

int Lexer::TokenToChar(Token token)
{
    switch (token) {
        case Token::ANGLE_BRACKETS_LEFT:
            return '<';
        case Token::ANGLE_BRACKETS_RIGHT:
            return '>';
        case Token::BRACES_LEFT:
            return '{';
        case Token::BRACES_RIGHT:
            return '}';
        case Token::BRACKETS_LEFT:
            return '[';
        case Token::BRACKETS_RIGHT:
            return ']';
        case Token::COMMA:
            return ',';
        case Token::DOT:
            return '.';
        case Token::PARENTHESES_LEFT:
            return '(';
        case Token::PARENTHESES_RIGHT:
            return ')';
        case Token::SEMICOLON:
            return ';';
        default:
            return -1;
    }
}

String Lexer::DumpToken() const
{
    switch (currentToken_) {
        case Token::ANGLE_BRACKETS_LEFT:
            return "<";
        case Token::ANGLE_BRACKETS_RIGHT:
            return ">";
        case Token::BOOLEAN:
            return "boolean";
        case Token::BRACES_LEFT:
            return "{";
        case Token::BRACES_RIGHT:
            return "}";
        case Token::BRACKETS_LEFT:
            return "[";
        case Token::BRACKETS_RIGHT:
            return "]";
        case Token::BYTE:
            return "byte";
        case Token::CHAR:
            return "char";
        case Token::COMMA:
            return ",";
        case Token::COMMENT_BLOCK:
        case Token::COMMENT_LINE:
            return comment_;
        case Token::DOT:
            return ".";
        default:
            return DumpTokenSecond();
    }
}

String Lexer::DumpTokenSecond() const
{
    switch (currentToken_) {
        case Token::DOUBLE:
            return "double";
        case Token::END_OF_FILE:
            return "eof";
        case Token::FLOAT:
            return "float";
        case Token::IDENTIFIER:
            return identifier_;
        case Token::IN:
            return "in";
        case Token::INOUT:
            return "inout";
        case Token::INTEGER:
            return "int";
        case Token::LIST:
            return "List";
        case Token::LONG:
            return "long";
        case Token::MAP:
            return "Map";
        case Token::ONEWAY:
            return "oneway";
        case Token::OUT:
            return "out";
        case Token::SEQUENCEABLE:
            return "sequenceable";
        case Token::PARENTHESES_LEFT:
            return "(";
        case Token::PARENTHESES_RIGHT:
            return ")";
        case Token::SEMICOLON:
            return ";";
        case Token::SHORT:
            return "short";
        case Token::STRING:
            return "String";
        default:
            return "unknown token";
    }
}

bool Lexer::strToInt(const char *str, int strLen, int& number)
{
    int result = 0;
    int positionWeight = 1;
    const int ten = 10;

    for (int i = strLen - 1; i >= 0; i--) {
        if (str[i] < '0' || str[i] > '9') {
            return false;
        }
        int digit = str[i] - '0';
        if (static_cast<int64_t>(digit * positionWeight) > std::numeric_limits<int32_t>::max() - result) {
            return false;
        }
        result += digit * positionWeight;
        positionWeight *= ten;
    }
    number = result;
    return true;
}

bool Lexer::ParseCacheable(int& cacheTime)
{
    bool ret = true;
    StringBuilder numbersb;

    while (!currentFile_->IsEof()) {
        char c = currentFile_->PeekChar();
        if (IsSpace(c)) {
            currentFile_->GetChar();
            continue;
        }
        if (!IsDecimalDigital(c)) {
            if (c != ']' && c != ',') {
                ret = false;
            }
            break;
        }
        numbersb.Append(c);
        currentFile_->GetChar();
    }

    if (ret == false) {
        return ret;
    }
    String numberStr = numbersb.ToString();
    if (numberStr.IsNull()) {
        return false;
    }

    ret = strToInt(numberStr.string(), numberStr.GetLength(), cacheTime);
    return ret;
}

} // namespace idl
} // namespace OHOS
