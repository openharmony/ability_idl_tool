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

#include "ast/ast_list_type.h"

namespace OHOS {
namespace Idl {
String ASTListType::GetSignature()
{
    if (elementType_ == nullptr) {
        return String::Format("LList[];");
    }
    return String::Format("LList[%s];", elementType_->GetSignature().string());
}

bool ASTListType::IsListType()
{
    return true;
}

String ASTListType::ToString()
{
    if (elementType_ == nullptr) {
        return String::Format("List<>");
    }
    return String::Format("List<%s>", elementType_->ToString().string());
}
} // namespace Idl
} // namespace OHOS
