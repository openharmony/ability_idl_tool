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

#ifndef OHOS_IDL_HDI_C_SERVICE_DRIVER_CODE_EMITTER_H
#define OHOS_IDL_HDI_C_SERVICE_DRIVER_CODE_EMITTER_H

#include "hdi_c_code_emitter.h"

namespace OHOS {
namespace Idl {
class CServiceDriverCodeEmitter : public HDICCodeEmitter {
public:
    CServiceDriverCodeEmitter();

    ~CServiceDriverCodeEmitter() override = default;

private:
    bool ResolveDirectory(const std::string &targetDirectory) override;

    void EmitCode() override;

    void EmitLowDriverSourceFile();

    void EmitLowDriverInclusions(StringBuilder &sb) const;

    void EmitLowDriverBind(StringBuilder &sb) const;

    void EmitLowDriverRelease(StringBuilder &sb) const;

    void EmitDriverSourceFile();

    void EmitDriverInclusions(StringBuilder &sb);

    void GetDriverSourceOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const;

    void EmitDriverServiceDecl(StringBuilder &sb) const;

    void EmitKernelDriverDispatch(StringBuilder &sb);

    void EmitDriverDispatch(StringBuilder &sb);

    void EmitDriverInit(StringBuilder &sb) const;

    void EmitKernelDriverBind(StringBuilder &sb);

    void EmitDriverBind(StringBuilder &sb);

    void EmitKernelDriverRelease(StringBuilder &sb);

    void EmitDriverRelease(StringBuilder &sb);

    void EmitDriverEntryDefinition(StringBuilder &sb) const;
private:
    std::string hostName_;
};
} // namespace Idl
} // namespace OHOS

#endif // OHOS_IDL_HDI_C_SERVICE_DRIVER_CODE_EMITTER_H