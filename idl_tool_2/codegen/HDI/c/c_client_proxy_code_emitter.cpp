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

#include "c_client_proxy_code_emitter.h"
#include "util/file.h"
#include "util/logger.h"

namespace OHOS {
namespace Idl {
bool CClientProxyCodeEmitter::ResolveDirectory(const std::string &targetDirectory)
{
    if (ast_->GetASTFileType() == ASTFileType::AST_IFACE || ast_->GetASTFileType() == ASTFileType::AST_ICALLBACK) {
        directory_ = GetFileParentPath(targetDirectory);
    } else {
        return false;
    }

    if (!File::CreateParentDir(directory_)) {
        Logger::E("CClientProxyCodeEmitter", "Create '%s' failed!", directory_.c_str());
        return false;
    }

    return true;
}

void CClientProxyCodeEmitter::EmitCode()
{
    switch (mode_) {
        case GenMode::PASSTHROUGH: {
            if (!interface_->IsSerializable()) {
                EmitPassthroughProxySourceFile();
            }
            break;
        }
        case GenMode::IPC:
        case GenMode::KERNEL: {
            EmitProxySourceFile();
            break;
        }
        default:
            break;
    }
}

void CClientProxyCodeEmitter::EmitPassthroughProxySourceFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.c", directory_.c_str(), FileName(proxyName_).c_str()));
    File file(filePath, File::WRITE);
    StringBuilder sb;

    EmitLicense(sb);
    EmitPassthroughProxyInclusions(sb);
    sb.Append("\n");
    EmitLogTagMacro(sb, FileName(proxyName_));
    sb.Append("\n");
    EmitProxyGetMethodImpl(sb);
    sb.Append("\n");
    EmitPassthroughGetInstanceMethod(sb);
    sb.Append("\n");
    EmitProxyReleaseMethodImpl(sb);
    sb.Append("\n");
    EmitPassthroughReleaseInstanceMethod(sb);
    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CClientProxyCodeEmitter::EmitPassthroughProxyInclusions(StringBuilder &sb)
{
    HeaderFile::HeaderFileSet headerFiles;
    headerFiles.emplace(HeaderFileType::OWN_MODULE_HEADER_FILE, EmitVersionHeaderName(interfaceName_));
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdi_support");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_core_log");

    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CClientProxyCodeEmitter::EmitPassthroughGetInstanceMethod(StringBuilder &sb) const
{
    sb.AppendFormat("struct %s *%sGetInstance(const char *serviceName, bool isStub)\n",
        interfaceName_.c_str(), interfaceName_.c_str());
    sb.Append("{\n");
    EmitProxyLoadOrUnLoadHdiImpl("serviceName", true, sb, TAB);
    sb.Append(TAB).Append("return NULL;\n");
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitPassthroughReleaseInstanceMethod(StringBuilder &sb) const
{
    sb.AppendFormat("void %sReleaseInstance(const char *serviceName, struct %s *instance, bool isStub)\n",
        interfaceName_.c_str(), interfaceName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("if (instance == NULL) {\n");
    sb.Append(TAB).Append(TAB).Append("return;\n");
    sb.Append(TAB).Append("}\n\n");
    EmitProxyLoadOrUnLoadHdiImpl("serviceName", false, sb, TAB);
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxySourceFile()
{
    std::string filePath =
        File::AdapterPath(StringHelper::Format("%s/%s.c", directory_.c_str(), FileName(proxyName_).c_str()));
    File file(filePath, File::WRITE);
    StringBuilder sb;

    EmitLicense(sb);
    EmitProxyInclusions(sb);
    sb.Append("\n");
    EmitLogTagMacro(sb, FileName(proxyName_));
    sb.Append("\n");
    EmitProxyDefinition(sb);
    sb.Append("\n");
    EmitUtilMethods(sb, true);
    sb.Append("\n");
    EmitUtilMethods(sb, false);
    sb.Append("\n");
    if (mode_ != GenMode::KERNEL) {
        EmitProxyCallMethodImpl(sb);
    } else {
        EmitProxyKernelCallMethodImpl(sb);
    }
    sb.Append("\n");
    EmitProxyMethodImpls(sb);
    sb.Append("\n");
    EmitProxyConstruction(sb);
    sb.Append("\n");
    EmitProxyExternalMethodImpl(sb);

    std::string data = sb.ToString();
    file.WriteData(data.c_str(), data.size());
    file.Flush();
    file.Close();
}

void CClientProxyCodeEmitter::EmitProxyInclusions(StringBuilder &sb)
{
    HeaderFile::HeaderFileSet headerFiles;

    headerFiles.emplace(HeaderFileType::OWN_MODULE_HEADER_FILE, EmitVersionHeaderName(interfaceName_));
    GetHeaderOtherLibInclusions(headerFiles);

    for (const auto &file : headerFiles) {
        sb.AppendFormat("%s\n", file.ToString().c_str());
    }
}

void CClientProxyCodeEmitter::GetHeaderOtherLibInclusions(HeaderFile::HeaderFileSet &headerFiles) const
{
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_base");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_core_log");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_sbuf");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "osal_mem");
    headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "securec");

    if (mode_ == GenMode::KERNEL) {
        headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_io_service_if");
    } else {
        headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "servmgr_hdi");
        headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdf_dlist");
        if (!interface_->IsSerializable()) {
            headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "hdi_support");
            headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "stub_collector");
        }
    }

    for (const auto &method : interface_->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
        for (size_t paramIndex = 0; paramIndex < method->GetParameterNumber(); paramIndex++) {
            AutoPtr<ASTParameter> param = method->GetParameter(paramIndex);
            AutoPtr<ASTType> paramType = param->GetType();
            if ((param->GetAttribute() == ASTParamAttr::PARAM_IN) &&
                (paramType->IsInterfaceType() || paramType->HasInnerType(TypeKind::TYPE_INTERFACE))) {
                headerFiles.emplace(HeaderFileType::OTHER_MODULES_HEADER_FILE, "stub_collector");
            }
        }
    }
}

void CClientProxyCodeEmitter::EmitProxyDefinition(StringBuilder &sb) const
{
    sb.AppendFormat("struct %sProxy {\n", baseName_.c_str());
    sb.Append(TAB).AppendFormat("struct %s impl;\n", interfaceName_.c_str());
    if (mode_ == GenMode::KERNEL) {
        sb.Append(TAB).Append("struct HdfIoService *serv;\n");
    } else {
        sb.Append(TAB).Append("struct HdfRemoteService *remote;\n");
    }

    sb.Append("};\n");
}

void CClientProxyCodeEmitter::EmitProxyCallMethodImpl(StringBuilder &sb) const
{
    sb.AppendFormat("static int32_t %sProxyCall(struct %s *self, int32_t id, struct HdfSBuf *data,\n",
        baseName_.c_str(), interfaceName_.c_str());
    sb.Append(TAB).Append("struct HdfSBuf *reply, bool isOneWay)\n");
    sb.Append("{\n");

    std::string remoteName = "remote";
    sb.Append(TAB).AppendFormat("struct HdfRemoteService *%s = self->AsObject(self);\n", remoteName.c_str());
    sb.Append(TAB).AppendFormat("if (%s == NULL ||\n", remoteName.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("%s->dispatcher == NULL ||\n", remoteName.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("%s->dispatcher->Dispatch == NULL ||\n", remoteName.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("%s->dispatcher->DispatchAsync == NULL) {\n", remoteName.c_str());
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%{public}s: Invalid HdfRemoteService obj\", __func__);\n");
    sb.Append(TAB).Append(TAB).Append("return HDF_ERR_INVALID_OBJECT;\n");
    sb.Append(TAB).Append("}\n");

    sb.Append(TAB).AppendFormat("if (isOneWay) {\n");
    sb.Append(TAB).Append(TAB).AppendFormat(
        "return %s->dispatcher->DispatchAsync(%s, id, data, reply);\n", remoteName.c_str(), remoteName.c_str());
    sb.Append(TAB).AppendFormat("} else {\n");
    sb.Append(TAB).Append(TAB).AppendFormat(
        "return %s->dispatcher->Dispatch(%s, id, data, reply);\n", remoteName.c_str(), remoteName.c_str());
    sb.Append(TAB).AppendFormat("}\n");
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxyKernelCallMethodImpl(StringBuilder &sb) const
{
    sb.AppendFormat("static int32_t %sProxyCall(struct %s *self, int32_t id, struct HdfSBuf *data,\n",
        baseName_.c_str(), interfaceName_.c_str());
    sb.Append(TAB).Append("struct HdfSBuf *reply)\n");
    sb.Append("{\n");

    std::string remoteName = "serv";
    sb.Append(TAB).AppendFormat("struct %sProxy *proxy = CONTAINER_OF(self, struct %sProxy, impl);\n",
        baseName_.c_str(), baseName_.c_str(), remoteName.c_str());
    sb.Append(TAB).AppendFormat("struct HdfIoService *%s = proxy->%s;\n", remoteName.c_str(), remoteName.c_str());

    sb.Append(TAB).AppendFormat("if (%s == NULL ||\n", remoteName.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("%s->dispatcher == NULL ||\n", remoteName.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("%s->dispatcher->Dispatch == NULL) {\n", remoteName.c_str());
    sb.Append(TAB).Append(TAB).Append("HDF_LOGE(\"%{public}s: Invalid HdfRemoteService obj\", __func__);\n");
    sb.Append(TAB).Append(TAB).Append("return HDF_ERR_INVALID_OBJECT;\n");
    sb.Append(TAB).Append("}\n\n");

    sb.Append(TAB).AppendFormat("return %s->dispatcher->Dispatch(", remoteName.c_str());
    sb.AppendFormat("(struct HdfObject *)&(%s->object), id, data, reply);\n", remoteName.c_str());
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxyMethodImpls(StringBuilder &sb)
{
    for (const auto &method : interface_->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
        EmitProxyMethodImpl(method, sb);
        sb.Append("\n");
    }

    EmitProxyMethodImpl(interface_->GetVersionMethod(), sb);

    if (mode_ != GenMode::KERNEL) {
        sb.Append("\n");
        EmitProxyAsObjectMethodImpl(sb);
    }
}

void CClientProxyCodeEmitter::EmitProxyMethodImpl(const AutoPtr<ASTMethod> &method, StringBuilder &sb)
{
    if (method->GetParameterNumber() == 0) {
        sb.AppendFormat("static int32_t %sProxy%s(struct %s *self)\n", baseName_.c_str(), method->GetName().c_str(),
            interfaceName_.c_str());
    } else {
        StringBuilder paramStr;
        paramStr.AppendFormat("static int32_t %sProxy%s(", baseName_.c_str(), method->GetName().c_str());
        paramStr.AppendFormat("struct %s *self, ", interfaceName_.c_str());
        for (size_t i = 0; i < method->GetParameterNumber(); i++) {
            AutoPtr<ASTParameter> param = method->GetParameter(i);
            EmitInterfaceMethodParameter(param, paramStr, "");
            if (i + 1 < method->GetParameterNumber()) {
                paramStr.Append(", ");
            }
        }

        paramStr.Append(")");
        sb.Append(SpecificationParam(paramStr, TAB));
        sb.Append("\n");
    }
    EmitProxyMethodBody(method, sb, "");
}

void CClientProxyCodeEmitter::EmitProxyMethodBody(
    const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix)
{
    sb.Append(prefix).Append("{\n");
    sb.Append(prefix + TAB).AppendFormat("int32_t %s = HDF_FAILURE;\n", HdiTypeEmitter::errorCodeName_.c_str());

    // Local variable definitions must precede all execution statements.
    EmitMethodNeedLoopVar(method, true, false, sb, prefix + TAB);
    sb.Append("\n");
    EmitCreateBuf(HdiTypeEmitter::dataParcelName_, HdiTypeEmitter::replyParcelName_, sb, prefix + TAB);
    sb.Append("\n");
    EmitCheckThisPointer(sb, prefix + TAB);

    if (mode_ != GenMode::KERNEL) {
        sb.Append("\n");
        EmitWriteInterfaceToken(HdiTypeEmitter::dataParcelName_, sb, prefix + TAB);
    }

    sb.Append("\n");
    EmitWriteFlagOfNeedSetMem(method, HdiTypeEmitter::dataParcelName_, sb, prefix + TAB);
    for (size_t i = 0; i < method->GetParameterNumber(); i++) {
        AutoPtr<ASTParameter> param = method->GetParameter(i);
        AutoPtr<HdiTypeEmitter> typeEmitter = GetTypeEmitter(param->GetType());
        if (typeEmitter == nullptr) {
            continue;
        }
        if (param->GetAttribute() == ASTParamAttr::PARAM_IN) {
            typeEmitter->EmitCWriteVar(TypeMode::PARAM_IN, param->GetName(), FINISHED_LABEL, sb, prefix + TAB);
            sb.Append("\n");
        } else {
            typeEmitter->EmitCProxyWriteOutVar(param->GetName(), FINISHED_LABEL, sb, prefix + TAB);
        }
    }

    EmitStubCallMethod(method, sb, prefix + TAB);
    sb.Append("\n");

    if (!method->IsOneWay()) {
        for (size_t i = 0; i < method->GetParameterNumber(); i++) {
            AutoPtr<ASTParameter> param = method->GetParameter(i);
            if (param->GetAttribute() == ASTParamAttr::PARAM_OUT) {
                EmitReadProxyMethodParameter(param, FINISHED_LABEL, sb, prefix + TAB);
                sb.Append("\n");
            }
        }
    }

    sb.Append(prefix).AppendFormat("%s:\n", FINISHED_LABEL);
    EmitReleaseBuf(HdiTypeEmitter::dataParcelName_, HdiTypeEmitter::replyParcelName_, sb, prefix + TAB);

    sb.Append(prefix + TAB).AppendFormat("return %s;\n", HdiTypeEmitter::errorCodeName_.c_str());
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitCreateBuf(const std::string &dataBufName,
    const std::string &replyBufName, StringBuilder &sb, const std::string &prefix) const
{
    if (mode_ == GenMode::KERNEL) {
        sb.Append(prefix).AppendFormat("struct HdfSBuf *%s = HdfSbufObtainDefaultSize();\n", dataBufName.c_str());
        sb.Append(prefix).AppendFormat("struct HdfSBuf *%s = HdfSbufObtainDefaultSize();\n", replyBufName.c_str());
    } else {
        sb.Append(prefix).AppendFormat("struct HdfSBuf *%s = HdfSbufTypedObtain(SBUF_IPC);\n", dataBufName.c_str());
        sb.Append(prefix).AppendFormat("struct HdfSBuf *%s = HdfSbufTypedObtain(SBUF_IPC);\n", replyBufName.c_str());
    }

    sb.Append("\n");
    sb.Append(prefix).AppendFormat("if (%s == NULL || %s == NULL) {\n", dataBufName.c_str(), replyBufName.c_str());
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: HdfSubf malloc failed!\", __func__);\n");
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_MALLOC_FAIL;\n", HdiTypeEmitter::errorCodeName_.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", FINISHED_LABEL);
    sb.Append(prefix).Append("}\n");
}

void CClientProxyCodeEmitter::EmitCheckThisPointer(StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).Append("if (self == NULL) {\n");
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: invalid interface object\", __func__);\n");
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_OBJECT;\n", HdiTypeEmitter::errorCodeName_.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", FINISHED_LABEL);
    sb.Append(prefix).Append("}\n");
}

void CClientProxyCodeEmitter::EmitWriteInterfaceToken(
    const std::string &dataBufName, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat(
        "if (!HdfRemoteServiceWriteInterfaceToken(self->AsObject(self), %s)) {\n", dataBufName.c_str());
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: write interface token failed!\", __func__);\n");
    sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", HdiTypeEmitter::errorCodeName_.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", FINISHED_LABEL);
    sb.Append(prefix).Append("}\n");
}

void CClientProxyCodeEmitter::EmitWriteFlagOfNeedSetMem(const AutoPtr<ASTMethod> &method,
    const std::string &dataBufName, StringBuilder &sb, const std::string &prefix) const
{
    if (NeedFlag(method)) {
        sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUint8(%s, 1)) {\n", dataBufName.c_str());
        sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: write flag of memory setting failed!\", __func__);\n");
        sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", HdiTypeEmitter::errorCodeName_.c_str());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", FINISHED_LABEL);
        sb.Append(prefix).Append("}\n\n");
    }
}

void CClientProxyCodeEmitter::EmitReleaseBuf(const std::string &dataBufName,
    const std::string &replyBufName, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (%s != NULL) {\n", dataBufName.c_str());
    sb.Append(prefix + TAB).AppendFormat("HdfSbufRecycle(%s);\n", dataBufName.c_str());
    sb.Append(prefix).Append("}\n");
    sb.Append(prefix).AppendFormat("if (%s != NULL) {\n", replyBufName.c_str());
    sb.Append(prefix + TAB).AppendFormat("HdfSbufRecycle(%s);\n", replyBufName.c_str());
    sb.Append(prefix).Append("}\n");
}

void CClientProxyCodeEmitter::EmitReadProxyMethodParameter(const AutoPtr<ASTParameter> &param,
    const std::string &gotoLabel, StringBuilder &sb, const std::string &prefix) const
{
    AutoPtr<ASTType> type = param->GetType();
    AutoPtr<HdiTypeEmitter> typeEmitter = GetTypeEmitter(type);
    if (typeEmitter == nullptr) {
        return;
    }
    std::string name = param->GetName();
    if (type->GetTypeKind() == TypeKind::TYPE_STRING) {
        std::string cloneName = StringHelper::Format("%sCopy", name.c_str());
        typeEmitter->EmitCProxyReadVar(cloneName, false, gotoLabel, sb, prefix);
        sb.Append(prefix).AppendFormat("if (strcpy_s(%s, %sLen, %s) != EOK) {\n", name.c_str(),
            name.c_str(), cloneName.c_str());
        sb.Append(prefix + TAB)
            .AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.c_str());
        sb.Append(prefix + TAB).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", HdiTypeEmitter::errorCodeName_.c_str());
        sb.Append(prefix + TAB).AppendFormat("goto %s;\n", gotoLabel.c_str());
        sb.Append(prefix).Append("}\n");
    } else if (type->GetTypeKind() == TypeKind::TYPE_UNION) {
        std::string cpName = StringHelper::Format("%sCp", name.c_str());
        typeEmitter->EmitCProxyReadVar(cpName, false, gotoLabel, sb, prefix);
        sb.Append(prefix).AppendFormat("if (memcpy_s(%s, sizeof(%s), %s, sizeof(%s)) != EOK) {\n",
            name.c_str(), typeEmitter->EmitCType().c_str(), cpName.c_str(), typeEmitter->EmitCType().c_str());
        sb.Append(prefix + TAB).AppendFormat("HDF_LOGE(\"%%{public}s: failed to memcpy %s\", __func__);\n",
            name.c_str());
        sb.Append(prefix + TAB).Append("return HDF_ERR_INVALID_PARAM;\n");
        sb.Append(prefix).Append("}\n");
    } else {
        typeEmitter->EmitCProxyReadVar(name, false, gotoLabel, sb, prefix);
    }
}

void CClientProxyCodeEmitter::EmitStubCallMethod(
    const AutoPtr<ASTMethod> &method, StringBuilder &sb, const std::string &prefix)
{
    if (mode_ != GenMode::KERNEL) {
        sb.Append(prefix).AppendFormat("%s = %sCall(self, %s, %s, %s, %s);\n", HdiTypeEmitter::errorCodeName_.c_str(),
            proxyName_.c_str(), EmitMethodCmdID(method).c_str(), HdiTypeEmitter::dataParcelName_.c_str(),
            HdiTypeEmitter::replyParcelName_.c_str(),
            method->IsOneWay() ? "true" : "false");
    } else {
        sb.Append(prefix).AppendFormat("%s = %sCall(self, %s, %s, %s);\n", HdiTypeEmitter::errorCodeName_.c_str(),
            proxyName_.c_str(), EmitMethodCmdID(method).c_str(), HdiTypeEmitter::dataParcelName_.c_str(),
            HdiTypeEmitter::replyParcelName_.c_str());
    }
    sb.Append(prefix).AppendFormat("if (%s != HDF_SUCCESS) {\n", HdiTypeEmitter::errorCodeName_.c_str());
    sb.Append(prefix + TAB).AppendFormat(
        "HDF_LOGE(\"%%{public}s: call failed! error code is %%{public}d\", __func__, %s);\n",
        HdiTypeEmitter::errorCodeName_.c_str());
    sb.Append(prefix + TAB).AppendFormat("goto %s;\n", FINISHED_LABEL);
    sb.Append(prefix).Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxyAsObjectMethodImpl(StringBuilder &sb) const
{
    sb.AppendFormat("static struct HdfRemoteService *%sProxyAsObject(struct %s *self)\n", baseName_.c_str(),
        interfaceName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("if (self == NULL) {\n");
    sb.Append(TAB).Append(TAB).Append("return NULL;\n");
    sb.Append(TAB).Append("}\n");
    sb.Append(TAB).AppendFormat(
        "struct %sProxy *proxy = CONTAINER_OF(self, struct %sProxy, impl);\n", baseName_.c_str(), baseName_.c_str());
    sb.Append(TAB).Append("return proxy->remote;\n");
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxyConstruction(StringBuilder &sb) const
{
    std::string objName = "impl";
    sb.AppendFormat(
        "static void %sProxyConstruct(struct %s *%s)\n", baseName_.c_str(), interfaceName_.c_str(), objName.c_str());
    sb.Append("{\n");

    for (const auto &method : interface_->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
        sb.Append(TAB).AppendFormat("%s->%s = %sProxy%s;\n", objName.c_str(), method->GetName().c_str(),
            baseName_.c_str(), method->GetName().c_str());
    }

    AutoPtr<ASTMethod> getVerMethod = interface_->GetVersionMethod();
    sb.Append(TAB).AppendFormat("%s->%s = %sProxy%s;\n", objName.c_str(), getVerMethod->GetName().c_str(),
        baseName_.c_str(), getVerMethod->GetName().c_str());

    if (mode_ != GenMode::KERNEL) {
        sb.Append(TAB).AppendFormat("%s->AsObject = %sProxyAsObject;\n", objName.c_str(), baseName_.c_str());
    }

    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxyExternalMethodImpl(StringBuilder &sb)
{
    if (mode_ == GenMode::KERNEL) {
        EmitKernelProxyExternalMethodImpl(sb);
    } else if (interface_->IsSerializable()) {
        EmitSerialProxyExternalMethodImpl(sb);
    } else {
        EmitProxyExternalMethod(sb);
    }
}

void CClientProxyCodeEmitter::EmitKernelProxyExternalMethodImpl(StringBuilder &sb)
{
    std::string remoteName = "serv";

    EmitKernelProxyGetMethodImpl(sb);
    sb.Append("\n");
    EmitKernelProxyGetInstanceMethodImpl(remoteName, sb);
    sb.Append("\n");
    EmitKernelProxyReleaseMethodImpl(remoteName, "HdfIoServiceRecycle", sb);
}

void CClientProxyCodeEmitter::EmitKernelProxyGetMethodImpl(StringBuilder &sb) const
{
    sb.AppendFormat("struct %s *%sGet()\n", interfaceName_.c_str(), interfaceName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).AppendFormat("return %sGetInstance(\"%s\");\n", interfaceName_.c_str(),
        FileName(implName_).c_str());
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitKernelProxyGetInstanceMethodImpl(const std::string &remoteName,
    StringBuilder &sb) const
{
    std::string objName = "client";
    std::string serMajorName = "serMajorVer";
    std::string serMinorName = "serMinorVer";
    std::string serviceName = "serviceName";

    sb.AppendFormat("struct %s *%sGetInstance(const char* %s)\n", interfaceName_.c_str(), interfaceName_.c_str(),
        serviceName.c_str());
    sb.Append("{\n");
    EmitProxyGetRemoteService(remoteName, serviceName, sb, TAB);
    sb.Append("\n");
    EmitProxyCreateProxyObject(objName, remoteName, "HdfIoServiceRecycle", sb, TAB);
    sb.Append("\n");
    EmitProxyCheckVersion(objName, serMajorName, serMinorName, sb, TAB);
    sb.Append("\n");
    sb.Append(TAB).AppendFormat("return %s;\n", objName.c_str());
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitKernelProxyReleaseMethodImpl(
    const std::string &remoteName, const std::string &recycleFuncName, StringBuilder &sb) const
{
    sb.AppendFormat("void %sRelease(struct %s *instance)\n", interfaceName_.c_str(), interfaceName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("if (instance == NULL) {\n");
    sb.Append(TAB).Append(TAB).Append("return;\n");
    sb.Append(TAB).Append("}\n");
    sb.Append(TAB).AppendFormat("struct %sProxy *proxy = CONTAINER_OF(instance, struct %sProxy, impl);\n",
        baseName_.c_str(), baseName_.c_str());
    sb.Append(TAB).AppendFormat("%s(proxy->%s);\n", recycleFuncName.c_str(), remoteName.c_str());
    sb.Append(TAB).Append("OsalMemFree(proxy);\n");
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitSerialProxyExternalMethodImpl(StringBuilder &sb)
{
    std::string serMajorName = "serMajorVer";
    std::string serMinorName = "serMinorVer";
    std::string remoteName = "remote";

    EmitIfaceProxyGetMethodImpl(serMajorName, serMinorName, remoteName, sb);
    sb.Append("\n");

    if (interface_->IsCallback()) {
        EmitCbProxyReleaseMethodImpl(remoteName, "HdfRemoteServiceRecycle", sb);
    } else {
        EmitIfaceProxyReleaseMethodImpl(remoteName, "HdfRemoteServiceRecycle", sb);
    }
}

void CClientProxyCodeEmitter::EmitIfaceProxyGetMethodImpl(const std::string &serMajorName,
    const std::string &serMinorName, const std::string &remoteName, StringBuilder &sb)
{
    std::string objName = "client";
    sb.AppendFormat("struct %s *%sGet(struct HdfRemoteService *%s)\n", interfaceName_.c_str(), interfaceName_.c_str(),
        remoteName.c_str());
    sb.Append("{\n");
    sb.Append(TAB).AppendFormat("if (%s == NULL) {\n", remoteName.c_str());
    sb.Append(TAB).Append(TAB).AppendFormat("HDF_LOGE(\"%%{public}s: remote is null\", __func__);\n");
    sb.Append(TAB).Append(TAB).Append("return NULL;\n");
    sb.Append(TAB).Append("}\n\n");
    EmitProxySetInterfaceDesc(remoteName, "HdfRemoteServiceRecycle", sb, TAB);
    sb.Append("\n");
    EmitProxyCreateProxyObject(objName, remoteName, "HdfRemoteServiceRecycle", sb, TAB);
    sb.Append("\n");
    EmitProxyCheckVersion(objName, serMajorName, serMinorName, sb, TAB);
    sb.Append("\n");
    sb.Append(TAB).AppendFormat("return %s;\n", objName.c_str());
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitIfaceProxyReleaseMethodImpl(
    const std::string &remoteName, const std::string &recycleFuncName, StringBuilder &sb) const
{
    std::string implReleaseMethodName = StringHelper::Format("%sImplRelease", interfaceName_.c_str());
    sb.AppendFormat("void %s(struct %s *instance) __attribute__((weak));\n", implReleaseMethodName.c_str(),
        interfaceName_.c_str());
    sb.AppendFormat("void %sRelease(struct %s *instance, bool isStub)\n", interfaceName_.c_str(),
        interfaceName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("if (instance == NULL) {\n");
    sb.Append(TAB).Append(TAB).Append("return;\n");
    sb.Append(TAB).Append("}\n");

    sb.Append(TAB).Append("if (isStub) {\n");
    sb.Append(TAB).Append(TAB).AppendFormat("if (%s != NULL) {\n", implReleaseMethodName.c_str());
    sb.Append(TAB).Append(TAB).Append(TAB).AppendFormat("%s(instance);\n", implReleaseMethodName.c_str());
    sb.Append(TAB).Append(TAB).Append("}\n");
    sb.Append(TAB).Append(TAB).Append("return;\n");
    sb.Append(TAB).Append("}\n");

    sb.Append(TAB).AppendFormat("struct %sProxy *proxy = CONTAINER_OF(instance, struct %sProxy, impl);\n",
        baseName_.c_str(), baseName_.c_str());
    sb.Append(TAB).AppendFormat("%s(proxy->%s);\n", recycleFuncName.c_str(), remoteName.c_str());
    sb.Append(TAB).Append("OsalMemFree(proxy);\n");
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitCbProxyReleaseMethodImpl(const std::string &remoteName,
    const std::string &recycleFuncName, StringBuilder &sb) const
{
    sb.AppendFormat("void %sRelease(struct %s *instance)\n", interfaceName_.c_str(), interfaceName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("if (instance == NULL) {\n");
    sb.Append(TAB).Append(TAB).Append("return;\n");
    sb.Append(TAB).Append("}\n\n");
    sb.Append(TAB).AppendFormat("struct %sProxy *proxy = CONTAINER_OF(instance, struct %sProxy, impl);\n",
        baseName_.c_str(), baseName_.c_str());
    sb.Append(TAB).AppendFormat("%s(proxy->%s);\n", recycleFuncName.c_str(), remoteName.c_str());
    sb.Append(TAB).Append("OsalMemFree(proxy);\n");
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxyExternalMethod(StringBuilder &sb)
{
    std::string remoteName = "remote";
    std::string serviceName = "serviceName";

    EmitProxyGetMethodImpl(sb);
    sb.Append("\n");
    EmitProxyGetInstanceMethodImpl(remoteName, serviceName, sb);
    sb.Append("\n");
    EmitProxyReleaseMethodImpl(sb);
    sb.Append("\n");
    EmitProxyReleaseInstanceMethodImpl(serviceName, remoteName, "HdfRemoteServiceRecycle", sb);
}

void CClientProxyCodeEmitter::EmitProxyGetMethodImpl(StringBuilder &sb) const
{
    sb.AppendFormat("struct %s *%sGet(bool isStub)\n", interfaceName_.c_str(), interfaceName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).AppendFormat("return %sGetInstance(\"%s\", isStub);\n", interfaceName_.c_str(),
        FileName(implName_).c_str());
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxyGetInstanceMethodImpl(const std::string &remoteName,
    const std::string &serviceName, StringBuilder &sb) const
{
    std::string objName = "client";
    std::string serMajorName = "serMajorVer";
    std::string serMinorName = "serMinorVer";

    sb.AppendFormat("struct %s *%sGetInstance(const char *%s, bool isStub)\n", interfaceName_.c_str(),
        interfaceName_.c_str(), serviceName.c_str());
    sb.Append("{\n");
    EmitProxyLoadOrUnLoadHdiImpl(serviceName, true, sb, TAB);
    sb.Append("\n");
    EmitProxyGetRemoteService(remoteName, serviceName, sb, TAB);
    sb.Append("\n");
    EmitProxySetInterfaceDesc(remoteName, "HdfRemoteServiceRecycle", sb, TAB);
    sb.Append("\n");
    EmitProxyCreateProxyObject(objName, remoteName, "HdfRemoteServiceRecycle", sb, TAB);
    sb.Append("\n");
    EmitProxyCheckVersion(objName, serMajorName, serMinorName, sb, TAB);
    sb.Append("\n");
    sb.Append(TAB).AppendFormat("return %s;\n", objName.c_str());
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxyGetRemoteService(const std::string &remoteName,
    const std::string &serviceName, StringBuilder &sb, const std::string &prefix) const
{
    if (mode_ != GenMode::KERNEL) {
        sb.Append(prefix).Append("struct HDIServiceManager *serviceMgr = HDIServiceManagerGet();\n");
        sb.Append(prefix).Append("if (serviceMgr == NULL) {\n");
        sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: HDIServiceManager not found!\", __func__);\n");
        sb.Append(prefix + TAB).Append("return NULL;\n");
        sb.Append(prefix).Append("}\n\n");
        sb.Append(prefix).AppendFormat("struct HdfRemoteService *%s = ", remoteName.c_str());
        sb.AppendFormat("serviceMgr->GetService(serviceMgr, %s);\n", serviceName.c_str());
        sb.Append(prefix).Append("HDIServiceManagerRelease(serviceMgr);\n");
        sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", remoteName.c_str());
        sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to get remote!\", __func__);\n");
        sb.Append(prefix + TAB).Append("return NULL;\n");
        sb.Append(prefix).Append("}\n");
    } else {
        sb.Append(prefix).AppendFormat("struct HdfIoService *%s = ", remoteName.c_str());
        sb.AppendFormat("HdfIoServiceBind(%s);\n", serviceName.c_str());
        sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", remoteName.c_str());
        sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: failed to get io service!\", __func__);\n");
        sb.Append(prefix + TAB).Append("return NULL;\n");
        sb.Append(prefix).Append("}\n");
    }
}

void CClientProxyCodeEmitter::EmitProxySetInterfaceDesc(const std::string &remoteName,
    const std::string &recycleFuncName, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("if (!HdfRemoteServiceSetInterfaceDesc(%s, %s)) {\n", remoteName.c_str(),
        EmitDescMacroName().c_str());
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: set interface token failed!\", __func__);\n");
    sb.Append(prefix + TAB).AppendFormat("%s(%s);\n", recycleFuncName.c_str(), remoteName.c_str());
    sb.Append(prefix + TAB).Append("return NULL;\n");
    sb.Append(prefix).Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxyCreateProxyObject(const std::string &clientObjName,
    const std::string &remoteName, const std::string &recycleFuncName, StringBuilder &sb,
    const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("struct %sProxy *proxy = (struct %sProxy *)OsalMemCalloc(sizeof(struct %sProxy));\n",
        baseName_.c_str(), baseName_.c_str(), baseName_.c_str());
    sb.Append(prefix).Append("if (proxy == NULL) {\n");
    sb.Append(prefix + TAB)
        .AppendFormat("HDF_LOGE(\"%%{public}s: malloc %s proxy failed!\", __func__);\n", interfaceName_.c_str());
    if (!interface_->IsSerializable()) {
        sb.Append(prefix + TAB).AppendFormat("%s(%s);\n", recycleFuncName.c_str(), remoteName.c_str());
    }
    sb.Append(prefix + TAB).Append("return NULL;\n");
    sb.Append(prefix).Append("}\n");
    sb.Append("\n");
    sb.Append(prefix).AppendFormat("proxy->%s = %s;\n", remoteName.c_str(), remoteName.c_str());
    sb.Append(prefix).AppendFormat("%sProxyConstruct(&proxy->impl);\n", baseName_.c_str());
    sb.Append(prefix).AppendFormat("struct %s *%s = &proxy->impl;\n", interfaceName_.c_str(), clientObjName.c_str());
}

void CClientProxyCodeEmitter::EmitProxyCheckVersion(const std::string &clientObjName, const std::string &serMajorName,
    const std::string &serMinorName, StringBuilder &sb, const std::string &prefix) const
{
    sb.Append(prefix).AppendFormat("uint32_t %s = 0;\n", serMajorName.c_str());
    sb.Append(prefix).AppendFormat("uint32_t %s = 0;\n", serMinorName.c_str());
    sb.Append(prefix).AppendFormat("int32_t %s = %s->GetVersion(%s, &%s, &%s);\n",
        HdiTypeEmitter::errorCodeName_.c_str(), clientObjName.c_str(),
        clientObjName.c_str(), serMajorName.c_str(), serMinorName.c_str());
    sb.Append(prefix).AppendFormat("if (%s != HDF_SUCCESS) {\n", HdiTypeEmitter::errorCodeName_.c_str());
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s: get version failed!\", __func__);\n");
    if (mode_ == GenMode::KERNEL || interface_->IsCallback()) {
        sb.Append(prefix + TAB).AppendFormat("%sRelease(%s);\n", interfaceName_.c_str(), clientObjName.c_str());
    } else if (mode_ == GenMode::PASSTHROUGH || loadVersionCheck_) {
        sb.Append(prefix + TAB).AppendFormat("%sRelease(%s, true);\n", interfaceName_.c_str(), clientObjName.c_str());
    } else {
        sb.Append(prefix + TAB).AppendFormat("%sRelease(%s, false);\n", interfaceName_.c_str(), clientObjName.c_str());
    }

    sb.Append(prefix + TAB).Append("return NULL;\n").Append(prefix).Append("}\n\n");
    sb.Append(prefix).AppendFormat("if (%s != %s) {\n", serMajorName.c_str(), majorVerName_.c_str());
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s:check version failed! ");
    sb.Append("version of service:%u.%u, version of client:%u.%u\", __func__,\n");
    sb.Append(prefix + TAB + TAB).AppendFormat("%s, %s, %s, %s);\n", serMajorName.c_str(), serMinorName.c_str(),
        majorVerName_.c_str(), minorVerName_.c_str());
    if (mode_ == GenMode::KERNEL || interface_->IsCallback()) {
        sb.Append(prefix + TAB).AppendFormat("%sRelease(%s);\n", interfaceName_.c_str(), clientObjName.c_str());
    } else if (mode_ == GenMode::PASSTHROUGH || loadVersionCheck_) {
        sb.Append(prefix + TAB).AppendFormat("%sRelease(%s, true);\n", interfaceName_.c_str(), clientObjName.c_str());
    } else {
        sb.Append(prefix + TAB).AppendFormat("%sRelease(%s, false);\n", interfaceName_.c_str(), clientObjName.c_str());
    }
    sb.Append(prefix + TAB).Append("return NULL;\n").Append(prefix).Append("}\n\n");
    sb.Append(prefix).AppendFormat("if (%s < %s) {\n", serMinorName.c_str(), minorVerName_.c_str());
    sb.Append(prefix + TAB).Append("HDF_LOGE(\"%{public}s:check version failed! ");
    sb.Append("client minor version(%u) should be less \"\n");
    sb.Append(prefix + TAB + TAB).Append("\"or equal to server minor version(%u).");
    sb.AppendFormat("\", __func__, %s, %s);\n", minorVerName_.c_str(), serMinorName.c_str());
    if (mode_ == GenMode::KERNEL || interface_->IsCallback()) {
        sb.Append(prefix + TAB).AppendFormat("%sRelease(%s);\n", interfaceName_.c_str(), clientObjName.c_str());
    } else if (mode_ == GenMode::PASSTHROUGH || loadVersionCheck_) {
        sb.Append(prefix + TAB).AppendFormat("%sRelease(%s, true);\n", interfaceName_.c_str(), clientObjName.c_str());
    } else {
        sb.Append(prefix + TAB).AppendFormat("%sRelease(%s, false);\n", interfaceName_.c_str(), clientObjName.c_str());
    }
    sb.Append(prefix + TAB).Append("return NULL;\n").Append(prefix).Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxyReleaseMethodImpl(StringBuilder &sb) const
{
    sb.AppendFormat(
        "void %sRelease(struct %s *instance, bool isStub)\n", interfaceName_.c_str(), interfaceName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).AppendFormat(
        "%sReleaseInstance(\"%s\", instance, isStub);\n", interfaceName_.c_str(), FileName(implName_).c_str());
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxyReleaseInstanceMethodImpl(const std::string &serviceName,
    const std::string &remoteName, const std::string &recycleFuncName, StringBuilder &sb)
{
    sb.AppendFormat("void %sReleaseInstance(const char *%s, struct %s *instance, bool isStub)\n",
        interfaceName_.c_str(), serviceName.c_str(), interfaceName_.c_str());
    sb.Append("{\n");
    sb.Append(TAB).Append("if (instance == NULL) {\n");
    sb.Append(TAB).Append(TAB).Append("return;\n");
    sb.Append(TAB).Append("}\n\n");
    EmitProxyLoadOrUnLoadHdiImpl(serviceName, false, sb, TAB);
    sb.Append("\n");
    sb.Append(TAB).AppendFormat("struct %sProxy *proxy = CONTAINER_OF(instance, struct %sProxy, impl);\n",
        baseName_.c_str(), baseName_.c_str());
    sb.Append(TAB).AppendFormat("%s(proxy->%s);\n", recycleFuncName.c_str(), remoteName.c_str());
    sb.Append(TAB).Append("OsalMemFree(proxy);\n");
    sb.Append("}\n");
}

void CClientProxyCodeEmitter::EmitProxyLoadOrUnLoadHdiImpl(const std::string &serviceName, bool isLoad,
    StringBuilder &sb, const std::string &prefix) const
{
    std::string instName = "instName";
    sb.Append(prefix).Append("if (isStub) {\n");
    sb.Append(prefix + TAB).AppendFormat("const char *%s = %s;\n", instName.c_str(), serviceName.c_str());
    sb.Append(prefix + TAB)
        .AppendFormat("if (strcmp(%s, \"%s\") == 0) {\n", instName.c_str(), FileName(implName_).c_str());
    sb.Append(prefix + TAB + TAB).AppendFormat("%s = \"service\";\n", instName.c_str());
    sb.Append(prefix + TAB).Append("}\n");
    if (isLoad) {
        std::string objName = "inst";
        std::string serMajorName = "serMajorVer";
        std::string serMinorName = "serMinorVer";

        sb.Append(prefix + TAB).AppendFormat("struct %s *inst = LoadHdiImpl(%s, %s);\n",
            interfaceName_.c_str(), EmitDescMacroName().c_str(), instName.c_str());
        sb.Append(prefix + TAB).Append("if (inst == NULL) {\n");
        sb.Append(prefix + TAB + TAB).AppendFormat("HDF_LOGE(\"%%{public}s, failed to load hdi "
            "impl %%{public}s!\", __func__, %s);\n", EmitDescMacroName().c_str());

        sb.Append(prefix + TAB + TAB).Append("return NULL;\n").Append(prefix + TAB).Append("}\n");
        sb.Append(prefix + TAB).Append("if (inst->GetVersion == NULL) {\n");

        sb.Append(prefix + TAB + TAB).Append("HDF_LOGE(\"%{public}s: "
            "GetVersion is not implemented!\", __func__);\n");
        if (mode_ == GenMode::KERNEL) {
            sb.Append(prefix + TAB + TAB).AppendFormat("%sRelease(%s);\n", \
                interfaceName_.c_str(), objName.c_str());
        } else if (interface_->IsCallback()) {
            sb.Append(prefix + TAB + TAB).AppendFormat("%sRelease(%s);\n", \
                interfaceName_.c_str(), objName.c_str());
        } else if (mode_ == GenMode::PASSTHROUGH || isLoad) {
            sb.Append(prefix + TAB + TAB).AppendFormat("%sRelease(%s, true);\n", \
                interfaceName_.c_str(), objName.c_str());
        } else {
            sb.Append(prefix + TAB + TAB).AppendFormat("%sRelease(%s, false);\n", \
                interfaceName_.c_str(), objName.c_str());
        }
        sb.Append(prefix + TAB + TAB).Append("return NULL;\n").Append(prefix + TAB).Append("}\n");
        loadVersionCheck_ = true;
        EmitProxyCheckVersion(objName, serMajorName, serMinorName, sb, prefix + TAB);
        loadVersionCheck_ = false;
        sb.Append(prefix + TAB).Append("return inst;\n");
    } else {
        sb.Append(prefix + TAB).AppendFormat("UnloadHdiImpl(%s, %s, instance);\n",
            EmitDescMacroName().c_str(), instName.c_str());
        sb.Append(prefix + TAB).Append("return;\n");
    }
    sb.Append(prefix).Append("}\n");
}

void CClientProxyCodeEmitter::EmitUtilMethods(StringBuilder &sb, bool isDecl)
{
    UtilMethodMap methods;
    for (const auto &method : interface_->GetMethodsBySystem(Options::GetInstance().GetSystemLevel())) {
        for (size_t paramIndex = 0; paramIndex < method->GetParameterNumber(); paramIndex++) {
            AutoPtr<ASTParameter> param = method->GetParameter(paramIndex);
            AutoPtr<HdiTypeEmitter> typeEmitter = GetTypeEmitter(param->GetType());
            if (typeEmitter == nullptr) {
                continue;
            }
            if (param->GetAttribute() == ASTParamAttr::PARAM_IN) {
                typeEmitter->EmitCWriteMethods(methods, "", "", isDecl);
            } else {
                typeEmitter->EmitCReadMethods(methods, "", "", isDecl);
            }
        }
    }
    EmitUtilMethodMap(sb, methods);
}
} // namespace Idl
} // namespace OHOS
