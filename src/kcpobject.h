/**
 * Copyright 2021 leenjewel
 *
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

#ifndef KCPOBJECT_H
#define KCPOBJECT_H

#include <napi.h>
#include "kcp/ikcp.h"

namespace node_kcp {

class KCPObject : public Napi::ObjectWrap<KCPObject> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    KCPObject(const Napi::CallbackInfo& info);
    ~KCPObject();

    inline static Napi::FunctionReference constructor;

    static Napi::Value NewInstance(const Napi::CallbackInfo& info);

private:
    static int KcpOutput(const char* buf, int len, ikcpcb* kcp, void* user);

    // Methods
    Napi::Value Release(const Napi::CallbackInfo& info);
    Napi::Value GetContext(const Napi::CallbackInfo& info);
    Napi::Value Recv(const Napi::CallbackInfo& info);
    Napi::Value Send(const Napi::CallbackInfo& info);
    Napi::Value Input(const Napi::CallbackInfo& info);
    Napi::Value Output(const Napi::CallbackInfo& info);
    Napi::Value Update(const Napi::CallbackInfo& info);
    Napi::Value Check(const Napi::CallbackInfo& info);
    Napi::Value Flush(const Napi::CallbackInfo& info);
    Napi::Value Peeksize(const Napi::CallbackInfo& info);
    Napi::Value Setmtu(const Napi::CallbackInfo& info);
    Napi::Value Wndsize(const Napi::CallbackInfo& info);
    Napi::Value Waitsnd(const Napi::CallbackInfo& info);
    Napi::Value Nodelay(const Napi::CallbackInfo& info);
    Napi::Value Stream(const Napi::CallbackInfo& info);

    ikcpcb* kcp;
    Napi::FunctionReference output;
    Napi::ObjectReference context;
    char* recvBuff;
    unsigned int recvBuffSize;
};

} // namespace node_kcp

#endif