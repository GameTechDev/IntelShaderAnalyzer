/*===================== begin_copyright_notice ==================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019, Intel Corporation
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of
// the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

======================= end_copyright_notice ==================================*/
/************************************************************************/
// This header provide an interface to call into Intel shader compiler for shaders in DXBC format
/************************************************************************/
#pragma once
namespace IntelGPUCompiler
{
typedef void* OpaqueCompiler;
typedef void* OpaqueShader;

// Platform supported by the library may be queried with pfnEnumPlatforms callback
enum class Platform {
    SKL, //Skylake
    KBL, //Kabylake
    ICLLP, //Icelake LP
};

struct PlatformInfo
{
    Platform Identifier;
    const char* platformName;
};

// Max version support in the header
static const unsigned int g_HeaderVersion = 1;

struct ShaderInput_DX11_V1
{
    const void* DXBCBin = nullptr;
};

struct ShaderInput_DX12_V1
{
    const void* DXBCBin       = nullptr;
    const void* rootSignature = nullptr;
    size_t rootSignatureSize = 0;
};

// DX11 callbacks
typedef bool(__stdcall* PFNCREATESHADERCOMPILER_DX11_V1)(Platform, OpaqueCompiler&);
typedef void(__stdcall* PFNDELETESHADERCOMPILER_DX11)(OpaqueCompiler&);
typedef bool(__stdcall* PFNCREATESHADER_DX11_V1)(OpaqueCompiler, const ShaderInput_DX11_V1& input, OpaqueShader& output);
typedef void(__stdcall* PFNDELETESHADER_DX11)(OpaqueShader& shader);

// DX12 callbacks
typedef bool(__stdcall* PFNCREATESHADERCOMPILER_DX12_V1)(Platform, OpaqueCompiler&);
typedef void(__stdcall* PFNDELETESHADERCOMPILER_DX12)(OpaqueCompiler&);
typedef bool(__stdcall* PFNCREATESHADER_DX12_V1)(OpaqueCompiler, const ShaderInput_DX12_V1& input, OpaqueShader& output);
typedef void(__stdcall* PFNDELETESHADER_DX12)(OpaqueShader& shader);


// common callbacks
typedef char*(__stdcall*  PFNGETISATEXT)(OpaqueShader, size_t& textSize);
typedef void*(__stdcall*  PFNGETISABINARY)(OpaqueShader, size_t& shaderSize);
typedef const char*(__stdcall* PFNGETLASTERROR)();
typedef size_t(__stdcall*  PFNENUMPLATFORMS)(PlatformInfo* pPlatforms, size_t nMaxPlatforms);


struct SFunctionTable_V1
{
    struct DX11Functions
    {
        // DX11 callbacks
        PFNCREATESHADERCOMPILER_DX11_V1 pfnCreateCompiler;
        PFNDELETESHADERCOMPILER_DX11    pfnDeleteShaderCompiler;
        PFNCREATESHADER_DX11_V1         pfnCreateShader;
        PFNDELETESHADER_DX11            pfnDeleteShader;
    } pfnDX11;
    struct DX12Functions
    {
        // DX12 callbacks
        PFNCREATESHADERCOMPILER_DX12_V1 pfnCreateCompiler;
        PFNDELETESHADERCOMPILER_DX12    pfnDeleteShaderCompiler;
        PFNCREATESHADER_DX12_V1         pfnCreateShader;
        PFNDELETESHADER_DX12            pfnDeleteShader;
    } pfnDX12;

    PFNENUMPLATFORMS pfnEnumPlatforms;
    PFNGETISATEXT   pfnGetIsaText;
    PFNGETISABINARY pfnGetIsaBinary;
    PFNGETLASTERROR pfnGetLastError;
};

struct SFunctionTable
{
    union 
    {
        SFunctionTable_V1 interface1;
    };
};

struct SOpenCompiler
{
    unsigned int       InterfaceVersion = g_HeaderVersion;
    SFunctionTable* pCompilerFuncs;
};

/*
The way to use this library is to call OpenCompiler with the interface version

    /// Load library
    HINSTANCE compilerInstance = (HINSTANCE)LoadLibrary("IntelGpuCompiler64.dll");
    PFNOPENCOMPILER pfnOpenCompiler = (PFNOPENCOMPILER)GetProcAddress(compilerInstance, g_cOpenCompilerFnName);
    SFunctionTable functionTable;
    SOpenCompiler desc;
    desc.pCompilerFuncs = &functionTable;

    /// get the right set of callbacks
    if(pfnOpenCompiler(desc))
    {
        /// create compiler context
        OpaqueCompiler pCompiler;
        functionTable.interface1.pfnDX11.pfnCreateCompiler(Platform::KBL, pCompiler);

        /// CreateShader
        ShaderInput_DX11_V1 input;
        input.DXBCBin = pShader;
        OpaqueShader output;
        functionTable.interface1.pfnDX11.pfnCreateShader(pCompiler, input, output);
        const char* isaText = functionTable.interface1.pfnGetIsaText(output);
        printf("%s", isaText);
        /// free up memory
        functionTable.interface1.pfnDX11.pfnDeleteShader(output);
        functionTable.interface1.pfnDX11.pfnDeleteShaderCompiler(pCompiler);
    }

*/

static const char* g_cOpenCompilerFnName = "OpenCompiler";

// auto pfnOpenCompiler = (PFNOPENCOMPILER)GetProcAddress( igcLib, g_cOpenCompilerFnName );
typedef bool(__stdcall* PFNOPENCOMPILER)(SOpenCompiler&);

}
