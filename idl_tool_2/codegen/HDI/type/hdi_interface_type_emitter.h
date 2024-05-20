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

#ifndef OHOS_IDL_HDI_INTERFACETYPE_EMITTER_H
#define OHOS_IDL_HDI_INTERFACETYPE_EMITTER_H

#include "codegen/HDI/hdi_type_emitter.h"

namespace OHOS {
namespace Idl {
class HdiInterfaceTypeEmitter : public HdiTypeEmitter {
public:
    TypeKind GetTypeKind() override;

    std::string EmitCType(TypeMode mode = TypeMode::NO_MODE) const override;

    std::string EmitCppType(TypeMode mode = TypeMode::NO_MODE) const override;

    std::string EmitJavaType(TypeMode mode, bool isInnerType = false) const override;

    std::string EmitDescMacroName() const;

    void EmitCWriteVar(TypeMode mode, const std::string &name, const std::string &gotoLabel,
        StringBuilder &sb, const std::string &prefix) const override;

    void EmitCProxyReadVar(const std::string &name, bool isInnerType, const std::string &gotoLabel, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitCStubReadVar(const std::string &name, const std::string &gotoLabel, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitCppWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, unsigned int innerLevel = 0) const override;

    void EmitCppReadVar(const std::string &name, StringBuilder &sb, const std::string &prefix,
        TypeMode mode, unsigned int innerLevel = 0) const override;

    void EmitJavaWriteVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix, TypeMode mode = TypeMode::NO_MODE) const override;

    void EmitJavaReadVar(const std::string &parcelName, const std::string &name, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitJavaReadInnerVar(const std::string &parcelName, const std::string &name, bool isInner, StringBuilder &sb,
        const std::string &prefix) const override;

    void EmitCWriteMethods(UtilMethodMap &methods, const std::string &prefix, const std::string &methodPrefix,
        bool isDecl) const override;

    void EmitCReadMethods(
        UtilMethodMap &methods, const std::string &prefix, const std::string &methodPrefix, bool isDecl) const override;

    void EmitCStubReadMethods(UtilMethodMap &methods, const std::string &prefix, const std::string &methodPrefix,
        bool isDecl) const override;

private:
    std::string GetNamespaceWithColon() const;
};
} // namespace Idl
} // namespace OHOS

#endif // OHOS_IDL_HDI_INTERFACETYPE_EMITTER_H