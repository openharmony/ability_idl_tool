/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#include <sstream>
#include "securec.h"
#include "util/file.h"
#include "codegen/cpp_code_emitter.h"

namespace OHOS {
namespace Idl {
void CppCodeEmitter::EmitInterface()
{
    EmitInterfaceHeaderFile();
}

void CppCodeEmitter::EmitInterfaceHeaderFile()
{
    String filePath = String::Format("%s/%s.h", directory_.string(), FileName(interfaceName_).string());
    File file(filePath, File::WRITE);

    StringBuilder sb;

    EmitLicense(sb);
    sb.Append("\n");
    EmitHeadMacro(sb, interfaceFullName_);
    sb.Append("\n");
    EmitInterfaceInclusions(sb);
    sb.Append("\n");
    if (EmitInterfaceUsings(sb)) {
        sb.Append("\n");
    }
    EmitInterfaceDefinition(sb);
    EmitTailMacro(sb, interfaceFullName_);

    String data = sb.ToString();
    file.WriteData(data.string(), data.GetLength());
    file.Flush();
    file.Close();
}

void CppCodeEmitter::EmitInterfaceInclusions(StringBuilder& sb)
{
    EmitInterfaceStdlibInclusions(sb);
    EmitInterfaceDBinderInclusions(sb);
    EmitInterfaceSelfDefinedTypeInclusions(sb);
}

void CppCodeEmitter::EmitInterfaceStdlibInclusions(StringBuilder& sb)
{
    bool includeString = false;
    bool includeList = false;
    bool includeMap = false;
    bool includeNum = false;
    if (metaComponent_ == nullptr) {
        return;
    }
    for (int i = 0; i < metaComponent_->typeNumber_; i++) {
        MetaType* mt = metaComponent_->types_[i];
        switch (mt->kind_) {
            case TypeKind::Byte: {
                if (!includeNum) {
                    sb.Append("#include <cstdint>\n");
                    includeNum = true;
                }
                break;
            }
            case TypeKind::String: {
                if (!includeString) {
                    sb.Append("#include <string_ex.h>\n");
                    includeString = true;
                }
                break;
            }
            case TypeKind::Array:
            case TypeKind::List: {
                if (!includeList) {
                    sb.Append("#include <vector>\n");
                    includeList = true;
                }
                break;
            }
            case TypeKind::Map: {
                if (!includeMap) {
                    sb.Append("#include <unordered_map>\n");
                    includeMap = true;
                }
                break;
            }
            default:
                break;
        }
    }
}

void CppCodeEmitter::EmitInterfaceDBinderInclusions(StringBuilder& sb)
{
    sb.Append("#include <iremote_broker.h>\n");
    if (logOn_) {
        sb.Append("#include \"hilog/log.h\"\n");
    }
}

String CppCodeEmitter::GetFilePath(const String& fpnp)
{
    int pos = fpnp.IndexOf("..");
    if (pos == -1) {
        return String();
    }
    String res = fpnp.Substring(0, pos + 1);
    return res;
}

String CppCodeEmitter::GetFilePathNoPoint(const String& fpnp)
{
    int pos = fpnp.IndexOf("..");
    if (pos == -1) {
        return String();
    }
    String res = fpnp.Substring(0, pos);
    return res;
}

String CppCodeEmitter::GetNamespace(const String& fpnp)
{
    int pos = fpnp.IndexOf("..");
    if (pos == -1) {
        return fpnp;
    }
    String res = fpnp.Substring(pos + 2);
    return res;
}

void CppCodeEmitter::EmitInterfaceSelfDefinedTypeInclusions(StringBuilder& sb)
{
    for (int i = 0; i < metaComponent_->sequenceableNumber_; i++) {
        MetaSequenceable* mp = metaComponent_->sequenceables_[i];
        if (mp == nullptr) {
            continue;
        }
        String filePath = GetFilePathNoPoint(String(mp->namespace_));
        String fileName;
        String sequenceName = mp->name_;
        filePath.IsEmpty() ? fileName = FileName(sequenceName) : fileName = FileName(filePath);
        sb.Append("#include ").AppendFormat("\"%s.h\"\n", fileName.string());
    }

    for (int i = 0; i < metaComponent_->interfaceNumber_; i++) {
        MetaInterface* mi = metaComponent_->interfaces_[i];
        if (mi->external_) {
            String filePath = GetFilePath(String(mi->namespace_));
            String fileName = FileName(filePath + mi->name_);
            sb.Append("#include ").AppendFormat("\"%s.h\"\n", fileName.string());
        }
    }
}

bool CppCodeEmitter::EmitInterfaceUsings(StringBuilder& sb)
{
    bool ret = false;
    for (int i = 0; i < metaComponent_->sequenceableNumber_; i++) {
        MetaSequenceable* mp = metaComponent_->sequenceables_[i];
        String np = GetNamespace(String(mp->namespace_));
        if (np.IsEmpty()) {
            continue;
        }
        String fullName = CppFullName(np + mp->name_);
        sb.Append("using ").AppendFormat("%s;\n", fullName.string());
        ret = true;
    }

    for (int i = 0; i < metaComponent_->interfaceNumber_; i++) {
        MetaInterface* mi = metaComponent_->interfaces_[i];
        if (mi->external_) {
            String np = GetNamespace(String(mi->namespace_));
            if (np.IsEmpty()) {
                continue;
            }
            String fullName = CppFullName(np + mi->name_);
            sb.Append("using ").AppendFormat("%s;\n", fullName.string());
            ret = true;
        }
    }
    return ret;
}

void CppCodeEmitter::EmitInterfaceDefinition(StringBuilder& sb)
{
    EmitBeginNamespace(sb);
    sb.AppendFormat("class %s : public IRemoteBroker {\n", metaInterface_->name_);
    sb.Append("public:\n");
    EmitInterfaceBody(sb, TAB);
    EmitInterfaceMemberVariables(sb, TAB);
    sb.Append("};\n");
    EmitEndNamespace(sb);
}

void CppCodeEmitter::EmitInterfaceBody(StringBuilder& sb, const String& prefix)
{
    String nameWithoutPath = GetNamespace(interfaceFullName_);
    sb.Append(TAB).AppendFormat("DECLARE_INTERFACE_DESCRIPTOR(u\"%s\");\n", nameWithoutPath.string());
    sb.Append("\n");
    EmitInterfaceMethods(sb, TAB);
}

void CppCodeEmitter::EmitInterfaceMethods(StringBuilder& sb, const String& prefix)
{
    if (metaInterface_->methodNumber_ > 0) {
        for (int i = 0; i < metaInterface_->methodNumber_; i++) {
            MetaMethod* mm = metaInterface_->methods_[i];
            EmitInterfaceMethod(mm, sb, prefix);
            if (i != metaInterface_->methodNumber_ - 1) {
                sb.Append("\n");
            }
        }
    }
}

void CppCodeEmitter::EmitInterfaceMethod(MetaMethod* mm, StringBuilder& sb, const String& prefix)
{
    MetaType* returnType = metaComponent_->types_[mm->returnTypeIndex_];
    if (mm->parameterNumber_ == 0 && returnType->kind_ == TypeKind::Void) {
        sb.Append(prefix).AppendFormat("virtual ErrCode %s() = 0;\n", mm->name_);
    } else {
        sb.Append(prefix).AppendFormat("virtual ErrCode %s(\n", mm->name_);
        for (int i = 0; i < mm->parameterNumber_; i++) {
            MetaParameter* mp = mm->parameters_[i];
            EmitInterfaceMethodParameter(mp, sb, prefix + TAB);
            if (i != mm->parameterNumber_ - 1 || returnType->kind_ != TypeKind::Void) {
                sb.Append(",\n");
            }
        }
        if (returnType->kind_ != TypeKind::Void) {
            EmitInterfaceMethodReturn(returnType, sb, prefix + TAB);
        }
        sb.Append(") = 0;\n");
    }
}

void CppCodeEmitter::EmitInterfaceMethodParameter(MetaParameter* mp, StringBuilder& sb, const String& prefix)
{
    sb.Append(prefix);

    MetaType* mt = metaComponent_->types_[mp->typeIndex_];
    const std::string name = mp->name_;
    sb.AppendFormat("%s %s", EmitType(mt, mp->attributes_, false).string(), name.c_str());
}

void CppCodeEmitter::EmitInterfaceMethodReturn(MetaType* mt, StringBuilder& sb, const String& prefix)
{
    sb.Append(prefix).AppendFormat("%s result", EmitType(mt, ATTR_OUT, false).string());
}

void CppCodeEmitter::EmitInterfaceMemberVariables(StringBuilder& sb, const String& prefix)
{
    sb.Append("protected:\n");
    if (!domainId_.IsNull() && !logTag_.IsNull()) {
        sb.Append(prefix).AppendFormat(
            "static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, %s, \"%s\"};\n",
            domainId_.string(), logTag_.string());
    }
    sb.Append(prefix).Append("const int VECTOR_MAX_SIZE = 102400;\n");
    sb.Append(prefix).Append("const int LIST_MAX_SIZE = 102400;\n");
    sb.Append(prefix).Append("const int MAP_MAX_SIZE = 102400;\n");
}

void CppCodeEmitter::EmitInterfaceProxy()
{
    EmitInterfaceProxyHeaderFile();
    EmitInterfaceProxyCppFile();
}

void CppCodeEmitter::EmitInterfaceProxyHeaderFile()
{
    String filePath = String::Format("%s/%s.h", directory_.string(), FileName(proxyName_).string());
    File file(filePath, File::WRITE);

    StringBuilder sb;

    EmitLicense(sb);
    sb.Append("\n");
    EmitHeadMacro(sb, proxyFullName_);
    sb.Append("\n");
    sb.AppendFormat("#include \"%s.h\"\n", FileName(interfaceName_).string());
    sb.Append("#include <iremote_proxy.h>\n");
    if (metaComponent_->hasCacheableProxyMethods_ == true) {
        sb.Append("#include \"api_cache_manager.h\"\n");
    }
    sb.Append("\n");
    EmitInterfaceProxyInHeaderFile(sb);
    EmitTailMacro(sb, proxyFullName_);

    String data = sb.ToString();
    file.WriteData(data.string(), data.GetLength());
    file.Flush();
    file.Close();
}

void CppCodeEmitter::EmitInterfaceProxyInHeaderFile(StringBuilder& sb)
{
    EmitBeginNamespace(sb);
    sb.AppendFormat("class %s : public IRemoteProxy<%s> {\n", proxyName_.string(), interfaceName_.string());
    sb.Append("public:\n");
    EmitInterfaceProxyConstructor(sb, TAB);
    sb.Append("\n");
    EmitInterfaceProxyMethodDecls(sb, TAB);
    sb.Append("\n");
    sb.Append("private:\n");
    EmitInterfaceProxyConstants(sb, TAB);
    sb.Append("};\n");
    EmitEndNamespace(sb);
}

void CppCodeEmitter::EmitInterfaceProxyRegisterDeathRecipient(StringBuilder& sb, const String& prefix)
{
    sb.Append(prefix).Append("if (remote != nullptr) {\n");
    sb.Append(prefix + TAB).Append("if (!remote->IsProxyObject()) {\n");
    if (logOn_) {
        sb.Append(prefix + TAB + TAB).Append("HiLog::Error(LABEL, \"remote is not proxy object!\");\n");
    }
    sb.Append(prefix + TAB + TAB).Append("return;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).AppendFormat("deathRecipient_ = new (std::nothrow) %s(*this);\n",
        deathRecipientName_.string());
    sb.Append(prefix + TAB).Append("if (deathRecipient_ == nullptr) {\n");
    if (logOn_) {
        sb.Append(prefix + TAB + TAB).Append("HiLog::Error(LABEL, \"deathRecipient_ is nullptr!\");\n");
    }
    sb.Append(prefix + TAB + TAB).Append("return;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("if (!remote->AddDeathRecipient(deathRecipient_)) {\n");
    if (logOn_) {
        sb.Append(prefix + TAB + TAB).Append("HiLog::Error(LABEL, \"AddDeathRecipient failed!\");\n");
    }
    sb.Append(prefix + TAB + TAB).Append("return;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("remote_ = remote;\n");
    sb.Append(prefix).Append("}\n");
}

void CppCodeEmitter::EmitInterfaceProxyAddCacheAbleAPI(StringBuilder& sb, const String& prefix)
{
    sb.Append("\n");
    if (metaComponent_->hasCacheableProxyMethods_ == false) {
        return;
    }
    for (int i = 0; i < metaInterface_->methodNumber_; i++) {
        MetaMethod* mm = metaInterface_->methods_[i];
        if (mm->cacheable_ == false) {
            continue;
        }
        if (mm->cacheabletime_ != 0) {
            sb.Append(prefix + TAB).AppendFormat(
                "ApiCacheManager::GetInstance().AddCacheApi(GetDescriptor(), COMMAND_%s, %d000);\n",
                ConstantName(mm->name_).string(), mm->cacheabletime_);
        } else {
            sb.Append(prefix + TAB).AppendFormat(
                "ApiCacheManager::GetInstance().AddCacheApi(GetDescriptor(), COMMAND_%s, 0);\n",
                ConstantName(mm->name_).string());
        }
    }
    sb.Append("\n");
    EmitInterfaceProxyRegisterDeathRecipient(sb, prefix + TAB);
}

void CppCodeEmitter::EmitInterfaceProxyUnRegisterDeathRecipient(StringBuilder& sb, const String& prefix)
{
    sb.Append(prefix).Append("if (remote_ == nullptr) {\n");
    sb.Append(prefix).Append(TAB).Append("return;\n");
    sb.Append(prefix).Append("}\n");
    sb.Append(prefix).Append("if (deathRecipient_ == nullptr) {\n");
    sb.Append(prefix).Append(TAB).Append("return;\n");
    sb.Append(prefix).Append("}\n");
    sb.Append(prefix).Append("remote_->RemoveDeathRecipient(deathRecipient_);\n");
    sb.Append(prefix).Append("remote_ = nullptr;\n");
    if (metaInterface_->methodNumber_ > 0) {
        sb.Append("\n");
        for (int i = 0; i < metaInterface_->methodNumber_; i++) {
            MetaMethod* mm = metaInterface_->methods_[i];
            if (mm->cacheable_) {
                sb.Append(prefix).AppendFormat(
                    "ApiCacheManager::GetInstance().DelCacheApi(GetDescriptor(), COMMAND_%s);\n",
                    ConstantName(mm->name_).string());
            }
        }
    }
}

void CppCodeEmitter::EmitInterfaceProxyConstructor(StringBuilder& sb, const String& prefix)
{
    sb.Append(prefix).AppendFormat("explicit %s(\n", proxyName_.string());
    sb.Append(prefix + TAB).Append("const sptr<IRemoteObject>& remote)\n");
    sb.Append(prefix + TAB).AppendFormat(": IRemoteProxy<%s>(remote)\n", interfaceName_.string());
    sb.Append(prefix).Append("{");
    EmitInterfaceProxyAddCacheAbleAPI(sb, prefix);
    sb.Append(prefix).Append("}\n\n");
    sb.Append(prefix).AppendFormat("virtual ~%s()\n", proxyName_.string());
    sb.Append(prefix).Append("{\n");
    if (metaComponent_->hasCacheableProxyMethods_) {
        EmitInterfaceProxyUnRegisterDeathRecipient(sb, prefix + TAB);
    }
    sb.Append(prefix).Append("}\n");
}

void CppCodeEmitter::EmitInterfaceProxyMethodDecls(StringBuilder& sb, const String& prefix)
{
    if (metaInterface_->methodNumber_ > 0) {
        for (int i = 0; i < metaInterface_->methodNumber_; i++) {
            MetaMethod* mm = metaInterface_->methods_[i];
            EmitInterfaceProxyMethodDecl(mm, sb, prefix);
            if (i != metaInterface_->methodNumber_ - 1) {
                sb.Append("\n");
            }
        }
    }
}

void CppCodeEmitter::EmitInterfaceProxyMethodDecl(MetaMethod* mm, StringBuilder& sb, const String& prefix)
{
    MetaType* returnType = metaComponent_->types_[mm->returnTypeIndex_];
    if (mm->parameterNumber_ == 0 && returnType->kind_ == TypeKind::Void) {
        sb.Append(prefix).AppendFormat("ErrCode %s() override;\n", mm->name_);
    } else {
        sb.Append(prefix).AppendFormat("ErrCode %s(\n", mm->name_);
        for (int i = 0; i < mm->parameterNumber_; i++) {
            MetaParameter* mp = mm->parameters_[i];
            EmitInterfaceMethodParameter(mp, sb, prefix + TAB);
            if (i != mm->parameterNumber_ - 1 || returnType->kind_ != TypeKind::Void) {
                sb.Append(",\n");
            }
        }
        if (returnType->kind_ != TypeKind::Void) {
            EmitInterfaceMethodReturn(returnType, sb, prefix + TAB);
        }
        sb.Append(") override;\n");
    }
}

void CppCodeEmitter::EmitInterfaceProxyDeathRecipient(StringBuilder& sb, const String& prefix)
{
    sb.Append(prefix).AppendFormat("class %s : public IRemoteObject::DeathRecipient {\n", deathRecipientName_.string());
    sb.Append(prefix).Append("public:\n");
    sb.Append(prefix + TAB).AppendFormat("explicit %s(%s &client) : client_(client) {}\n", deathRecipientName_.string(),
        proxyName_.string());
    sb.Append(prefix + TAB).AppendFormat("~%s() override = default;\n", deathRecipientName_.string());
    sb.Append(prefix + TAB).Append("void OnRemoteDied(const wptr<IRemoteObject> &remote) override\n");
    sb.Append(prefix + TAB).Append("{\n");
    sb.Append(prefix + TAB + TAB).Append("client_.OnRemoteDied(remote);\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix).Append("private:\n");
    sb.Append(prefix + TAB).AppendFormat("%s &client_;\n", proxyName_.string());
    sb.Append(prefix).Append("};\n\n");

    sb.Append(prefix).Append("void OnRemoteDied(const wptr<IRemoteObject> &remoteObject)\n");
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).Append("(void)remoteObject;\n");
    sb.Append(prefix + TAB).Append("ApiCacheManager::GetInstance().ClearCache(GetDescriptor());\n");
    sb.Append(prefix).Append("}\n");
    sb.Append(prefix).Append("sptr<IRemoteObject> remote_ = nullptr;\n");
    sb.Append(prefix).Append("sptr<IRemoteObject::DeathRecipient> deathRecipient_ = nullptr;\n");
}

void CppCodeEmitter::EmitInterfaceProxyConstants(StringBuilder& sb, const String& prefix)
{
    if (metaComponent_->hasCacheableProxyMethods_) {
        EmitInterfaceProxyDeathRecipient(sb, prefix);
    }
    EmitInterfaceMethodCommands(sb, prefix);
    sb.Append("\n");
    sb.Append(prefix).AppendFormat("static inline BrokerDelegator<%s> delegator_;\n", proxyName_.string());
}

void CppCodeEmitter::EmitInterfaceProxyCppFile()
{
    String filePath = String::Format("%s/%s.cpp", directory_.string(), FileName(proxyName_).string());
    File file(filePath, File::WRITE);

    StringBuilder sb;

    EmitLicense(sb);
    sb.Append("\n");
    sb.AppendFormat("#include \"%s.h\"\n", FileName(proxyName_).string());
    if (logOn_) {
        sb.Append("#include \"hilog/log.h\"\n");
    }
    if (hitraceOn_) {
        sb.Append("#include \"hitrace_meter.h\"\n");
    }
    sb.Append("\n");
    if (logOn_) {
        sb.Append("using OHOS::HiviewDFX::HiLog;\n\n");
    }
    EmitBeginNamespace(sb);
    EmitInterfaceProxyMethodImpls(sb, "");
    EmitEndNamespace(sb);

    String data = sb.ToString();
    file.WriteData(data.string(), data.GetLength());
    file.Flush();
    file.Close();
}

void CppCodeEmitter::EmitInterfaceProxyMethodImpls(StringBuilder& sb, const String& prefix)
{
    if (metaInterface_->methodNumber_ > 0) {
        for (int i = 0; i < metaInterface_->methodNumber_; i++) {
            MetaMethod* mm = metaInterface_->methods_[i];
            EmitInterfaceProxyMethodImpl(mm, sb, prefix);
            if (i != metaInterface_->methodNumber_ - 1) {
                sb.Append("\n");
            }
        }
    }
}

void CppCodeEmitter::EmitInterfaceProxyMethodImpl(MetaMethod* mm, StringBuilder& sb, const String& prefix)
{
    MetaType* returnType = metaComponent_->types_[mm->returnTypeIndex_];
    if (mm->parameterNumber_ == 0 && returnType->kind_ == TypeKind::Void) {
        sb.Append(prefix).AppendFormat("ErrCode %s::%s()\n", proxyName_.string(), mm->name_);
    } else {
        sb.Append(prefix).AppendFormat("ErrCode %s::%s(\n", proxyName_.string(), mm->name_);
        for (int i = 0; i < mm->parameterNumber_; i++) {
            MetaParameter* mp = mm->parameters_[i];
            EmitInterfaceMethodParameter(mp, sb, prefix + TAB);
            if (i != mm->parameterNumber_ - 1 || returnType->kind_ != TypeKind::Void) {
                sb.Append(",\n");
            }
        }
        if (returnType->kind_ != TypeKind::Void) {
            EmitInterfaceMethodReturn(returnType, sb, prefix + TAB);
        }
        sb.Append(")\n");
    }
    EmitInterfaceProxyMethodBody(mm, sb, prefix);
}

void CppCodeEmitter::EmitInterfaceProxyMethodPreSendRequest(MetaMethod* mm, StringBuilder& sb, const String& prefix)
{
    if ((mm->cacheable_ == true) && ((mm->properties_ & METHOD_PROPERTY_ONEWAY) == 0)) {
        sb.Append("\n");
        sb.Append(prefix + TAB).AppendFormat(
            "bool hitCache = ApiCacheManager::GetInstance().PreSendRequest(GetDescriptor(), COMMAND_%s, data, reply);",
            ConstantName(mm->name_).string());
        sb.Append("\n");
        sb.Append(prefix + TAB).Append("if (hitCache == true) {\n");
        EmitInterfaceProxyMethodErrCode(sb, prefix + TAB);
        EmitInterfaceProxyMethodReply(mm, sb, prefix + TAB);
        sb.Append(prefix + TAB + TAB).Append("return ERR_OK;\n");
        sb.Append(prefix + TAB).Append("}\n\n");
    }
}

void CppCodeEmitter::EmitInterfaceProxyMethodPostSendRequest(MetaMethod* mm, StringBuilder& sb, const String& prefix)
{
    sb.Append(prefix + TAB).AppendFormat(
        "ApiCacheManager::GetInstance().PostSendRequest(GetDescriptor(), COMMAND_%s, data, reply);\n",
        ConstantName(mm->name_).string());
}

void CppCodeEmitter::EmitInterfaceProxyMethodBody(MetaMethod* mm, StringBuilder& sb, const String& prefix)
{
    sb.Append(prefix).Append("{\n");
    if (hitraceOn_) {
        sb.Append(prefix + TAB).AppendFormat("HITRACE_METER_NAME(%s, __PRETTY_FUNCTION__);\n",
            hitraceTag_.string());
    }
    sb.Append(prefix + TAB).Append("MessageParcel data;\n");
    sb.Append(prefix + TAB).Append("MessageParcel reply;\n");
    sb.Append(prefix + TAB).AppendFormat("MessageOption option(%s);\n",
        (mm->properties_ & METHOD_PROPERTY_ONEWAY) != 0 ? "MessageOption::TF_ASYNC" : "MessageOption::TF_SYNC");
    sb.Append("\n");
    sb.Append(prefix + TAB).Append("if (!data.WriteInterfaceToken(GetDescriptor())) {\n");
    if (logOn_) {
        sb.Append(prefix + TAB).Append(TAB).Append("HiLog::Error(LABEL, \"Write interface token failed!\");\n");
    }
    sb.Append(prefix + TAB).Append(TAB).Append("return ERR_INVALID_VALUE;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append("\n");

    for (int i = 0; i < mm->parameterNumber_; i++) {
        MetaParameter* mp = mm->parameters_[i];
        if ((mp->attributes_ & ATTR_IN) != 0) {
            EmitWriteMethodParameter(mp, "data.", sb, prefix + TAB);
        }
    }
    EmitInterfaceProxyMethodPreSendRequest(mm, sb, prefix);
    sb.Append(prefix + TAB).Append("sptr<IRemoteObject> remote = Remote();\n");
    sb.Append(prefix + TAB).Append("if (remote == nullptr) {\n");
    if (logOn_) {
        sb.Append(prefix + TAB).Append(TAB).Append("HiLog::Error(LABEL, \"Remote is nullptr!\");\n");
    }
    sb.Append(prefix + TAB).Append(TAB).Append("return ERR_INVALID_DATA;\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).AppendFormat("int32_t result = remote->SendRequest(COMMAND_%s, data, reply, option);\n",
        ConstantName(mm->name_).string());
    sb.Append(prefix + TAB).Append("if (FAILED(result)) {\n");
    if (logOn_) {
        sb.Append(prefix + TAB).Append(TAB).Append("HiLog::Error(LABEL, \"Send request failed!\");\n");
    }
    sb.Append(prefix + TAB).Append("    return result;\n");
    sb.Append(prefix + TAB).Append("}\n");
    EmitInterfaceProxyMethodRetValue(mm, sb, prefix);
    sb.Append(prefix).Append("}\n");
}

void CppCodeEmitter::EmitInterfaceProxyMethodErrCode(StringBuilder& sb, const String& prefix)
{
    sb.Append(prefix + TAB).Append("ErrCode errCode = reply.ReadInt32();\n");
    sb.Append(prefix + TAB).Append("if (FAILED(errCode)) {\n");
    if (logOn_) {
        sb.Append(prefix + TAB + TAB).Append("HiLog::Error(LABEL, \"Read Int32 failed!\");\n");
    }
    sb.Append(prefix + TAB).Append("    return errCode;\n");
    sb.Append(prefix + TAB).Append("}\n");
}

void CppCodeEmitter::EmitInterfaceProxyMethodReply(MetaMethod* mm, StringBuilder& sb, const String& prefix)
{
    for (int i = 0; i < mm->parameterNumber_; i++) {
        MetaParameter* mp = mm->parameters_[i];
        if ((mp->attributes_ & ATTR_OUT) != 0) {
            EmitReadMethodParameter(mp, "reply.", sb, prefix + TAB);
        }
    }
    MetaType* returnType = metaComponent_->types_[mm->returnTypeIndex_];
    if (returnType->kind_ != TypeKind::Void) {
        EmitReadVariable("reply.", "result", returnType, sb, prefix + TAB, false);
    }
}

void CppCodeEmitter::EmitInterfaceProxyMethodRetValue(MetaMethod* mm, StringBuilder& sb, const String& prefix)
{
    if ((mm->properties_ & METHOD_PROPERTY_ONEWAY) == 0) {
        EmitInterfaceProxyMethodErrCode(sb, prefix);
        sb.Append("\n");
        if (mm->cacheable_ == true) {
            EmitInterfaceProxyMethodPostSendRequest(mm, sb, prefix);
        }
        EmitInterfaceProxyMethodReply(mm, sb, prefix);
    }
    sb.Append(prefix + TAB).Append("return ERR_OK;\n");
}

void CppCodeEmitter::EmitWriteMethodParameter(MetaParameter* mp, const String& parcelName, StringBuilder& sb,
    const String& prefix)
{
    MetaType* mt = metaComponent_->types_[mp->typeIndex_];
    const std::string name = mp->name_;
    EmitWriteVariable(parcelName, name, mt, sb, prefix);
}

void CppCodeEmitter::EmitReadMethodParameter(MetaParameter* mp, const String& parcelName, StringBuilder& sb,
    const String& prefix)
{
    MetaType* mt = metaComponent_->types_[mp->typeIndex_];
    const std::string name = mp->name_;
    EmitReadVariable(parcelName, name, mt, sb, prefix, false);
}

void CppCodeEmitter::EmitInterfaceStub()
{
    EmitInterfaceStubHeaderFile();
    EmitInterfaceStubCppFile();
}

void CppCodeEmitter::EmitInterfaceStubHeaderFile()
{
    String filePath = String::Format("%s/%s.h", directory_.string(), FileName(stubName_).string());
    File file(filePath, File::WRITE);

    StringBuilder sb;

    EmitLicense(sb);
    sb.Append("\n");
    EmitHeadMacro(sb, stubFullName_);
    sb.Append("\n");
    sb.AppendFormat("#include \"%s.h\"\n", FileName(interfaceName_).string());
    sb.Append("#include <iremote_stub.h>\n");
    sb.Append("\n");
    EmitInterfaceStubInHeaderFile(sb);
    EmitTailMacro(sb, stubFullName_);

    String data = sb.ToString();
    file.WriteData(data.string(), data.GetLength());
    file.Flush();
    file.Close();
}

void CppCodeEmitter::EmitInterfaceStubInHeaderFile(StringBuilder& sb)
{
    EmitBeginNamespace(sb);
    sb.AppendFormat("class %s : public IRemoteStub<%s> {\n", stubName_.string(), interfaceName_.string());
    sb.Append("public:\n");
    EmitInterfaceStubMethodDecls(sb, TAB);
    sb.Append("\n");
    sb.Append("private:\n");
    EmitInterfaceStubConstants(sb, TAB);
    sb.Append("};\n");
    EmitEndNamespace(sb);
}

void CppCodeEmitter::EmitInterfaceStubMethodDecls(StringBuilder& sb, const String& prefix)
{
    sb.Append(prefix).Append("int32_t OnRemoteRequest(\n");
    sb.Append(prefix + TAB).Append("uint32_t code,\n");
    sb.Append(prefix + TAB).Append("MessageParcel& data,\n");
    sb.Append(prefix + TAB).Append("MessageParcel& reply,\n");
    sb.Append(prefix + TAB).Append("MessageOption& option) override;\n");
}

void CppCodeEmitter::EmitInterfaceStubConstants(StringBuilder& sb, const String& prefix)
{
    EmitInterfaceMethodCommands(sb, prefix);
}

void CppCodeEmitter::EmitInterfaceStubCppFile()
{
    String filePath = String::Format("%s/%s.cpp", directory_.string(), FileName(stubName_).string());
    File file(filePath, File::WRITE);

    StringBuilder sb;

    EmitLicense(sb);
    sb.Append("\n");
    sb.AppendFormat("#include \"%s.h\"\n", FileName(stubName_).string());
    if (logOn_) {
        sb.Append("#include \"hilog/log.h\"\n");
    }
    if (hitraceOn_) {
        sb.Append("#include \"hitrace_meter.h\"\n");
    }
    sb.Append("\n");
    if (logOn_) {
        sb.Append("using OHOS::HiviewDFX::HiLog;\n\n");
    }
    EmitBeginNamespace(sb);
    EmitInterfaceStubMethodImpls(sb, "");
    EmitEndNamespace(sb);

    String data = sb.ToString();
    file.WriteData(data.string(), data.GetLength());
    file.Flush();
    file.Close();
}

void CppCodeEmitter::EmitInterfaceStubMethodImpls(StringBuilder& sb, const String& prefix)
{
    sb.Append(prefix).AppendFormat("int32_t %s::OnRemoteRequest(\n", stubName_.string());
    sb.Append(prefix + TAB).Append("uint32_t code,\n");
    sb.Append(prefix + TAB).Append("MessageParcel& data,\n");
    sb.Append(prefix + TAB).Append("MessageParcel& reply,\n");
    sb.Append(prefix + TAB).Append("MessageOption& option)\n");
    sb.Append(prefix).Append("{\n");
    if (hitraceOn_) {
    sb.Append(prefix + TAB).AppendFormat("HITRACE_METER_NAME(%s, __PRETTY_FUNCTION__);\n",
        hitraceTag_.string());
    }
    sb.Append(prefix + TAB).Append("std::u16string localDescriptor = GetDescriptor();\n");
    sb.Append(prefix + TAB).Append("std::u16string remoteDescriptor = data.ReadInterfaceToken();\n");
    sb.Append(prefix + TAB).Append("if (localDescriptor != remoteDescriptor) {\n");
    sb.Append(prefix + TAB).Append(TAB).Append("return ERR_TRANSACTION_FAILED;\n");
    sb.Append(prefix + TAB).Append("}\n");
    sb.Append(prefix + TAB).Append("switch (code) {\n");
    for (int i = 0; i < metaInterface_->methodNumber_; i++) {
        MetaMethod* mm = metaInterface_->methods_[i];
        EmitInterfaceStubMethodImpl(mm, sb, prefix + TAB + TAB);
    }
    sb.Append(prefix + TAB).Append(TAB).Append("default:\n");
    sb.Append(prefix + TAB).Append(TAB).Append(TAB).Append(
        "return IPCObjectStub::OnRemoteRequest(code, data, reply, option);\n");
    sb.Append(prefix + TAB).Append("}\n\n");
    sb.Append(prefix + TAB).Append("return ERR_TRANSACTION_FAILED;\n");
    sb.Append(prefix).Append("}\n");
}

void CppCodeEmitter::EmitInterfaceStubMethodImplReturn(MetaMethod* mm,
    StringBuilder& sb, const String& prefix, MetaType* returnType)
{
    if (returnType->kind_ != TypeKind::Void) {
        if ((returnType->kind_ == TypeKind::Sequenceable) || (returnType->kind_ == TypeKind::Interface)) {
            sb.Append(prefix + TAB).AppendFormat("%s result = nullptr;\n",
                EmitType(returnType, ATTR_IN, true).string());
        } else {
            sb.Append(prefix + TAB).AppendFormat("%s result;\n", EmitType(returnType, ATTR_IN, true).string());
        }
    }
    if (mm->parameterNumber_ == 0 && returnType->kind_ == TypeKind::Void) {
        sb.Append(prefix + TAB).AppendFormat("ErrCode errCode = %s();\n", mm->name_);
    } else {
        sb.Append(prefix + TAB).AppendFormat("ErrCode errCode = %s(", mm->name_);
        for (int i = 0; i < mm->parameterNumber_; i++) {
            MetaParameter* mp = mm->parameters_[i];
            if (mp == nullptr) {
                continue;
            }
            const std::string name = mp->name_;
            MetaType* mGetType = metaComponent_->types_[mp->typeIndex_];
            if (mGetType != nullptr && mGetType->kind_ == TypeKind::Sequenceable && !mp->isSequenceableForOut_) {
                const std::string parameterName = "*" + name;
                sb.Append(parameterName.c_str());
            } else {
                sb.Append(name.c_str());
            }

            if (i != mm->parameterNumber_ - 1 || returnType->kind_ != TypeKind::Void) {
                sb.Append(", ");
            }
        }
        if (returnType->kind_ != TypeKind::Void) {
            EmitReturnParameter("result", returnType, sb);
        }
        sb.AppendFormat(");\n", mm->name_);
    }
}

void CppCodeEmitter::EmitInterfaceStubMethodImpl(MetaMethod* mm, StringBuilder& sb, const String& prefix)
{
    sb.Append(prefix).AppendFormat("case COMMAND_%s: {\n", ConstantName(mm->name_).string());
    for (int i = 0; i < mm->parameterNumber_; i++) {
        MetaParameter* mp = mm->parameters_[i];
        if ((mp->attributes_ & ATTR_IN) != 0) {
            MetaType* mt = metaComponent_->types_[mp->typeIndex_];
            const std::string name = mp->name_;
            EmitReadVariable("data.", name, mt, sb, prefix + TAB);
        } else if ((mp->attributes_ & ATTR_OUT) != 0) {
            EmitLocalVariable(mp, sb, prefix + TAB);
        }
    }
    MetaType* returnType = metaComponent_->types_[mm->returnTypeIndex_];
    if (returnType == nullptr) {
        return;
    }
    EmitInterfaceStubMethodImplReturn(mm, sb, prefix, returnType);

    sb.Append(prefix + TAB).Append("if (!reply.WriteInt32(errCode)) {\n");
    if (logOn_) {
        sb.Append(prefix + TAB).Append(TAB).Append("HiLog::Error(LABEL, \"Write Int32 failed!\");\n");
    }
    sb.Append(prefix + TAB).Append(TAB).Append("return ERR_INVALID_VALUE;\n");
    sb.Append(prefix + TAB).Append("}\n");
    bool hasOutParameter = false;
    for (int i = 0; i < mm->parameterNumber_; i++) {
        MetaParameter* mp = mm->parameters_[i];
        if ((mp->attributes_ & ATTR_OUT) != 0) {
            hasOutParameter = true;
        }
    }
    if (hasOutParameter || returnType->kind_ != TypeKind::Void) {
        sb.Append(prefix + TAB).Append("if (SUCCEEDED(errCode)) {\n");
        for (int i = 0; i < mm->parameterNumber_; i++) {
            MetaParameter* mp = mm->parameters_[i];
            if ((mp->attributes_ & ATTR_OUT) != 0) {
                EmitWriteMethodParameter(mp, "reply.", sb, prefix + TAB + TAB);
            }
        }
        if (returnType->kind_ != TypeKind::Void) {
            EmitWriteVariable("reply.", "result", returnType, sb, prefix + TAB + TAB);
        }
        sb.Append(prefix + TAB).Append("}\n");
    }
    sb.Append(prefix + TAB).Append("return ERR_NONE;\n");
    sb.Append(prefix).Append("}\n");
}

void CppCodeEmitter::EmitInterfaceMethodCommands(StringBuilder& sb, const String& prefix)
{
    for (int i = 0; i < metaInterface_->methodNumber_; i++) {
        MetaMethod* mm = metaInterface_->methods_[i];
        sb.Append(prefix).AppendFormat("static constexpr int32_t COMMAND_%s = MIN_TRANSACTION_ID + %d;\n",
            ConstantName(mm->name_).string(), i);
    }
}

void CppCodeEmitter::EmitLicense(StringBuilder& sb)
{
    sb.Append(metaInterface_->license_).Append("\n");
}

void CppCodeEmitter::EmitHeadMacro(StringBuilder& sb, const String& fullName)
{
    String macroName = MacroName(fullName);
    sb.Append("#ifndef ").Append(macroName).Append("\n");
    sb.Append("#define ").Append(macroName).Append("\n");
}

void CppCodeEmitter::EmitTailMacro(StringBuilder& sb, const String& fullName)
{
    String macroName = MacroName(fullName);
    sb.Append("#endif // ").Append(macroName).Append("\n\n");
}

void CppCodeEmitter::EmitBeginNamespace(StringBuilder& sb)
{
    String nspace = GetNamespace(metaInterface_->namespace_);
    int index = nspace.IndexOf('.');
    while (index != -1) {
        sb.AppendFormat("namespace %s {\n", nspace.Substring(0, index).string());
        nspace = nspace.Substring(index + 1);
        index = nspace.IndexOf('.');
    }
}

void CppCodeEmitter::EmitEndNamespace(StringBuilder& sb)
{
    String nspace = GetNamespace(metaInterface_->namespace_);
    nspace = nspace.Substring(0, nspace.GetLength() - 1);
    while (!nspace.IsEmpty()) {
        int index = nspace.LastIndexOf('.');
        sb.AppendFormat("} // namespace %s\n", index != -1 ?
            nspace.Substring(index + 1, nspace.GetLength()).string() : nspace.string());
        nspace = nspace.Substring(0, index);
    }
}

void CppCodeEmitter::EmitWriteVariable(const String& parcelName, const std::string& name, MetaType* mt,
    StringBuilder& sb, const String& prefix)
{
    switch (mt->kind_) {
        case TypeKind::Boolean:
            sb.Append(prefix).AppendFormat("if (!%sWriteInt32(%s ? 1 : 0)) {\n", parcelName.string(), name.c_str());
            if (logOn_) {
                sb.Append(prefix).Append(TAB).AppendFormat("HiLog::Error(LABEL, \"Write [%s] failed!\");\n",
                    name.c_str());
            }
            sb.Append(prefix).Append(TAB).Append("return ERR_INVALID_DATA;\n");
            sb.Append(prefix).Append("}\n");
            break;
        case TypeKind::Char:
        case TypeKind::Byte:
        case TypeKind::Short:
        case TypeKind::Integer:
            sb.Append(prefix).AppendFormat("if (!%sWriteInt32(%s)) {\n", parcelName.string(), name.c_str());
            if (logOn_) {
                sb.Append(prefix).Append(TAB).AppendFormat("HiLog::Error(LABEL, \"Write [%s] failed!\");\n",
                    name.c_str());
            }
            sb.Append(prefix).Append(TAB).Append("return ERR_INVALID_DATA;\n");
            sb.Append(prefix).Append("}\n");
            break;
        case TypeKind::Long:
            sb.Append(prefix).AppendFormat("if (!%sWriteInt64(%s)) {\n", parcelName.string(), name.c_str());
            if (logOn_) {
                sb.Append(prefix).Append(TAB).AppendFormat("HiLog::Error(LABEL, \"Write [%s] failed!\");\n",
                    name.c_str());
            }
            sb.Append(prefix).Append(TAB).Append("return ERR_INVALID_DATA;\n");
            sb.Append(prefix).Append("}\n");
            break;
        default:
            EmitWriteVariableFloat(parcelName, name, mt, sb, prefix);
            break;
    }
}

void CppCodeEmitter::EmitWriteVariableFloat(
    const String& parcelName, const std::string& name, MetaType* mt, StringBuilder& sb, const String& prefix)
{
    switch (mt->kind_) {
        case TypeKind::Float:
            sb.Append(prefix).AppendFormat("if (!%sWriteFloat(%s)) {\n", parcelName.string(), name.c_str());
            if (logOn_) {
                sb.Append(prefix).Append(TAB).AppendFormat("HiLog::Error(LABEL, \"Write [%s] failed!\");\n",
                    name.c_str());
            }
            sb.Append(prefix).Append(TAB).Append("return ERR_INVALID_DATA;\n");
            sb.Append(prefix).Append("}\n");
            break;
        case TypeKind::Double:
            sb.Append(prefix).AppendFormat("if (!%sWriteDouble(%s)) {\n", parcelName.string(), name.c_str());
            if (logOn_) {
                sb.Append(prefix).Append(TAB).AppendFormat("HiLog::Error(LABEL, \"Write [%s] failed!\");\n",
                    name.c_str());
            }
            sb.Append(prefix).Append(TAB).Append("return ERR_INVALID_DATA;\n");
            sb.Append(prefix).Append("}\n");
            break;
        default:
            EmitWriteVariableComplex(parcelName, name, mt, sb, prefix);
            break;
    }
}

void CppCodeEmitter::EmitWriteVariableComplex(
    const String& parcelName, const std::string& name, MetaType* mt, StringBuilder& sb, const String& prefix)
{
    switch (mt->kind_) {
        case TypeKind::Array:
        case TypeKind::List: {
            sb.Append(prefix).AppendFormat("if (%s.size() > static_cast<size_t>(VECTOR_MAX_SIZE)) {\n", name.c_str());
            if (logOn_) {
                if (mt != nullptr && mt->kind_ == TypeKind::Array) {
                    sb.Append(prefix).Append(TAB).Append(
                        "HiLog::Error(LABEL, \"The vector/array size exceeds the security limit!\");\n");
                } else {
                    sb.Append(prefix).Append(TAB).AppendFormat(
                        "HiLog::Error(LABEL, \"The list size exceeds the security limit!\");\n");
                }
            }
            sb.Append(prefix).Append(TAB).Append("return ERR_INVALID_DATA;\n");
            sb.Append(prefix).Append("}\n");
            sb.Append(prefix).AppendFormat("%sWriteInt32(%s.size());\n", parcelName.string(), name.c_str());
            sb.Append(prefix).AppendFormat("for (auto it = %s.begin(); it != %s.end(); ++it) {\n",
                name.c_str(), name.c_str());
            if (mt != nullptr) {
                MetaType* innerType = metaComponent_->types_[mt->nestedTypeIndexes_[0]];
                EmitWriteVariable(parcelName, "(*it)", innerType, sb, prefix + TAB);
                sb.Append(prefix).Append("}\n");
            }
            break;
        }
        case TypeKind::Map: {
            sb.Append(prefix).AppendFormat("if (%s.size() > static_cast<size_t>(MAP_MAX_SIZE)) {\n", name.c_str());
            if (logOn_) {
                sb.Append(prefix).Append(TAB).AppendFormat(
                    "HiLog::Error(LABEL, \"The map size exceeds the security limit!\");\n");
            }
            sb.Append(prefix).Append(TAB).Append("return ERR_INVALID_DATA;\n");
            sb.Append(prefix).Append("}\n");
            sb.Append("\n");
            sb.Append(prefix).AppendFormat("%sWriteInt32(%s.size());\n", parcelName.string(), name.c_str());
            sb.Append(prefix).AppendFormat("for (auto it = %s.begin(); it != %s.end(); ++it) {\n",
                name.c_str(), name.c_str());
            MetaType* keyType = metaComponent_->types_[mt->nestedTypeIndexes_[0]];
            MetaType* valueType = metaComponent_->types_[mt->nestedTypeIndexes_[1]];
            EmitWriteVariable(parcelName, "(it->first)", keyType, sb, prefix + TAB);
            EmitWriteVariable(parcelName, "(it->second)", valueType, sb, prefix + TAB);
            sb.Append(prefix).Append("}\n");
            break;
        }
        default:
            EmitWriteVariableObject(parcelName, name, mt, sb, prefix);
            break;
    }
}

void CppCodeEmitter::EmitWriteVariableObject(
    const String& parcelName, const std::string& name, MetaType* mt, StringBuilder& sb, const String& prefix)
{
    switch (mt->kind_) {
        case TypeKind::String:
            sb.Append(prefix).AppendFormat("if (!%sWriteString16(Str8ToStr16(%s))) {\n", parcelName.string(),
                name.c_str());
            if (logOn_) {
                sb.Append(prefix).Append(TAB).AppendFormat("HiLog::Error(LABEL, \"Write [%s] failed!\");\n",
                    name.c_str());
            }
            sb.Append(prefix).Append(TAB).Append("return ERR_INVALID_DATA;\n");
            sb.Append(prefix).Append("}\n");
            break;
        case TypeKind::Sequenceable:
            sb.Append(prefix).AppendFormat("if (!%sWriteParcelable(&%s)) {\n", parcelName.string(), name.c_str());
            if (logOn_) {
                sb.Append(prefix).Append(TAB).AppendFormat("HiLog::Error(LABEL, \"Write [%s] failed!\");\n",
                    name.c_str());
            }
            sb.Append(prefix).Append(TAB).Append("return ERR_INVALID_DATA;\n");
            sb.Append(prefix).Append("}\n");
            break;
        case TypeKind::Interface:
            sb.Append(prefix).AppendFormat("if (%s == nullptr) {\n", name.c_str());
            if (logOn_) {
                sb.Append(prefix).Append(TAB).AppendFormat("HiLog::Error(LABEL, \"%s is nullptr!\");\n", name.c_str());
            }
            sb.Append(prefix).Append(TAB).Append("return ERR_INVALID_DATA;\n");
            sb.Append(prefix).Append("}\n");
            sb.Append(prefix).AppendFormat(
                "if (!%sWriteRemoteObject(%s->AsObject())) {\n", parcelName.string(), name.c_str());
            if (logOn_) {
                sb.Append(prefix).Append(TAB).AppendFormat("HiLog::Error(LABEL, \"Write [%s] failed!\");\n",
                    name.c_str());
            }
            sb.Append(prefix).Append(TAB).Append("return ERR_INVALID_DATA;\n");
            sb.Append(prefix).Append("}\n");
            break;
        default:
            break;
    }
}

void CppCodeEmitter::EmitReadVariable(const String& parcelName, const std::string& name, MetaType* mt,
    StringBuilder& sb, const String& prefix, bool emitType)
{
    switch (mt->kind_) {
        case TypeKind::Boolean:
            if (emitType) {
                sb.Append(prefix).AppendFormat("%s %s = %sReadInt32() == 1 ? true : false;\n",
                    EmitType(mt, ATTR_IN, true).string(), name.c_str(), parcelName.string());
            } else {
                sb.Append(prefix).AppendFormat("%s = %sReadInt32() == 1 ? true : false;\n",
                    name.c_str(), parcelName.string());
            }
            break;
        case TypeKind::Char:
        case TypeKind::Byte:
        case TypeKind::Short:
            if (emitType) {
                sb.Append(prefix).AppendFormat("%s %s = (%s)%sReadInt32();\n", EmitType(mt, ATTR_IN, true).string(),
                    name.c_str(), EmitType(mt, ATTR_IN, true).string(), parcelName.string());
            } else {
                sb.Append(prefix).AppendFormat("%s = (%s)%sReadInt32();\n", name.c_str(),
                    EmitType(mt, ATTR_IN, true).string(), parcelName.string());
            }
            break;
        case TypeKind::Integer:
            if (emitType) {
                sb.Append(prefix).AppendFormat("%s %s = %sReadInt32();\n",
                    EmitType(mt, ATTR_IN, true).string(), name.c_str(), parcelName.string());
            } else {
                sb.Append(prefix).AppendFormat("%s = %sReadInt32();\n", name.c_str(), parcelName.string());
            }
            break;
        case TypeKind::Long:
            if (emitType) {
                sb.Append(prefix).AppendFormat("%s %s = %sReadInt64();\n",
                    EmitType(mt, ATTR_IN, true).string(), name.c_str(), parcelName.string());
            } else {
                sb.Append(prefix).AppendFormat("%s = %sReadInt64();\n", name.c_str(), parcelName.string());
            }
            break;
        default:
            EmitReadVariableFloat(parcelName, name, mt, sb, prefix, emitType);
            break;
    }
}

void CppCodeEmitter::EmitReadVariableFloat(const String& parcelName, const std::string& name, MetaType* mt,
    StringBuilder& sb, const String& prefix, bool emitType)
{
    switch (mt->kind_) {
        case TypeKind::Float:
            if (emitType) {
                sb.Append(prefix).AppendFormat("%s %s = %sReadFloat();\n",
                    EmitType(mt, ATTR_IN, true).string(), name.c_str(), parcelName.string());
            } else {
                sb.Append(prefix).AppendFormat("%s = %sReadFloat();\n", name.c_str(), parcelName.string());
            }
            break;
        case TypeKind::Double:
            if (emitType) {
                sb.Append(prefix).AppendFormat("%s %s = %sReadDouble();\n",
                    EmitType(mt, ATTR_IN, true).string(), name.c_str(), parcelName.string());
            } else {
                sb.Append(prefix).AppendFormat("%s = %sReadDouble();\n", name.c_str(), parcelName.string());
            }
            break;
        default:
            EmitReadVariableComplex(parcelName, name, mt, sb, prefix, emitType);
            break;
    }
}

void CppCodeEmitter::EmitReadVariableComplex(const String& parcelName, const std::string& name, MetaType* mt,
    StringBuilder& sb, const String& prefix, bool emitType)
{
    switch (mt->kind_) {
        case TypeKind::String:
            if (emitType) {
                sb.Append(prefix).AppendFormat("%s %s = Str16ToStr8(%sReadString16());\n",
                    EmitType(mt, ATTR_IN, true).string(), name.c_str(), parcelName.string());
            } else {
                sb.Append(prefix).AppendFormat("%s = Str16ToStr8(%sReadString16());\n",
                    name.c_str(), parcelName.string());
            }
            break;
        case TypeKind::Map: {
            if (emitType) {
                sb.Append(prefix).AppendFormat("%s %s;\n", EmitType(mt, ATTR_IN, true).string(), name.c_str());
            }
            sb.Append(prefix).AppendFormat("int32_t %sSize = %sReadInt32();\n", name.c_str(), parcelName.string());
            sb.Append(prefix).AppendFormat("for (int32_t i = 0; i < %sSize; ++i) {\n", name.c_str());
            MetaType* keyType = metaComponent_->types_[mt->nestedTypeIndexes_[0]];
            MetaType* valueType = metaComponent_->types_[mt->nestedTypeIndexes_[1]];
            EmitReadVariable(parcelName, "key", keyType, sb, prefix + TAB);
            EmitReadVariable(parcelName, "value", valueType, sb, prefix + TAB);
            sb.Append(prefix + TAB).AppendFormat("%s[key] = value;\n", name.c_str());
            sb.Append(prefix).Append("}\n");
            break;
        }
        default:
            EmitReadVariableList(parcelName, name, mt, sb, prefix, emitType);
            break;
    }
}

void CppCodeEmitter::EmitReadVariableList(const String& parcelName, const std::string& name, MetaType* mt,
    StringBuilder& sb, const String& prefix, bool emitType)
{
    switch (mt->kind_) {
        case TypeKind::Array:
        case TypeKind::List: {
            if (emitType) {
                sb.Append(prefix).AppendFormat("%s %s;\n", EmitType(mt, ATTR_IN, true).string(), name.c_str());
            }
            sb.Append(prefix).AppendFormat("int32_t %sSize = %sReadInt32();\n", name.c_str(), parcelName.string());
            sb.Append(prefix).AppendFormat("if (%sSize > static_cast<int32_t>(VECTOR_MAX_SIZE)) {\n", name.c_str());
            if (logOn_) {
                sb.Append(prefix + TAB)
                    .Append("HiLog::Error(LABEL, \"The vector/array size exceeds the security limit!\");\n");
            }
            sb.Append(prefix + TAB).Append("return ERR_INVALID_DATA;\n");
            sb.Append(prefix).Append("}\n");
            circleCount++;
            std::stringstream circleCountStr;
            circleCountStr << circleCount;
            std::string iStr = "i" + circleCountStr.str();
            std::string valueStr = "value" + circleCountStr.str();
            sb.Append(prefix).AppendFormat("for (int32_t %s = 0; %s < %sSize; ++%s) {\n",
                iStr.c_str(), iStr.c_str(), name.c_str(), iStr.c_str());
            MetaType* innerType = metaComponent_->types_[mt->nestedTypeIndexes_[0]];
            EmitReadVariable(parcelName, valueStr.c_str(), innerType, sb, prefix + TAB);
            if (innerType->kind_ == TypeKind::Sequenceable) {
                sb.Append(prefix + TAB).AppendFormat("%s.push_back(*%s);\n", name.c_str(), valueStr.c_str());
            } else {
                sb.Append(prefix + TAB).AppendFormat("%s.push_back(%s);\n", name.c_str(), valueStr.c_str());
            }
            sb.Append(prefix).Append("}\n");
            break;
        }
        default:
            EmitReadVariableObject(parcelName, name, mt, sb, prefix, emitType);
            break;
    }
}

void CppCodeEmitter::EmitReadVariableObject(const String& parcelName, const std::string& name, MetaType* mt,
    StringBuilder& sb, const String& prefix, bool emitType)
{
    switch (mt->kind_) {
        case TypeKind::Sequenceable: {
            MetaSequenceable* mp = metaComponent_->sequenceables_[mt->index_];
            if (mp == nullptr) {
                break;
            }
            if (emitType) {
                readSequenceable_ = true;
                sb.Append(prefix).AppendFormat("std::unique_ptr<%s> %s(%sReadParcelable<%s>());\n\n",
                    EmitType(mt, ATTR_IN, true).string(), name.c_str(), parcelName.string(), mp->name_);
                sb.Append(prefix).AppendFormat("if (!%s) {\n", name.c_str());
                if (logOn_) {
                    sb.Append(prefix).Append(TAB).AppendFormat(
                        "HiLog::Error(LABEL, \"Read [%s] failed!\");\n", mp->name_);
                }
                sb.Append(prefix).Append(TAB).Append("return ERR_INVALID_DATA;\n");
                sb.Append(prefix).Append("}\n");
            } else {
                sb.Append(prefix).AppendFormat("std::unique_ptr<%s> info(%sReadParcelable<%s>());\n",
                    mp->name_, parcelName.string(), mp->name_);
                sb.Append(prefix).Append("if (info != nullptr) {\n");
                sb.Append(prefix).Append(TAB).AppendFormat("%s = *info;\n", name.c_str());
                sb.Append(prefix).Append("}\n\n");
            }
            break;
        }
        case TypeKind::Interface: {
            MetaInterface* mi = metaComponent_->interfaces_[mt->index_];
            if (emitType) {
                sb.Append(prefix).AppendFormat("%s %s = iface_cast<%s>(%sReadRemoteObject());\n",
                    EmitType(mt, ATTR_IN, true).string(), name.c_str(), mi->name_, parcelName.string());
                sb.Append(prefix).AppendFormat("if (%s == nullptr) {\n", name.c_str());
                if (logOn_) {
                    sb.Append(prefix).Append(TAB).AppendFormat("HiLog::Error(LABEL, \"Read [%s] failed!\");\n",
                        name.c_str());
                }
                sb.Append(prefix).Append(TAB).AppendFormat("return ERR_INVALID_DATA;\n");
                sb.Append(prefix).Append("}\n\n");
            } else {
                sb.Append(prefix).AppendFormat("%s = iface_cast<%s>(%sReadRemoteObject());\n",
                    name.c_str(),  mi->name_, parcelName.string());
            }
            break;
        }
        default:
            break;
    }
}

void CppCodeEmitter::EmitLocalVariable(MetaParameter* mp, StringBuilder& sb, const String& prefix)
{
    MetaType* mt = metaComponent_->types_[mp->typeIndex_];
    const std::string name = mp->name_;
    if ((mt->kind_ == TypeKind::Sequenceable) || (mt->kind_ == TypeKind::Interface)) {
        createSequenceableForOut_ = true;
        mp->isSequenceableForOut_ = true;
        sb.Append(prefix).AppendFormat("%s %s;\n", EmitType(mt, ATTR_IN, true).string(), name.c_str());
    } else {
        sb.Append(prefix).AppendFormat("%s %s;\n", EmitType(mt, ATTR_IN, true).string(), name.c_str());
    }
}

void CppCodeEmitter::EmitReturnParameter(const String& name, MetaType* mt, StringBuilder& sb)
{
    switch (mt->kind_) {
        case TypeKind::Char:
        case TypeKind::Boolean:
        case TypeKind::Byte:
        case TypeKind::Short:
        case TypeKind::Integer:
        case TypeKind::Long:
        case TypeKind::Float:
        case TypeKind::Double:
        case TypeKind::String:
        case TypeKind::Sequenceable:
        case TypeKind::Interface:
        case TypeKind::List:
        case TypeKind::Map:
        case TypeKind::Array:
            sb.Append(name);
            break;
        default:
            break;
    }
}

String CppCodeEmitter::EmitType(MetaType* mt, unsigned int attributes, bool isInnerType)
{
    switch (mt->kind_) {
        case TypeKind::Char:
            enteredVector_ = false;
            if (attributes & ATTR_IN) {
                return "zchar";
            } else {
                return "zchar&";
            }
        case TypeKind::Boolean:
            enteredVector_ = false;
            if (attributes & ATTR_IN) {
                return "bool";
            } else {
                return "bool&";
            }
        case TypeKind::Byte:
            enteredVector_ = false;
            if (attributes & ATTR_IN) {
                return "int8_t";
            } else {
                return "int8_t&";
            }
        case TypeKind::Short:
            enteredVector_ = false;
            if (attributes & ATTR_IN) {
                return "short";
            } else {
                return "short&";
            }
        case TypeKind::Integer:
            enteredVector_ = false;
            if (attributes & ATTR_IN) {
                return "int32_t";
            } else {
                return "int32_t&";
            }
        case TypeKind::Long:
            enteredVector_ = false;
            if (attributes & ATTR_IN) {
                return "long";
            } else {
                return "long&";
            }
        default:
            return EmitFloatType(mt, attributes, isInnerType);
    }
}


String CppCodeEmitter::EmitFloatType(MetaType* mt, unsigned int attributes, bool isInnerType)
{
    switch (mt->kind_) {
        case TypeKind::Float:
            enteredVector_ = false;
            if (attributes & ATTR_IN) {
                return "float";
            } else {
                return "float&";
            }
        case TypeKind::Double:
            enteredVector_ = false;
            if (attributes & ATTR_IN) {
                return "double";
            } else {
                return "double&";
            }
        case TypeKind::Void:
            enteredVector_ = false;
            return "void";
        default:
            return EmitComplexType(mt, attributes, isInnerType);
    }
}

String CppCodeEmitter::EmitComplexType(MetaType* mt, unsigned int attributes, bool isInnerType)
{
    switch (mt->kind_) {
        case TypeKind::String:
            enteredVector_ = false;
            if (attributes & ATTR_IN) {
                if (!isInnerType) {
                    return "const std::string&";
                } else {
                    return "std::string";
                }
            } else {
                return "std::string&";
            }
        case TypeKind::Map: {
            MetaType* keyType = metaComponent_->types_[mt->nestedTypeIndexes_[0]];
            MetaType* valueType = metaComponent_->types_[mt->nestedTypeIndexes_[1]];
            if (attributes & ATTR_OUT) {
                return String::Format("std::unordered_map<%s, %s>&",
                    EmitType(keyType, ATTR_IN, true).string(), EmitType(valueType, ATTR_IN, true).string());
            } else {
                if (!isInnerType) {
                    return String::Format("const std::unordered_map<%s, %s>&",
                        EmitType(keyType, ATTR_IN, true).string(), EmitType(valueType, ATTR_IN, true).string());
                } else {
                    return String::Format("std::unordered_map<%s, %s>",
                        EmitType(keyType, ATTR_IN, true).string(), EmitType(valueType, ATTR_IN, true).string());
                }
            }
            break;
        }
        default:
            return EmitListType(mt, attributes, isInnerType);
    }
}

String CppCodeEmitter::EmitListType(MetaType* mt, unsigned int attributes, bool isInnerType)
{
    switch (mt->kind_) {
        case TypeKind::Array:
        case TypeKind::List: {
            enteredVector_ = true;
            MetaType* elementType = metaComponent_->types_[mt->nestedTypeIndexes_[0]];
            if (attributes & ATTR_OUT) {
                return String::Format("std::vector<%s>&",
                                      EmitType(elementType, ATTR_IN, true).string());
            } else {
                if (!isInnerType) {
                    return String::Format("const std::vector<%s>&",
                                          EmitType(elementType, ATTR_IN, true).string());
                } else {
                    return String::Format("std::vector<%s>",
                                          EmitType(elementType, ATTR_IN, true).string());
                }
            }
        }
        default:
            return EmitObjectType(mt, attributes, isInnerType);
    }
}

String CppCodeEmitter::EmitObjectType(MetaType* mt, unsigned int attributes, bool isInnerType)
{
    switch (mt->kind_) {
        case TypeKind::Sequenceable: {
            MetaSequenceable* mp = metaComponent_->sequenceables_[mt->index_];
            if (mp == nullptr) {
                return "unknown type";
            }
            if (enteredVector_) {
                enteredVector_ = false;
                return String::Format("%s", mp->name_);
            }
            if (readSequenceable_) {
                readSequenceable_ = false;
                return String::Format("%s", mp->name_);
            }
            if (createSequenceableForOut_) {
                createSequenceableForOut_ = false;
                return String::Format("%s", mp->name_);
            }
            if ((attributes & ATTR_MASK) == (ATTR_IN | ATTR_OUT)) {
                return String::Format("%s*", mp->name_);
            } else if (attributes & ATTR_IN) {
                return String::Format("const %s&", mp->name_);
            } else {
                return String::Format("%s&", mp->name_);
            }
        }
        case TypeKind::Interface: {
            MetaInterface* mi = metaComponent_->interfaces_[mt->index_];
            if (mi == nullptr) {
                return "unknown type";
            }
            if (attributes & ATTR_IN) {
                if (!isInnerType) {
                    return String::Format("const sptr<%s>&", mi->name_);
                } else {
                    return String::Format("sptr<%s>", mi->name_);
                }
            } else {
                return String::Format("sptr<%s>&", mi->name_);
            }
        }
        default:
            return "unknown type";
    }
}

String CppCodeEmitter::CppFullName(const String& name)
{
    if (name.IsEmpty()) {
        return name;
    }

    return name.Replace(".", "::");
}

String CppCodeEmitter::FileName(const String& name)
{
    if (name.IsEmpty()) {
        return name;
    }

    StringBuilder sb;

    for (int i = 0; i < name.GetLength(); i++) {
        char c = name[i];
        if (isupper(c) != 0) {
            // 2->Index of the last char array.
            if (i > 1 && name[i - 1] != '.' && name[i - 2] != '.') {
                sb.Append('_');
            }
            sb.Append(tolower(c));
        } else {
            sb.Append(c);
        }
    }

    return sb.ToString().Replace('.', '/');
}

String CppCodeEmitter::MacroName(const String& name)
{
    if (name.IsEmpty()) {
        return name;
    }

    String macro = name.Replace('.', '_').ToUpperCase() + "_H";
    return macro;
}

String CppCodeEmitter::ConstantName(const String& name)
{
    if (name.IsEmpty()) {
        return name;
    }

    StringBuilder sb;

    for (int i = 0; i < name.GetLength(); i++) {
        char c = name[i];
        if (isupper(c) != 0) {
            if (i > 1) {
                sb.Append('_');
            }
            sb.Append(c);
        } else {
            sb.Append(toupper(c));
        }
    }

    return sb.ToString();
}

const std::string CppCodeEmitter::UnderlineAdded(const String& originName)
{
    std::string underline("_");
    return underline + std::string(originName.string());
}
} // namespace Idl
} // namespace OHOS