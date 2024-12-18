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

#include "ast/ast_map_type.h"

namespace OHOS {
namespace Idl {
String ASTMapType::GetSignature()
{
    if (keyType_ == nullptr || valueType_ == nullptr) {
        return String::Format("LMap[];");
    }
    return String::Format("LMap[%s%s];", keyType_->GetSignature().string(), valueType_->GetSignature().string());
}

bool ASTMapType::IsMapType()
{
    return true;
}

String ASTMapType::ToString()
{
    if (keyType_ == nullptr || valueType_ == nullptr) {
        return String::Format("Map<, >");
    }
    return String::Format("Map<%s, %s>", keyType_->ToString().string(), valueType_->ToString().string());
}
} // namespace Idl
} // namespace OHOS
