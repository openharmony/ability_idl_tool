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

#include "cpp_custom_types_code_emitter.h"
#include "util/file.h"
#include "util/logger.h"

namespace OHOS {
namespace Idl {
bool CppCustomTypesCodeEmitter::ResolveDirectory(const std::string &targetDirectory)
{
    if (ast_->GetASTFileType() != ASTFileType::AST_TYPES) {
        return false;
    }

    directory_ = GetFileParentPath(targetDirectory);
    if (!File::CreateParentDir(directory_)) {
        Logger::E("CppCustomTypesCodeEmitter", "Create '%s' failed!", directory_.c_str());
        return false;
    }

    return true;
}

void CppCustomTypesCodeEmitter::EmitCode()
{
    switch (mode_) {
        case GenMode::PASSTHROUGH: {
            EmitPassthroughCustomTypesHeaderFile();
            break;
        }
        case GenMode::IPC: {
            EmitCustomTypesHeaderFile();
            EmitCustomTypesSourceFile();
            break;
        }
        default:
            break;
    }
}

void CppCustomTypesCodeEmitter::EmitPassthroughCustomTypesHeaderFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.h", directory_.c_str(), FileName(baseName_).c_str()));
    File file(filePath, File::WRITE);
    std::string marcoName = StringHelper::Format("%s.%s", ast_->GetPackageName().c_str(), baseName_.c_str());
    StringBuilder sb;

    EmitLicense(sb);
    EmitHeadMacro(sb, marcoName);
    sb.Append("\n");
    EmitPassthroughHeaderFileInclusions(sb);
    sb.Append("\n");
    EmitBeginNamespace(sb);
    sb.Append("\n");
    EmitUsingNamespace(sb);
    sb.Append("\n");
    EmitCustomTypeDecls(sb);
    EmitEndNamespace(sb);
    sb.Append("\n");
    EmitTailMacro(sb, marcoName);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CppCustomTypesCodeEmitter::EmitPassthroughHeaderFileInclusions(StringBuilder &sb)
{
    HeaderFile::HeaderFileSet headerFiles;
    headerFiles.emplace(HeaderFileType::CPP_STD_HEADER_FILE, "cstdbool");
    headerFiles.emplace(HeaderFileType::CPP_STD_HEADER_FILE, "cstdint");
    GetStdlibInclusions(headerFiles);
    GetImportInclusions(headerFiles);

    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CppCustomTypesCodeEmitter::EmitCustomTypesHeaderFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.h", directory_.c_str(), FileName(baseName_).c_str()));
    File file(filePath, File::WRITE);
    std::string marcoName = StringHelper::Format("%s.%s", ast_->GetPackageName().c_str(), baseName_.c_str());
    StringBuilder sb;

    EmitLicense(sb);
    EmitHeadMacro(sb, marcoName);
    sb.Append("\n");
    EmitHeaderFileInclusions(sb);
    sb.Append("\n");
    EmitInterfaceBuffSizeMacro(sb);
    sb.Append("\n");
    EmitForwardDeclaration(sb);
    sb.Append("\n");
    EmitBeginNamespace(sb);
    sb.Append("\n");
    EmitUsingNamespace(sb);
    sb.Append("\n");
    EmitCustomTypeDecls(sb);
    sb.Append("\n");
    EmitCustomTypeFuncDecl(sb);
    sb.Append("\n");
    EmitEndNamespace(sb);
    sb.Append("\n");
    EmitTailMacro(sb, marcoName);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CppCustomTypesCodeEmitter::EmitHeaderFileInclusions(StringBuilder &sb)
{
    HeaderFile::HeaderFileSet headerFiles;
    headerFiles.emplace(HeaderFileType::CPP_STD_HEADER_FILE, "cstdbool");
    headerFiles.emplace(HeaderFileType::CPP_STD_HEADER_FILE, "cstdint");
    GetStdlibInclusions(headerFiles);
    GetImportInclusions(headerFiles);

    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CppCustomTypesCodeEmitter::EmitForwardDeclaration(StringBuilder &sb) const
{
    sb.Append("namespace OHOS {\n");
    sb.Append("class MessageParcel;\n");
    sb.Append("}\n");
}

void CppCustomTypesCodeEmitter::EmitUsingNamespace(StringBuilder &sb)
{
    sb.Append("using namespace OHOS;\n");
    EmitImportUsingNamespace(sb);
}

void CppCustomTypesCodeEmitter::EmitCustomTypeDecls(StringBuilder &sb) const
{
    for (size_t i = 0; i < ast_->GetTypeDefinitionNumber(); i++) {
        AutoPtr<HdiTypeEmitter> typeEmitter = GetTypeEmitter(ast_->GetTypeDefintion(i));
        if (typeEmitter == nullptr) {
            continue;
        }
        sb.Append(typeEmitter->EmitCppTypeDecl()).Append("\n");
        if (i + 1 < ast_->GetTypeDefinitionNumber()) {
            sb.Append("\n");
        }
    }
}

void CppCustomTypesCodeEmitter::EmitCustomTypeFuncDecl(StringBuilder &sb) const
{
    for (size_t i = 0; i < ast_->GetTypeDefinitionNumber(); i++) {
        AutoPtr<ASTType> type = ast_->GetTypeDefintion(i);
        if (type->GetTypeKind() == TypeKind::TYPE_STRUCT) {
            EmitCustomTypeMarshallFuncDecl(sb, type);
            if (i + 1 < ast_->GetTypeDefinitionNumber()) {
                sb.Append("\n");
            }
        }
    }
}

void CppCustomTypesCodeEmitter::EmitCustomTypeMarshallFuncDecl(StringBuilder &sb, const AutoPtr<ASTType> &type) const
{
    AutoPtr<HdiTypeEmitter> typeEmitter = GetTypeEmitter(type);
    if (typeEmitter == nullptr) {
        return;
    }
    std::string objName("dataBlock");
    sb.AppendFormat("bool %sBlockMarshalling(OHOS::MessageParcel &data, const %s& %s);\n\n", type->GetName().c_str(),
        typeEmitter->EmitCppType().c_str(), objName.c_str());
    sb.AppendFormat("bool %sBlockUnmarshalling(OHOS::MessageParcel &data, %s& %s);\n", type->GetName().c_str(),
        typeEmitter->EmitCppType().c_str(), objName.c_str());
}

void CppCustomTypesCodeEmitter::EmitCustomTypesSourceFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.cpp", directory_.c_str(), FileName(baseName_).c_str()));
    File file(filePath, File::WRITE);
    StringBuilder sb;

    EmitLicense(sb);
    EmitSourceFileInclusions(sb);
    sb.Append("\n");
    EmitBeginNamespace(sb);
    sb.Append("\n");
    EmitUtilMethods(sb, true);
    sb.Append("\n");
    EmitUtilMethods(sb, false);
    sb.Append("\n");
    EmitCustomTypeDataProcess(sb);
    sb.Append("\n");
    EmitEndNamespace(sb);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CppCustomTypesCodeEmitter::EmitSourceFileInclusions(StringBuilder &sb)
{
    HeaderFile::HeaderFileSet headerFiles;

    headerFiles.emplace(HeaderFileType::OWN_HEADER_FILE, EmitVersionHeaderName(baseName_));
    GetSourceOtherLibInclusions(headerFiles);

    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CppCustomTypesCodeEmitter::GetSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const
{
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_core_log");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "securec");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "message_parcel");
}

void CppCustomTypesCodeEmitter::EmitCustomTypeDataProcess(StringBuilder &sb) const
{
    for (size_t i = 0; i < ast_->GetTypeDefinitionNumber(); i++) {
        AutoPtr<ASTType> type = ast_->GetTypeDefintion(i);
        if (type->GetTypeKind() == TypeKind::TYPE_STRUCT) {
            AutoPtr<ASTStructType> structType = static_cast<ASTStructType *>(type.Get());
            EmitCustomTypeMarshallingImpl(sb, structType);
            EmitCustomTypeUnmarshallingImpl(sb, structType);
            if (i + 1 < ast_->GetTypeDefinitionNumber()) {
                sb.Append("\n");
            }
        }
    }
}

void CppCustomTypesCodeEmitter::EmitCustomTypeMarshallingImpl(
    StringBuilder &sb, const AutoPtr<ASTStructType> &type) const
{
    AutoPtr<HdiTypeEmitter> typeEmitter = GetTypeEmitter(type.Get());
    if (typeEmitter == nullptr) {
        return;
    }
    std::string typeName = typeEmitter->EmitCppType();
    std::string objName("dataBlock");

    sb.AppendFormat("bool %sBlockMarshalling(OHOS::MessageParcel& data, const %s& %s)\n", type->GetName().c_str(),
        typeName.c_str(), objName.c_str());
    sb.Append("{\n");

    if (type->IsPod()) {
        sb.Append(TAB).AppendFormat("if (!data.WriteUnpadBuffer((const void*)&%s, sizeof(%s))) {\n",
            objName.c_str(), typeName.c_str());
        sb.Append(TAB).Append(TAB).Append("return false;\n");
        sb.Append(TAB).Append("}\n");
    } else {
        for (size_t i = 0; i < type->GetMemberNumber(); i++) {
            std::string memberName = type->GetMemberName(i);
            std::string name = StringHelper::Format("%s.%s", objName.c_str(), memberName.c_str());
            GetTypeEmitter(type->GetMemberType(i))->EmitCppMarshalling("data", name, sb, TAB);
            if (i + 1 < type->GetMemberNumber()) {
                sb.Append("\n");
            }
        }
    }

    sb.Append(TAB).Append("return true;\n");
    sb.Append("}\n\n");
}

void CppCustomTypesCodeEmitter::EmitCustomTypeUnmarshallingImpl(
    StringBuilder &sb, const AutoPtr<ASTStructType> &type) const
{
    AutoPtr<HdiTypeEmitter> typeEmitter = GetTypeEmitter(type.Get());
    if (typeEmitter == nullptr) {
        return;
    }
    std::string typeName = typeEmitter->EmitCppType();
    std::string objName("dataBlock");

    sb.AppendFormat("bool %sBlockUnmarshalling(OHOS::MessageParcel& data, %s& %s)\n", type->GetName().c_str(),
        typeName.c_str(), objName.c_str());
    sb.Append("{\n");

    if (type->IsPod()) {
        std::string objPtrName = StringHelper::Format("%sPtr", objName.c_str());
        sb.Append(TAB).AppendFormat("const %s *%s = reinterpret_cast<const %s*>(data.ReadUnpadBuffer(sizeof(%s)));\n",
            typeName.c_str(), objPtrName.c_str(), typeName.c_str(), typeName.c_str());
        sb.Append(TAB).AppendFormat("if (%s == NULL) {\n", objPtrName.c_str());
        sb.Append(TAB).Append(TAB).Append("return false;\n");
        sb.Append(TAB).Append("}\n\n");
        sb.Append(TAB).AppendFormat("if (memcpy_s(&%s, sizeof(%s), %s, sizeof(%s)) != EOK) {\n", objName.c_str(),
            typeName.c_str(), objPtrName.c_str(), typeName.c_str());
        sb.Append(TAB).Append(TAB).Append("return false;\n");
        sb.Append(TAB).Append("}\n");
    } else {
        for (size_t i = 0; i < type->GetMemberNumber(); i++) {
            AutoPtr<ASTType> memberType = type->GetMemberType(i);
            std::string memberName = type->GetMemberName(i);
            std::string name = StringHelper::Format("%s.%s", objName.c_str(), memberName.c_str());
            if (i > 0) {
                sb.Append("\n");
            }

            typeEmitter = GetTypeEmitter(memberType);
            if (typeEmitter == nullptr) {
                continue;
            }
            if (memberType->GetTypeKind() == TypeKind::TYPE_UNION) {
                std::string cpName = StringHelper::Format("%sCp", memberName.c_str());
                typeEmitter->EmitCppUnMarshalling("data", cpName, sb, TAB);
                sb.Append(TAB).AppendFormat("if (memcpy_s(&%s, sizeof(%s), %s, sizeof(%s)) != EOK) {\n",
                    name.c_str(), typeEmitter->EmitCppType().c_str(), cpName.c_str(),
                    typeEmitter->EmitCppType().c_str());
                sb.Append(TAB).Append(TAB).AppendFormat("HDF_LOGE(\"%%{public}s: failed to memcpy %s\", __func__);\n",
                    name.c_str());
                sb.Append(TAB).Append(TAB).Append("return false;\n");
                sb.Append(TAB).Append("}\n");
            } else if (memberType->GetTypeKind() == TypeKind::TYPE_STRING) {
                std::string cpName = StringHelper::Format("%sCp", memberName.c_str());
                typeEmitter->EmitCppUnMarshalling("data", cpName, sb, TAB);
                sb.Append(TAB).AppendFormat("%s = %s;\n", name.c_str(), cpName.c_str());
            } else {
                typeEmitter->EmitCppUnMarshalling("data", name, sb, TAB);
            }
        }
    }
    sb.Append(TAB).AppendFormat("return true;\n", objName.c_str());
    sb.Append("}\n");
}

void CppCustomTypesCodeEmitter::EmitBeginNamespace(StringBuilder &sb)
{
    std::vector<std::string> cppNamespaceVec = EmitCppNameSpaceVec(ast_->GetPackageName());
    for (const auto &nspace : cppNamespaceVec) {
        sb.AppendFormat("namespace %s {\n", nspace.c_str());
    }
}

void CppCustomTypesCodeEmitter::EmitEndNamespace(StringBuilder &sb)
{
    std::vector<std::string> cppNamespaceVec = EmitCppNameSpaceVec(ast_->GetPackageName());
    for (std::vector<std::string>::const_reverse_iterator nspaceIter = cppNamespaceVec.rbegin();
        nspaceIter != cppNamespaceVec.rend(); ++nspaceIter) {
        sb.AppendFormat("} // %s\n", nspaceIter->c_str());
    }
}

void CppCustomTypesCodeEmitter::EmitUtilMethods(StringBuilder &sb, bool isDecl)
{
    UtilMethodMap methods;
    for (const auto &typePair : ast_->GetTypes()) {
        AutoPtr<HdiTypeEmitter> typeEmitter = GetTypeEmitter(typePair.second);
        if (typeEmitter != nullptr) {
            typeEmitter->EmitCppWriteMethods(methods, "", "", isDecl);
            typeEmitter->EmitCppReadMethods(methods, "", "", isDecl);
        }
    }
    EmitUtilMethodMap(sb, methods);
}
} // namespace Idl
} // namespace OHOS
