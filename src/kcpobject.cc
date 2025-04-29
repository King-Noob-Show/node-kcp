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

#include "kcpobject.h"
#include <napi.h>
#include <cstring>

#define RECV_BUFFER_SIZE 4096

namespace node_kcp {

int KCPObject::KcpOutput(const char* buf, int len, ikcpcb* kcp, void* user) {
    KCPObject* thiz = (KCPObject*)user;
    if (thiz->output.IsEmpty()) {
        return len;
    }

    Napi::Env env = thiz->Env();
    Napi::HandleScope scope(env);

    Napi::Buffer<char> buffer = Napi::Buffer<char>::Copy(env, buf, len);
    Napi::Number size = Napi::Number::New(env, len);

    if (thiz->context.IsEmpty()) {
        thiz->output.Call({buffer, size});
    } else {
        thiz->output.Call({buffer, size, thiz->context.Value()});
    }
    return len;
}

KCPObject::KCPObject(const Napi::CallbackInfo& info) : Napi::ObjectWrap<KCPObject>(info),
    recvBuff(nullptr), recvBuffSize(RECV_BUFFER_SIZE) {
    Napi::Env env = info.Env();

    if (!info[0].IsNumber() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "kcp.KCP requires conv (number) and token (number) arguments")
            .ThrowAsJavaScriptException();
        return;
    }

    uint32_t conv = info[0].As<Napi::Number>().Uint32Value();
    uint32_t token = info[1].As<Napi::Number>().Uint32Value();

    kcp = ikcp_create(conv, token, this);
    kcp->output = KcpOutput;
    recvBuff = (char*)malloc(recvBuffSize);

    if (info.Length() > 2 && info[2].IsObject()) {
        context.Reset(info[2].As<Napi::Object>(), 1);
    }
}

KCPObject::~KCPObject() {
    if (kcp) {
        ikcp_release(kcp);
        kcp = nullptr;
    }
    if (recvBuff) {
        free(recvBuff);
        recvBuff = nullptr;
    }
}

Napi::Object KCPObject::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "KCP", {
        InstanceMethod("release", &KCPObject::Release),
        InstanceMethod("context", &KCPObject::GetContext),
        InstanceMethod("recv", &KCPObject::Recv),
        InstanceMethod("send", &KCPObject::Send),
        InstanceMethod("input", &KCPObject::Input),
        InstanceMethod("output", &KCPObject::Output),
        InstanceMethod("update", &KCPObject::Update),
        InstanceMethod("check", &KCPObject::Check),
        InstanceMethod("flush", &KCPObject::Flush),
        InstanceMethod("peeksize", &KCPObject::Peeksize),
        InstanceMethod("setmtu", &KCPObject::Setmtu),
        InstanceMethod("wndsize", &KCPObject::Wndsize),
        InstanceMethod("waitsnd", &KCPObject::Waitsnd),
        InstanceMethod("nodelay", &KCPObject::Nodelay),
        InstanceMethod("stream", &KCPObject::Stream)
    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();

    exports.Set("KCP", func);
    return exports;
}

Napi::Value KCPObject::NewInstance(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected two number arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Create new instance
    Napi::Object obj = constructor.New({
        info[0],  // conv
        info[1]   // token
    });

    return obj;
}

Napi::Value KCPObject::GetContext(const Napi::CallbackInfo& info) {
    if (!context.IsEmpty()) {
        return context.Value();
    }
    return info.Env().Undefined();
}

Napi::Value KCPObject::Release(const Napi::CallbackInfo& info) {
    if (kcp) {
        ikcp_release(kcp);
        kcp = nullptr;
    }
    if (recvBuff) {
        free(recvBuff);
        recvBuff = nullptr;
    }
    return info.Env().Undefined();
}

Napi::Value KCPObject::Recv(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    int bufsize = 0;
    unsigned int allsize = 0;
    int buflen = 0;
    int len = 0;

    while (1) {
        bufsize = ikcp_peeksize(kcp);
        if (bufsize <= 0) {
            break;
        }
        allsize += bufsize;
        if (allsize > recvBuffSize) {
            int align = allsize % 4;
            if (align) {
                allsize += 4 - align;
            }
            recvBuffSize = allsize;
            recvBuff = (char*)realloc(recvBuff, recvBuffSize);
            if (!recvBuff) {
                Napi::Error::New(env, "realloc error").ThrowAsJavaScriptException();
                len = 0;
                break;
            }
        }

        buflen = ikcp_recv(kcp, recvBuff + len, bufsize);
        if (buflen <= 0) {
            break;
        }
        len += buflen;
        if (kcp->stream == 0) {
            break;
        }
    }

    if (len > 0) {
        return Napi::Buffer<char>::Copy(env, recvBuff, len);
    }
    return env.Undefined();
}

Napi::Value KCPObject::Input(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        return env.Undefined();
    }

    if (info[0].IsString()) {
        std::string str = info[0].As<Napi::String>();
        int len = str.length();
        if (len == 0) {
            Napi::Error::New(env, "Input string error").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        int t = ikcp_input(kcp, str.c_str(), len);
        return Napi::Number::New(env, t);
    } else if (info[0].IsBuffer()) {
        Napi::Buffer<char> buffer = info[0].As<Napi::Buffer<char>>();
        int len = buffer.Length();
        if (len == 0) {
            return env.Undefined();
        }
        char* buf = buffer.Data();
        int t = ikcp_input(kcp, buf, len);
        return Napi::Number::New(env, t);
    }
    return env.Undefined();
}

Napi::Value KCPObject::Send(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        return env.Undefined();
    }

    if (info[0].IsString()) {
        std::string str = info[0].As<Napi::String>();
        int len = str.length();
        if (len == 0) {
            Napi::Error::New(env, "Send string error").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        int t = ikcp_send(kcp, str.c_str(), len);
        return Napi::Number::New(env, t);
    } else if (info[0].IsBuffer()) {
        Napi::Buffer<char> buffer = info[0].As<Napi::Buffer<char>>();
        int len = buffer.Length();
        if (len == 0) {
            return env.Undefined();
        }
        char* buf = buffer.Data();
        int t = ikcp_send(kcp, buf, len);
        return Napi::Number::New(env, t);
    }
    return env.Undefined();
}

Napi::Value KCPObject::Output(const Napi::CallbackInfo& info) {
    if (!info[0].IsFunction()) {
        return info.Env().Undefined();
    }
    output.Reset(info[0].As<Napi::Function>(), 1);
    return info.Env().Undefined();
}

Napi::Value KCPObject::Update(const Napi::CallbackInfo& info) {
    if (!info[0].IsNumber()) {
        Napi::TypeError::New(info.Env(), "KCP update first argument must be number")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }
    int64_t arg0 = info[0].As<Napi::Number>().Int64Value();
    IUINT32 current = (IUINT32)(arg0 & 0xfffffffful);
    ikcp_update(kcp, current);
    return info.Env().Undefined();
}

Napi::Value KCPObject::Check(const Napi::CallbackInfo& info) {
    if (!info[0].IsNumber()) {
        Napi::TypeError::New(info.Env(), "KCP check first argument must be number")
            .ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }

    int64_t arg0 = info[0].As<Napi::Number>().Int64Value();
    IUINT32 current = (IUINT32)(arg0 & 0xfffffffful);
    IUINT32 ret = ikcp_check(kcp, current) - current;
    return Napi::Number::New(info.Env(), (uint32_t)(ret > 0 ? ret : 0));
}

Napi::Value KCPObject::Flush(const Napi::CallbackInfo& info) {
    ikcp_flush(kcp);
    return info.Env().Undefined();
}

Napi::Value KCPObject::Peeksize(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), ikcp_peeksize(kcp));
}

Napi::Value KCPObject::Setmtu(const Napi::CallbackInfo& info) {
    int mtu = 1400;
    if (info[0].IsNumber()) {
        mtu = info[0].As<Napi::Number>().Int32Value();
    }
    return Napi::Number::New(info.Env(), ikcp_setmtu(kcp, mtu));
}

Napi::Value KCPObject::Wndsize(const Napi::CallbackInfo& info) {
    int sndwnd = 32;
    int rcvwnd = 32;
    if (info[0].IsNumber()) {
        sndwnd = info[0].As<Napi::Number>().Int32Value();
    }
    if (info[1].IsNumber()) {
        rcvwnd = info[1].As<Napi::Number>().Int32Value();
    }
    return Napi::Number::New(info.Env(), ikcp_wndsize(kcp, sndwnd, rcvwnd));
}

Napi::Value KCPObject::Waitsnd(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), ikcp_waitsnd(kcp));
}

Napi::Value KCPObject::Nodelay(const Napi::CallbackInfo& info) {
    int nodelay = 0;
    int interval = 100;
    int resend = 0;
    int nc = 0;
    if (info[0].IsNumber()) {
        nodelay = info[0].As<Napi::Number>().Int32Value();
    }
    if (info[1].IsNumber()) {
        interval = info[1].As<Napi::Number>().Int32Value();
    }
    if (info[2].IsNumber()) {
        resend = info[2].As<Napi::Number>().Int32Value();
    }
    if (info[3].IsNumber()) {
        nc = info[3].As<Napi::Number>().Int32Value();
    }
    return Napi::Number::New(info.Env(), ikcp_nodelay(kcp, nodelay, interval, resend, nc));
}

Napi::Value KCPObject::Stream(const Napi::CallbackInfo& info) {
    if (info[0].IsNumber()) {
        int stream = info[0].As<Napi::Number>().Int32Value();
        kcp->stream = stream;
        return Napi::Number::New(info.Env(), kcp->stream);
    }
    return info.Env().Undefined();
}

} // namespace node_kcp