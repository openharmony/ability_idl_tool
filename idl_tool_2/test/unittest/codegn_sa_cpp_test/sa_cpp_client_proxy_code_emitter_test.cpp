/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "test_log.h"

#ifdef IDLTOOL_GTEST
#define private public
#define protected public
#endif
#include "codegen/SA/cpp/sa_cpp_client_proxy_code_emitter.h"
#include "codegen/SA/cpp/sa_cpp_interface_code_emitter.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Idl;

namespace OHOS::Idl {
class SaCppClientProxyCodeEmitterTest : public testing::Test {
public:
    SaCppClientProxyCodeEmitterTest() {}
    virtual ~SaCppClientProxyCodeEmitterTest() {}
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SaCppClientProxyCodeEmitterTest::SetUpTestCase()
{
    DTEST_LOG << "SetUpTestCase" << std::endl;
}

void SaCppClientProxyCodeEmitterTest::TearDownTestCase()
{
    DTEST_LOG << "TearDownTestCase" << std::endl;
}

void SaCppClientProxyCodeEmitterTest::SetUp()
{
    DTEST_LOG << "SetUp" << std::endl;
}

void SaCppClientProxyCodeEmitterTest::TearDown()
{
    DTEST_LOG << "TearDown" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyConstructor_001
 * @tc.desc: test EmitInterfaceProxyConstructor function.
 * @tc.type: FUNC
  * @tc.require: #I8JQUO
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyConstructor_001, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyConstructor_001 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    std::string methodName0 = "methodName0";
    std::string methodName1 = "methodName1";
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    ASTMethod* method0 = new ASTMethod();
    ASTType* methodType0 = new ASTVoidType();
    method0->SetName(methodName0);
    method0->SetReturnType(methodType0);
    AutoPtr<ASTAttr> attr0 = new ASTAttr(ASTAttr::ONEWAY);
    attr0->SetCacheableTimeString("0");
    method0->SetCacheable(attr0);
    
    ASTMethod* method1 = new ASTMethod();
    ASTType* methodType1 = new ASTVoidType();
    method1->SetName(methodName1);
    method1->SetReturnType(methodType1);
    AutoPtr<ASTAttr> attr1 = new ASTAttr(ASTAttr::CACHEABLE);
    attr1->cacheableTime_ = 1;
    method1->SetCacheable(attr1);
    emitter->logOn_ = false;
    emitter->ast_ = new AST();
    emitter->proxyName_ = "FooProxy";
    emitter->interfaceName_ = "IFoo";
    emitter->ast_->SetHasCacheableProxyMethods(true);
    emitter->interface_ = new ASTInterfaceType();
    emitter->interface_->AddMethod(method0);
    emitter->interface_->AddMethod(method1);
    emitter->EmitInterfaceProxyConstructor(sb, prefix);
    std::string expectedCode =
        "explicit FooProxy(\n"
        "    const sptr<IRemoteObject>& remote)\n"
        "    : IRemoteProxy<IFoo>(remote)\n"
        "{\n"
        "ApiCacheManager::GetInstance().AddCacheApi(GetDescriptor(),\n"
        "    static_cast<uint32_t>(IpcCode::COMMAND_METHOD_NAME1), 0);\n"
        "\n"
        "    if (remote_) {\n"
        "        if (!remote->IsProxyObject()) {\n"
        "            return;\n"
        "        }\n"
        "        deathRecipient_ = new (std::nothrow) (*this);\n"
        "        if (!deathRecipient_) {\n"
        "            return;\n"
        "        }\n"
        "        if (!remote->AddDeathRecipient(deathRecipient_)) {\n"
        "            return;\n"
        "        }\n"
        "        remote_ = remote;\n"
        "    }\n"
        "}\n"
        "\n"
        "virtual ~FooProxy()\n"
        "{\n"
        "    if (remote_ == nullptr) {\n"
        "        return;\n"
        "    }\n"
        "    if (deathRecipient_ == nullptr) {\n"
        "        return;\n"
        "    }\n"
        "    remote_->RemoveDeathRecipient(deathRecipient_);\n"
        "    remote_ = nullptr;\n"
        "\n"
        "ApiCacheManager::GetInstance().DelCacheApi(GetDescriptor(),\n"
        "    static_cast<uint32_t>(IpcCode::COMMAND_METHOD_NAME1));\n"
        "}\n";

    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyConstructor_001 end" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyConstructor_002
 * @tc.desc: test EmitInterfaceProxyConstructor function.
 * @tc.type: FUNC
  * @tc.require: #I8JQUO
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyConstructor_002, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyConstructor_002 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    emitter->interface_ = new ASTInterfaceType();
    std::string methodName = "methodName";
    ASTMethod* method = new ASTMethod();
    method->SetName(methodName);
    method->attr_ = new ASTAttr(ASTAttr::CACHEABLE);
    method->attr_->cacheableTime_ = 23;
    emitter->interface_->AddMethod(method);
    emitter->logOn_ = true;
    emitter->proxyName_ = "UProxy";
    emitter->interfaceName_ = "Iuu";
    emitter->ast_ = new AST();
    emitter->ast_->SetHasCacheableProxyMethods(true);

    emitter->EmitInterfaceProxyConstructor(sb, prefix);
    std::string expectedCode =
        "explicit UProxy(\n"
        "    const sptr<IRemoteObject>& remote)\n"
        "    : IRemoteProxy<Iuu>(remote)\n"
        "{\n"
        "ApiCacheManager::GetInstance().AddCacheApi(GetDescriptor(),\n"
        "    static_cast<uint32_t>(IpcCode::COMMAND_METHOD_NAME), 23);\n"
        "\n"
        "    if (remote_) {\n"
        "        if (!remote->IsProxyObject()) {\n"
        "            HiLog::Error(LABEL, \"remote is not proxy object!\");\n"
        "            return;\n"
        "        }\n"
        "        deathRecipient_ = new (std::nothrow) (*this);\n"
        "        if (!deathRecipient_) {\n"
        "            HiLog::Error(LABEL, \"deathRecipient_ is nullptr!\");\n"
        "            return;\n"
        "        }\n"
        "        if (!remote->AddDeathRecipient(deathRecipient_)) {\n"
        "            HiLog::Error(LABEL, \"AddDeathRecipient failed!\");\n"
        "            return;\n"
        "        }\n"
        "        remote_ = remote;\n"
        "    }\n"
        "}\n"
        "\n"
        "virtual ~UProxy()\n"
        "{\n"
        "    if (remote_ == nullptr) {\n"
        "        return;\n"
        "    }\n"
        "    if (deathRecipient_ == nullptr) {\n"
        "        return;\n"
        "    }\n"
        "    remote_->RemoveDeathRecipient(deathRecipient_);\n"
        "    remote_ = nullptr;\n"
        "\n"
        "ApiCacheManager::GetInstance().DelCacheApi(GetDescriptor(),\n"
        "    static_cast<uint32_t>(IpcCode::COMMAND_METHOD_NAME));\n"
        "}\n";

    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyConstructor_002 end" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyConstants_001
 * @tc.desc: test EmitInterfaceProxyConstants function.
 * @tc.type: FUNC
  * @tc.require: #I8JQUO
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyConstants_001, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyConstants_001 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    std::string methodName = "methodName";
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    ASTMethod* method = new ASTMethod();
    ASTType* methodType = new ASTVoidType();
    method->SetName(methodName);
    method->SetReturnType(methodType);
    AutoPtr<ASTAttr> attr = new ASTAttr(ASTAttr::IPC_IN_CAPACITY);
    attr->SetCacheableTimeString("0");
    method->SetCacheable(attr);
    emitter->ast_ = new AST();
    emitter->ast_->SetHasCacheableProxyMethods(true);
    emitter->interface_ = new ASTInterfaceType();
    emitter->interface_->AddMethod(method);
    emitter->deathRecipientName_ = "FooRecipient";
    emitter->proxyName_ = "FooProxy";
    emitter->EmitInterfaceProxyConstants(sb, prefix);
    std::string expectedCode =
        "class FooRecipient : public IRemoteObject::DeathRecipient {\n"
        "public:\n"
        "    explicit FooRecipient(FooProxy &client) : client_(client) {}\n"
        "    ~FooRecipient() override = default;\n"
        "    void OnRemoteDied(const wptr<IRemoteObject> &remote) override\n"
        "    {\n"
        "        client_.OnRemoteDied(remote);\n"
        "    }\n"
        "private:\n"
        "    FooProxy &client_;\n"
        "};\n"
        "\n"
        "void OnRemoteDied(const wptr<IRemoteObject> &remoteObject)\n"
        "{\n"
        "    (void)remoteObject;\n"
        "    ApiCacheManager::GetInstance().ClearCache(GetDescriptor());\n"
        "}\n"
        "sptr<IRemoteObject> remote_ = nullptr;\n"
        "sptr<IRemoteObject::DeathRecipient> deathRecipient_ = nullptr;\n"
        "static inline BrokerDelegator<FooProxy> delegator_;\n";

    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyConstants_001 end" << std::endl;
}

/*
 * @tc.name: UnitPreprocess_test_001
 * @tc.desc: test UnitPreprocess function.
 * @tc.type: FUNC
  * @tc.require: #I8JQUO
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, GetFilePathNoPoint_001, Level1)
{
    DTEST_LOG << "GetFilePathNoPoint_001 begin" << std::endl;
    AutoPtr<SaCppInterfaceCodeEmitter> emitter = new SaCppInterfaceCodeEmitter;
    std::string filePath = "myTest/../fcallback/ICallback";
    auto ret = emitter->GetFilePath(filePath);
    EXPECT_EQ(ret, "myTest/.");
    ret = emitter->GetFilePathNoPoint(filePath);
    EXPECT_EQ(ret, "myTest/");
    filePath = "/fcallback/ICallback";
    ret = emitter->GetFilePath(filePath);
    ret = emitter->GetFilePathNoPoint(filePath);
    EXPECT_EQ(ret, "");
    filePath = "";
    ret = emitter->CppFullName(filePath);
    EXPECT_EQ(ret, "");
    filePath = "test.MySeq2";
    ret = emitter->CppFullName(filePath);
    EXPECT_EQ(ret, "test::MySeq2");
    DTEST_LOG << "GetFilePathNoPoint_001 end" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyMethodBody_OnewayCustomMsgOptNoAsync_001
 * @tc.desc: oneway + customMsgOption(without TF_ASYNC) should generate async option.
 * @tc.type: FUNC
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyMethodBody_OnewayCustomMsgOptNoAsync_001, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptNoAsync_001 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    emitter->logOn_ = false;
    emitter->hitraceOn_ = false;
    emitter->interface_ = new ASTInterfaceType();
    emitter->interface_->SetName("IFoo");
    emitter->SetOverloadName("");

    AutoPtr<ASTMethod> method = new ASTMethod();
    method->SetName("test_func");
    method->SetReturnType(new ASTVoidType());
    method->attr_ = new ASTAttr(ASTAttr::ONEWAY | ASTAttr::CUSTOM_MSG_OPTION);
    method->SetMessageOption("MessageOption::TF_ASYNC_WAKEUP_LATER");

    emitter->EmitInterfaceProxyMethodBody(method, sb, prefix);
    std::string expectedCode =
        "{\n"
        "    MessageParcel data;\n"
        "    MessageParcel reply;\n"
        "    MessageOption option(MessageOption::TF_ASYNC | MessageOption::TF_ASYNC_WAKEUP_LATER);\n"
        "\n"
        "    if (!data.WriteInterfaceToken(GetDescriptor())) {\n"
        "        return ERR_INVALID_VALUE;\n"
        "    }\n"
        "\n"
        "    sptr<IRemoteObject> remote = Remote();\n"
        "    if (!remote) {\n"
        "        return ERR_INVALID_DATA;\n"
        "    }\n"
        "    int32_t result = remote->SendRequest(\n"
        "        static_cast<uint32_t>(IFooIpcCode::COMMAND_TEST_FUNC), data, reply, option);\n"
        "    if (FAILED(result)) {\n"
        "        return result;\n"
        "    }\n"
        "    return ERR_OK;\n"
        "}\n";
    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptNoAsync_001 end" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyMethodBody_OnewayCustomMsgOptHasAsync_001
 * @tc.desc: oneway + customMsgOption(already has TF_ASYNC) should not duplicate async flag.
 * @tc.type: FUNC
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyMethodBody_OnewayCustomMsgOptHasAsync_001, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptHasAsync_001 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    emitter->logOn_ = false;
    emitter->hitraceOn_ = false;
    emitter->interface_ = new ASTInterfaceType();
    emitter->interface_->SetName("IFoo");
    emitter->SetOverloadName("");

    AutoPtr<ASTMethod> method = new ASTMethod();
    method->SetName("test_func");
    method->SetReturnType(new ASTVoidType());
    method->attr_ = new ASTAttr(ASTAttr::ONEWAY | ASTAttr::CUSTOM_MSG_OPTION);
    method->SetMessageOption("MessageOption::TF_ASYNC_WAKEUP_LATER | MessageOption::TF_ASYNC");

    emitter->EmitInterfaceProxyMethodBody(method, sb, prefix);
    std::string expectedCode =
        "{\n"
        "    MessageParcel data;\n"
        "    MessageParcel reply;\n"
        "    MessageOption option(MessageOption::TF_ASYNC_WAKEUP_LATER | MessageOption::TF_ASYNC);\n"
        "\n"
        "    if (!data.WriteInterfaceToken(GetDescriptor())) {\n"
        "        return ERR_INVALID_VALUE;\n"
        "    }\n"
        "\n"
        "    sptr<IRemoteObject> remote = Remote();\n"
        "    if (!remote) {\n"
        "        return ERR_INVALID_DATA;\n"
        "    }\n"
        "    int32_t result = remote->SendRequest(\n"
        "        static_cast<uint32_t>(IFooIpcCode::COMMAND_TEST_FUNC), data, reply, option);\n"
        "    if (FAILED(result)) {\n"
        "        return result;\n"
        "    }\n"
        "    return ERR_OK;\n"
        "}\n";
    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptHasAsync_001 end" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyMethodBody_OnewayCustomMsgOptWaitTime_001
 * @tc.desc: oneway + customMsgOption(no TF_ASYNC, with waitTime) should add async to flags only.
 * @tc.type: FUNC
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyMethodBody_OnewayCustomMsgOptWaitTime_001, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptWaitTime_001 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    emitter->logOn_ = false;
    emitter->hitraceOn_ = false;
    emitter->interface_ = new ASTInterfaceType();
    emitter->interface_->SetName("IFoo");
    emitter->SetOverloadName("");

    AutoPtr<ASTMethod> method = new ASTMethod();
    method->SetName("test_func");
    method->SetReturnType(new ASTVoidType());
    method->attr_ = new ASTAttr(ASTAttr::ONEWAY | ASTAttr::CUSTOM_MSG_OPTION);
    method->SetMessageOption("MessageOption::TF_ASYNC_WAKEUP_LATER, MessageOption::TF_WAIT_TIME");

    emitter->EmitInterfaceProxyMethodBody(method, sb, prefix);
    std::string expectedCode =
        "{\n"
        "    MessageParcel data;\n"
        "    MessageParcel reply;\n"
        "    MessageOption option(MessageOption::TF_ASYNC | MessageOption::TF_ASYNC_WAKEUP_LATER"
        ", MessageOption::TF_WAIT_TIME);\n"
        "\n"
        "    if (!data.WriteInterfaceToken(GetDescriptor())) {\n"
        "        return ERR_INVALID_VALUE;\n"
        "    }\n"
        "\n"
        "    sptr<IRemoteObject> remote = Remote();\n"
        "    if (!remote) {\n"
        "        return ERR_INVALID_DATA;\n"
        "    }\n"
        "    int32_t result = remote->SendRequest(\n"
        "        static_cast<uint32_t>(IFooIpcCode::COMMAND_TEST_FUNC), data, reply, option);\n"
        "    if (FAILED(result)) {\n"
        "        return result;\n"
        "    }\n"
        "    return ERR_OK;\n"
        "}\n";
    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptWaitTime_001 end" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyMethodBody_OnewayOnly_001
 * @tc.desc: oneway only (no customMsgOption) should generate TF_ASYNC.
 * @tc.type: FUNC
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyMethodBody_OnewayOnly_001, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayOnly_001 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    emitter->logOn_ = false;
    emitter->hitraceOn_ = false;
    emitter->interface_ = new ASTInterfaceType();
    emitter->interface_->SetName("IFoo");
    emitter->SetOverloadName("");

    AutoPtr<ASTMethod> method = new ASTMethod();
    method->SetName("test_func");
    method->SetReturnType(new ASTVoidType());
    method->attr_ = new ASTAttr(ASTAttr::ONEWAY);

    emitter->EmitInterfaceProxyMethodBody(method, sb, prefix);
    std::string expectedCode =
        "{\n"
        "    MessageParcel data;\n"
        "    MessageParcel reply;\n"
        "    MessageOption option(MessageOption::TF_ASYNC);\n"
        "\n"
        "    if (!data.WriteInterfaceToken(GetDescriptor())) {\n"
        "        return ERR_INVALID_VALUE;\n"
        "    }\n"
        "\n"
        "    sptr<IRemoteObject> remote = Remote();\n"
        "    if (!remote) {\n"
        "        return ERR_INVALID_DATA;\n"
        "    }\n"
        "    int32_t result = remote->SendRequest(\n"
        "        static_cast<uint32_t>(IFooIpcCode::COMMAND_TEST_FUNC), data, reply, option);\n"
        "    if (FAILED(result)) {\n"
        "        return result;\n"
        "    }\n"
        "    return ERR_OK;\n"
        "}\n";
    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayOnly_001 end" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyMethodBody_DefaultSync_001
 * @tc.desc: neither oneway nor customMsgOption should generate default TF_SYNC.
 * @tc.type: FUNC
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyMethodBody_DefaultSync_001, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyMethodBody_DefaultSync_001 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    emitter->logOn_ = false;
    emitter->hitraceOn_ = false;
    emitter->interface_ = new ASTInterfaceType();
    emitter->interface_->SetName("IFoo");
    emitter->SetOverloadName("");

    AutoPtr<ASTMethod> method = new ASTMethod();
    method->SetName("test_func");
    method->SetReturnType(new ASTVoidType());
    method->attr_ = new ASTAttr();

    emitter->EmitInterfaceProxyMethodBody(method, sb, prefix);
    std::string expectedCode =
        "{\n"
        "    MessageParcel data;\n"
        "    MessageParcel reply;\n"
        "    MessageOption option(MessageOption::TF_SYNC);\n"
        "\n"
        "    if (!data.WriteInterfaceToken(GetDescriptor())) {\n"
        "        return ERR_INVALID_VALUE;\n"
        "    }\n"
        "\n"
        "    sptr<IRemoteObject> remote = Remote();\n"
        "    if (!remote) {\n"
        "        return ERR_INVALID_DATA;\n"
        "    }\n"
        "    int32_t result = remote->SendRequest(\n"
        "        static_cast<uint32_t>(IFooIpcCode::COMMAND_TEST_FUNC), data, reply, option);\n"
        "    if (FAILED(result)) {\n"
        "        return result;\n"
        "    }\n"
        "\n"
        "    ErrCode errCode = reply.ReadInt32();\n"
        "    if (FAILED(errCode)) {\n"
        "        return errCode;\n"
        "    }\n"
        "\n"
        "    return ERR_OK;\n"
        "}\n";
    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyMethodBody_DefaultSync_001 end" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyMethodBody_CustomMsgOptOnlySync_001
 * @tc.desc: customMsgOption(TF_SYNC) without oneway should use option as-is.
 * @tc.type: FUNC
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyMethodBody_CustomMsgOptOnlySync_001, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyMethodBody_CustomMsgOptOnlySync_001 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    emitter->logOn_ = false;
    emitter->hitraceOn_ = false;
    emitter->interface_ = new ASTInterfaceType();
    emitter->interface_->SetName("IFoo");
    emitter->SetOverloadName("");

    AutoPtr<ASTMethod> method = new ASTMethod();
    method->SetName("test_func");
    method->SetReturnType(new ASTVoidType());
    method->attr_ = new ASTAttr(ASTAttr::CUSTOM_MSG_OPTION);
    method->SetMessageOption("MessageOption::TF_SYNC");

    emitter->EmitInterfaceProxyMethodBody(method, sb, prefix);
    std::string expectedCode =
        "{\n"
        "    MessageParcel data;\n"
        "    MessageParcel reply;\n"
        "    MessageOption option(MessageOption::TF_SYNC);\n"
        "\n"
        "    if (!data.WriteInterfaceToken(GetDescriptor())) {\n"
        "        return ERR_INVALID_VALUE;\n"
        "    }\n"
        "\n"
        "    sptr<IRemoteObject> remote = Remote();\n"
        "    if (!remote) {\n"
        "        return ERR_INVALID_DATA;\n"
        "    }\n"
        "    int32_t result = remote->SendRequest(\n"
        "        static_cast<uint32_t>(IFooIpcCode::COMMAND_TEST_FUNC), data, reply, option);\n"
        "    if (FAILED(result)) {\n"
        "        return result;\n"
        "    }\n"
        "\n"
        "    ErrCode errCode = reply.ReadInt32();\n"
        "    if (FAILED(errCode)) {\n"
        "        return errCode;\n"
        "    }\n"
        "\n"
        "    return ERR_OK;\n"
        "}\n";
    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyMethodBody_CustomMsgOptOnlySync_001 end" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncSpace_001
 * @tc.desc: HasAsyncMessageOption boundary: TF_ASYNC followed by space.
 * @tc.type: FUNC
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncSpace_001, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncSpace_001 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    emitter->logOn_ = false;
    emitter->hitraceOn_ = false;
    emitter->interface_ = new ASTInterfaceType();
    emitter->interface_->SetName("IFoo");
    emitter->SetOverloadName("");

    AutoPtr<ASTMethod> method = new ASTMethod();
    method->SetName("test_func");
    method->SetReturnType(new ASTVoidType());
    method->attr_ = new ASTAttr(ASTAttr::ONEWAY | ASTAttr::CUSTOM_MSG_OPTION);
    method->SetMessageOption("MessageOption::TF_ASYNC | MessageOption::TF_ASYNC_WAKEUP_LATER");

    emitter->EmitInterfaceProxyMethodBody(method, sb, prefix);
    std::string expectedCode =
        "{\n"
        "    MessageParcel data;\n"
        "    MessageParcel reply;\n"
        "    MessageOption option(MessageOption::TF_ASYNC | MessageOption::TF_ASYNC_WAKEUP_LATER);\n"
        "\n"
        "    if (!data.WriteInterfaceToken(GetDescriptor())) {\n"
        "        return ERR_INVALID_VALUE;\n"
        "    }\n"
        "\n"
        "    sptr<IRemoteObject> remote = Remote();\n"
        "    if (!remote) {\n"
        "        return ERR_INVALID_DATA;\n"
        "    }\n"
        "    int32_t result = remote->SendRequest(\n"
        "        static_cast<uint32_t>(IFooIpcCode::COMMAND_TEST_FUNC), data, reply, option);\n"
        "    if (FAILED(result)) {\n"
        "        return result;\n"
        "    }\n"
        "    return ERR_OK;\n"
        "}\n";
    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncSpace_001 end" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncComma_001
 * @tc.desc: HasAsyncMessageOption boundary: TF_ASYNC followed by comma.
 * @tc.type: FUNC
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncComma_001, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncComma_001 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    emitter->logOn_ = false;
    emitter->hitraceOn_ = false;
    emitter->interface_ = new ASTInterfaceType();
    emitter->interface_->SetName("IFoo");
    emitter->SetOverloadName("");

    AutoPtr<ASTMethod> method = new ASTMethod();
    method->SetName("test_func");
    method->SetReturnType(new ASTVoidType());
    method->attr_ = new ASTAttr(ASTAttr::ONEWAY | ASTAttr::CUSTOM_MSG_OPTION);
    method->SetMessageOption("MessageOption::TF_ASYNC, MessageOption::TF_WAIT_TIME");

    emitter->EmitInterfaceProxyMethodBody(method, sb, prefix);
    std::string expectedCode =
        "{\n"
        "    MessageParcel data;\n"
        "    MessageParcel reply;\n"
        "    MessageOption option(MessageOption::TF_ASYNC, MessageOption::TF_WAIT_TIME);\n"
        "\n"
        "    if (!data.WriteInterfaceToken(GetDescriptor())) {\n"
        "        return ERR_INVALID_VALUE;\n"
        "    }\n"
        "\n"
        "    sptr<IRemoteObject> remote = Remote();\n"
        "    if (!remote) {\n"
        "        return ERR_INVALID_DATA;\n"
        "    }\n"
        "    int32_t result = remote->SendRequest(\n"
        "        static_cast<uint32_t>(IFooIpcCode::COMMAND_TEST_FUNC), data, reply, option);\n"
        "    if (FAILED(result)) {\n"
        "        return result;\n"
        "    }\n"
        "    return ERR_OK;\n"
        "}\n";
    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncComma_001 end" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncPipe_001
 * @tc.desc: HasAsyncMessageOption boundary: TF_ASYNC followed by pipe.
 * @tc.type: FUNC
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncPipe_001, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncPipe_001 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    emitter->logOn_ = false;
    emitter->hitraceOn_ = false;
    emitter->interface_ = new ASTInterfaceType();
    emitter->interface_->SetName("IFoo");
    emitter->SetOverloadName("");

    AutoPtr<ASTMethod> method = new ASTMethod();
    method->SetName("test_func");
    method->SetReturnType(new ASTVoidType());
    method->attr_ = new ASTAttr(ASTAttr::ONEWAY | ASTAttr::CUSTOM_MSG_OPTION);
    method->SetMessageOption("MessageOption::TF_ASYNC|MessageOption::TF_ASYNC_WAKEUP_LATER");

    emitter->EmitInterfaceProxyMethodBody(method, sb, prefix);
    std::string expectedCode =
        "{\n"
        "    MessageParcel data;\n"
        "    MessageParcel reply;\n"
        "    MessageOption option(MessageOption::TF_ASYNC|MessageOption::TF_ASYNC_WAKEUP_LATER);\n"
        "\n"
        "    if (!data.WriteInterfaceToken(GetDescriptor())) {\n"
        "        return ERR_INVALID_VALUE;\n"
        "    }\n"
        "\n"
        "    sptr<IRemoteObject> remote = Remote();\n"
        "    if (!remote) {\n"
        "        return ERR_INVALID_DATA;\n"
        "    }\n"
        "    int32_t result = remote->SendRequest(\n"
        "        static_cast<uint32_t>(IFooIpcCode::COMMAND_TEST_FUNC), data, reply, option);\n"
        "    if (FAILED(result)) {\n"
        "        return result;\n"
        "    }\n"
        "    return ERR_OK;\n"
        "}\n";
    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncPipe_001 end" << std::endl;
}

/*
 * @tc.name: EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncParen_001
 * @tc.desc: HasAsyncMessageOption boundary: TF_ASYNC followed by right paren.
 * @tc.type: FUNC
 */
HWTEST_F(SaCppClientProxyCodeEmitterTest, EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncParen_001, Level1)
{
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncParen_001 begin" << std::endl;
    StringBuilder sb;
    std::string prefix;
    AutoPtr<SaCppClientProxyCodeEmitter> emitter = new SaCppClientProxyCodeEmitter;
    emitter->logOn_ = false;
    emitter->hitraceOn_ = false;
    emitter->interface_ = new ASTInterfaceType();
    emitter->interface_->SetName("IFoo");
    emitter->SetOverloadName("");

    AutoPtr<ASTMethod> method = new ASTMethod();
    method->SetName("test_func");
    method->SetReturnType(new ASTVoidType());
    method->attr_ = new ASTAttr(ASTAttr::ONEWAY | ASTAttr::CUSTOM_MSG_OPTION);
    method->SetMessageOption("MessageOption::TF_ASYNC)");

    emitter->EmitInterfaceProxyMethodBody(method, sb, prefix);
    std::string expectedCode =
        "{\n"
        "    MessageParcel data;\n"
        "    MessageParcel reply;\n"
        "    MessageOption option(MessageOption::TF_ASYNC));\n"
        "\n"
        "    if (!data.WriteInterfaceToken(GetDescriptor())) {\n"
        "        return ERR_INVALID_VALUE;\n"
        "    }\n"
        "\n"
        "    sptr<IRemoteObject> remote = Remote();\n"
        "    if (!remote) {\n"
        "        return ERR_INVALID_DATA;\n"
        "    }\n"
        "    int32_t result = remote->SendRequest(\n"
        "        static_cast<uint32_t>(IFooIpcCode::COMMAND_TEST_FUNC), data, reply, option);\n"
        "    if (FAILED(result)) {\n"
        "        return result;\n"
        "    }\n"
        "    return ERR_OK;\n"
        "}\n";
    EXPECT_STREQ(sb.buffer_, expectedCode.c_str());
    DTEST_LOG << "EmitInterfaceProxyMethodBody_OnewayCustomMsgOptAsyncParen_001 end" << std::endl;
}
} // namespace OHOS::Idl
