{
    "targets": [
        {
            "target_name": "kcp",
            "defines": [
                "NAPI_VERSION=8",
                "NODE_ADDON_API_CPP_EXCEPTIONS"
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")"
            ],
            "dependencies": [
                "<!(node -p \"require('node-addon-api').gyp\")"
            ],
            "sources": [
                "src/kcp/ikcp.c",
                "src/kcpobject.cc",
                "src/node-kcp.cc"
            ],
            "cflags": ["-std=c++17"],
            "xcode_settings": {
                "OTHER_CPLUSPLUSFLAGS": ["-std=c++17"]
            },
            "msvs_settings": {
                "VCCLCompilerTool": {
                    "ExceptionHandling": 1,
                    "AdditionalOptions": ["/EHsc"]
                }
            }
        }
    ]
}