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

#ifndef OHOS_IDL_LEXER_H
#define OHOS_IDL_LEXER_H

#include <ctype.h>
#include <memory>
#include <unordered_map>

#include "parser/token.h"
#include "util/file.h"
#include "util/string.h"

namespace OHOS {
namespace Idl {
class Lexer {
public:
    Lexer();

    ~Lexer();

    bool OpenSourceFile(const String& filePath);

    Token GetToken(bool skipComment = true);

    Token PeekToken(bool skipComment = true);

    bool ReadTokenPeek(bool skipComment, char letter);

    String GetIdentifier() const
    {
        return identifier_;
    }

    String GetComment() const
    {
        return comment_;
    }

    void SkipCurrentLine();

    bool SkipCurrentLine(char untilChar);

    std::shared_ptr<File> GetSourceFile() const
    {
        return currentFile_;
    }

    String DumpToken() const;

    String DumpTokenSecond() const;

    int GetTokenLineNumber() const
    {
        return tokenLineNo_;
    }

    int GetTokenColumnNumber() const
    {
        return tokenColumnNo_;
    }

    static int TokenToChar(Token token);

    bool ParseCacheable(int& cacheTime);
private:
    void InitializeKeywords();

    Token ReadToken(bool skipComment);

    Token ReadIdentifier(char c);

    Token ReadLineComment(char c);

    Token ReadBlockComment(char c);

    static bool IsAlphabet(char c)
    {
        return isalpha(c);
    }

    static bool IsDecimalDigital(char c)
    {
        return isdigit(c);
    }

    static bool IsSpace(char c)
    {
        return isspace(c);
    }

    static bool strToInt(const char *str, int strLen, int& number);

    static const char* TAG;
    std::unordered_map<String, Token, StringHashFunc, StringEqualFunc> keywords_;
    Token currentToken_ = Token::UNKNOWN;
    int tokenLineNo_ = 0;
    int tokenColumnNo_ = 0;
    String identifier_;
    String comment_;
    bool havePeek_ = false;
    std::shared_ptr<File> currentFile_;
    std::unordered_map<char, Token> token_map_ = {
        {'<', Token::ANGLE_BRACKETS_LEFT},
        {'>', Token::ANGLE_BRACKETS_RIGHT},
        {'{', Token::BRACES_LEFT},
        {'}', Token::BRACES_RIGHT},
        {'[', Token::BRACKETS_LEFT},
        {']', Token::BRACKETS_RIGHT},
        {',', Token::COMMA},
        {'(', Token::PARENTHESES_LEFT},
        {')', Token::PARENTHESES_RIGHT},
        {'.', Token::DOT},
        {';', Token::SEMICOLON},
    };
};
} // namespace Idl
} // namespace OHOS
#endif // OHOS_IDL_LEXER_H
