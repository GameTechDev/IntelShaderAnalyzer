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

#pragma once
#ifndef _INTEL_SHADER_ANALYZER_H_
#define _INTEL_SHADER_ANALYZER_H_

#include <vector>
#include "IntelGPUCompiler.h"

struct FrontendOptions
{
    std::vector< std::pair<const char*,const char*> > defines;
    std::string input_text;

    const char* profile         = nullptr;
    const char* entry           = "main";
    unsigned int dx_flags       = 0;
    const char* dx_location     = "d3dcompiler_47.dll";
    const char* input_file      = nullptr;
    const char* rs_macro        = nullptr;
    const char* rs_profile      = "rootsig_1_0";
};

struct ToolInputs
{
    std::vector<uint8_t> bytecode;
    std::vector<uint8_t> rootsig;
    const char* isa_prefix = "./isa_";
    std::vector< IntelGPUCompiler::PlatformInfo > asics;
};

bool CompileHLSL( FrontendOptions& opts, ToolInputs& inputs );
bool GetRootSignatureFromDXBC( FrontendOptions& frontend_opts, ToolInputs& inputs );

#endif