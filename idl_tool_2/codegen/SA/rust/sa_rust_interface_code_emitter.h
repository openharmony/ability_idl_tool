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

#ifndef OHOS_IDL_SA_RUST_INTERFACE_CODE_EMITTER_H
#define OHOS_IDL_SA_RUST_INTERFACE_CODE_EMITTER_H

#include "sa_rust_code_emitter.h"

namespace OHOS {
namespace Idl {
class SaRustInterfaceCodeEmitter : public RustCodeEmitter {
public:
    SaRustInterfaceCodeEmitter() : RustCodeEmitter() {}

    ~SaRustInterfaceCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitInterfaceHeaderFile();

    void EmitHeaders(StringBuilder &sb) const;

    void EmitIPCHeaders(StringBuilder &sb) const;

    bool EmitCustomHeaders(StringBuilder &sb) const;

    bool AppendRealPath(StringBuilder& sb, const std::string &fpnpp) const;

    std::string GeneratePath(const std::string &fpnp) const;

    std::string TrimDot(const std::string &fpnp) const;

    void EmitCommands(StringBuilder &sb) const;

    std::string GetCodeFromMethod(const std::string &name) const;

    void EmitRemoteObject(StringBuilder &sb) const;

    void EmitBrokers(StringBuilder &sb) const;

    void WrapLine(StringBuilder &sb, int index, const std::string &prefix) const;

    void AppendBrokerParameters(StringBuilder &sb, AutoPtr<ASTMethod> &method) const;

    std::string GetNameFromParameter(const std::string &name) const;

    void EmitRemoteRequest(StringBuilder &sb) const;

    void AddRemoteRequestMethods(StringBuilder &sb) const;

    void AddRemoteRequestParameters(StringBuilder &sb, AutoPtr<ASTMethod> &method) const;

    void EmitStub(StringBuilder &sb) const;

    void AppendStubParameters(StringBuilder &sb, AutoPtr<ASTMethod> &method) const;

    void EmitProxy(StringBuilder &sb) const;

    void EmitProxyMethodImpl(AutoPtr<ASTMethod> &method, StringBuilder &sb) const;
};
} // namespace Idl
} // namespace OHOS

#endif // OHOS_IDL_RUST_INTERFACE_CODE_EMITTER_H