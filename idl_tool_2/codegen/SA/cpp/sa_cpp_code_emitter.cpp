/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "sa_cpp_code_emitter.h"

#include <regex>
#include <unordered_set>

#include "util/options.h"

namespace OHOS {
namespace Idl {
void SACppCodeEmitter::GetImportInclusions(HeaderFile::HeaderFileSet &headerFiles) const
{
    for (const auto &importName : ast_->GetImportNames()) {
        size_t index = importName.rfind(SEPARATOR);
        if (index == std::string::npos) {
            index = -1;
        }
        std::string fileName = importName.substr(index + 1);
        std::string relativePath = importName.substr(0, index + 1) + FileName(fileName);
        std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
        headerFiles.emplace(HeaderFileType::OWN_MODULE_HEADER_FILE, relativePath);
    }
}

void SACppCodeEmitter::EmitImportUsingNamespace(StringBuilder &sb) const
{
    std::unordered_map<std::string, int> usedNamespace;
    for (const auto &importPair : ast_->GetImports()) {
        AutoPtr<AST> importAst = importPair.second;
        if (importAst->GetASTFileType() == ASTFileType::AST_SEQUENCEABLE) {
            continue;
        }
        size_t index = importPair.first.rfind('.');
        std::string usingName = importPair.first.substr(0, index);
        if (usingName.empty()) {
            continue;
        }
        std::regex dotToColons("\\.");
        usingName = std::regex_replace(usingName, dotToColons, "::");
        if (usedNamespace[usingName] == 0) {
            usedNamespace[usingName] = 1;
            sb.Append("using namespace ").AppendFormat("%s;\n", usingName.c_str());
        }
    }
    sb.Append('\n');
}

void SACppCodeEmitter::GetStdlibInclusions(HeaderFile::HeaderFileSet &headerFiles) const
{
    const AST::TypeStringMap &types = ast_->GetTypes();

    // Add header files dependency by default to avoid compatibility problems
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "string_ex");
    headerFiles.emplace(HeaderFileType::CPP_STD_HEADER_FILE, "cstdint");
    for (const auto &pair : types) {
        AutoPtr<ASTType> type = pair.second;
        switch (type->GetTypeKind()) {
            case TypeKind::TYPE_ARRAY:
            case TypeKind::TYPE_LIST: {
                headerFiles.emplace(HeaderFileType::CPP_STD_HEADER_FILE, "vector");
                break;
            }
            case TypeKind::TYPE_MAP: {
                headerFiles.emplace(HeaderFileType::CPP_STD_HEADER_FILE, "unordered_map");
                break;
            }
            default:
                break;
        }
    }
}

void SACppCodeEmitter::EmitBeginNamespace(StringBuilder &sb) const
{
    std::string fullNamespace = GetNamespace(interface_->GetNamespace()->ToString());
    std::vector<std::string> namespaceVec = StringHelper::Split(fullNamespace, ".");

    for (const auto &nspace : namespaceVec) {
        sb.AppendFormat("namespace %s {\n", nspace.c_str());
    }
}

void SACppCodeEmitter::EmitEndNamespace(StringBuilder &sb) const
{
    std::string fullNamespace = GetNamespace(interface_->GetNamespace()->ToString());
    std::vector<std::string> namespaceVec = StringHelper::Split(fullNamespace, ".");

    for (auto nspaceIter = namespaceVec.rbegin(); nspaceIter != namespaceVec.rend(); ++nspaceIter) {
        sb.AppendFormat("} // namespace %s\n", nspaceIter->c_str());
    }
}

void SACppCodeEmitter::EmitWriteMethodParameter(const AutoPtr<ASTParameter> &param, const std::string &parcelName,
    StringBuilder &sb, const std::string &prefix) const
{
    AutoPtr<SaTypeEmitter> typeEmitter = GetTypeEmitter(param->GetType());
    typeEmitter->EmitCppWriteVar(parcelName, param->GetName(), sb, prefix);
}

void SACppCodeEmitter::EmitReadMethodParameter(const AutoPtr<ASTParameter> &param, const std::string &parcelName,
    bool emitType, StringBuilder &sb, const std::string &prefix) const
{
    AutoPtr<SaTypeEmitter> typeEmitter = GetTypeEmitter(param->GetType());
    typeEmitter->EmitCppReadVar(parcelName, param->GetName(), sb, prefix, emitType);
}

void SACppCodeEmitter::EmitInterfaceMethodParams(const AutoPtr<ASTMethod> &method, StringBuilder &sb,
    const std::string &prefix) const
{
    AutoPtr<ASTType> returnType = method->GetReturnType();
    TypeKind retTypeKind = returnType->GetTypeKind();
    size_t paramNumber = method->GetParameterNumber();

    for (size_t i = 0; i < paramNumber; i++) {
        AutoPtr<ASTParameter> param = method->GetParameter(i);
        sb.Append('\n').Append(prefix).AppendFormat(EmitCppParameter(param).c_str());
        if ((i != paramNumber - 1) || (retTypeKind != TypeKind::TYPE_VOID)) {
            sb.Append(',');
        }
    }
    if (retTypeKind != TypeKind::TYPE_VOID) {
        AutoPtr<SaTypeEmitter> typeEmitter = GetTypeEmitter(returnType);
        sb.Append('\n').Append(prefix).AppendFormat("%s funcResult",
            typeEmitter->EmitCppType(TypeMode::PARAM_OUT).c_str());
    }
}

std::string SACppCodeEmitter::EmitCppParameter(const AutoPtr<ASTParameter> &param) const
{
    std::string name = param->GetName();
    AutoPtr<SaTypeEmitter> typeEmitter = GetTypeEmitter(param->GetType());
    ASTParamAttr::ParamAttr attrAttr = param->GetAttribute();
    if (attrAttr == ASTParamAttr::PARAM_INOUT) {
        return StringHelper::Format("%s %s", typeEmitter->EmitCppType(TypeMode::PARAM_INOUT).c_str(), name.c_str());
    } else if (attrAttr == ASTParamAttr::PARAM_IN) {
        return StringHelper::Format("%s %s", typeEmitter->EmitCppType(TypeMode::PARAM_IN).c_str(), name.c_str());
    } else if (attrAttr == ASTParamAttr::PARAM_OUT) {
        return StringHelper::Format("%s %s", typeEmitter->EmitCppType(TypeMode::PARAM_OUT).c_str(), name.c_str());
    }
    return StringHelper::Format("unknown param attr %s", name.c_str());
}

void SACppCodeEmitter::EmitSecurecInclusion(StringBuilder &sb) const
{
    size_t methodNumber = interface_->GetMethodNumber();
    for (size_t i = 0; i < methodNumber; i++) {
        AutoPtr<ASTMethod> method = interface_->GetMethod(i);
        size_t paramNumber = method->GetParameterNumber();
        for (size_t j = 0; j < paramNumber; j++) {
            AutoPtr<ASTParameter> param = method->GetParameter(j);
            TypeKind paramTypeKind = param->GetType()->GetTypeKind();
            if (paramTypeKind == TypeKind::TYPE_UNION || paramTypeKind == TypeKind::TYPE_STRUCT) {
                sb.Append("#include <securec.h>\n");
                return;
            }
        }

        AutoPtr<ASTType> returnType = method->GetReturnType();
        TypeKind retTypeKind = returnType->GetTypeKind();
        if (retTypeKind == TypeKind::TYPE_UNION || retTypeKind == TypeKind::TYPE_STRUCT) {
            sb.Append("#include <securec.h>\n");
            return;
        }
    }
}

void SACppCodeEmitter::EmitInterfaceMethodCommands(StringBuilder &sb, const std::string &prefix)
{
    size_t methodNumber = interface_->GetMethodNumber();
    sb.AppendFormat("enum class %sIpcCode {\n", interface_->GetName().c_str());
    for (size_t i = 0; i < methodNumber; i++) {
        AutoPtr<ASTMethod> method = interface_->GetMethod(i);
        std::string commandCode = "COMMAND_" + ConstantName(method->GetName());
        bool hasIpcCode = method->HasIpcCode();
        if (i == 0 && !hasIpcCode) {
            commandCode += " = MIN_TRANSACTION_ID";
        } else if (hasIpcCode) {
            commandCode += " = " + method->GetIpcCodeStr();
        }
        commandCode += ",\n";
        sb.Append(prefix).AppendFormat(commandCode.c_str());
    }
    sb.Append("};\n\n");
}

} // namespace Idl
} // namespace OHOS