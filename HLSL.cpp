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

#include "IntelShaderAnalyzer.h"
#include <d3dcompiler.h>
#include <fstream>
#include <atlbase.h>

typedef HRESULT( WINAPI *D3DCOMPILE_FUNC )(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    LPCSTR pSourceName,
    const D3D_SHADER_MACRO *pDefines,
    ID3DInclude *pInclude,
    LPCSTR pEntrypoint,
    LPCSTR pTarget,
    UINT Flags1,
    UINT Flags2,
    ID3DBlob **ppCode,
    ID3DBlob **ppErrorMsgs);

typedef HRESULT( WINAPI *D3DGETBLOBPART_FUNC )(
    LPCVOID pShaderBytecode,
    SIZE_T BytecodeLength,
    UINT uBlobPartEnum,
    UINT uFlags,
    ID3DBlob **ppBlob
    );

bool GetRootSignatureFromDXBC( FrontendOptions& frontend_opts, ToolInputs& inputs  )
{
    HINSTANCE hCompiler = LoadLibraryA( frontend_opts.dx_location );
    if(!hCompiler)
    {
        printf( "Failed to load D3D compiler dll from: %s\n", frontend_opts.dx_location );
        return false;
    }

    D3DGETBLOBPART_FUNC pfnD3DGetBlobPart = (D3DGETBLOBPART_FUNC) GetProcAddress( hCompiler, "D3DGetBlobPart" );
    if(!pfnD3DGetBlobPart)
    {
        printf( "GetProcAddress failed for D3DGetBlobPart\n" );
        return false;
    }

    CComPtr<ID3DBlob> pEmbeddedRS;
    HRESULT hr = pfnD3DGetBlobPart( inputs.bytecode.data(), inputs.bytecode.size(), D3D_BLOB_ROOT_SIGNATURE, 0, &pEmbeddedRS );
    if(SUCCEEDED( hr ))
    {
        inputs.rootsig.resize( pEmbeddedRS->GetBufferSize() );
        memcpy( inputs.rootsig.data(), pEmbeddedRS->GetBufferPointer(), pEmbeddedRS->GetBufferSize() );
    }

    return true;
}


bool CompileHLSL( FrontendOptions& frontend_opts,ToolInputs& inputs )
{
    HINSTANCE hCompiler = LoadLibraryA( frontend_opts.dx_location );
    if ( !hCompiler )
    {
        printf( "Failed to load D3D compiler dll from: %s\n",frontend_opts.dx_location );
        return false;
    }

    D3DCOMPILE_FUNC pfnD3DCompile = (D3DCOMPILE_FUNC)GetProcAddress( hCompiler,"D3DCompile" );
    if ( !pfnD3DCompile )
    {
        printf( "GetProcAddress failed for D3DCompile\n" );
        return false;
    }
    D3DGETBLOBPART_FUNC pfnD3DGetBlobPart = (D3DGETBLOBPART_FUNC)GetProcAddress( hCompiler,"D3DGetBlobPart" );
    if ( !pfnD3DGetBlobPart )
    {
        printf( "GetProcAddress failed for D3DGetBlobPart\n" );
        return false;
    }

    std::vector<D3D_SHADER_MACRO> macros;
    for ( auto it : frontend_opts.defines )
    {
        D3D_SHADER_MACRO m;
        m.Definition = it.second;
        m.Name = it.first;
        macros.push_back( m );
    }

    macros.push_back( D3D_SHADER_MACRO{ nullptr,nullptr } );

    CComPtr<ID3DBlob> pCode;
    CComPtr<ID3DBlob> pMessages;
    HRESULT hr = pfnD3DCompile( frontend_opts.input_text.c_str(),
        frontend_opts.input_text.size(),
        frontend_opts.input_file,macros.data(),
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        frontend_opts.entry,
        frontend_opts.profile,
        frontend_opts.dx_flags,
        0,
        &pCode,
        &pMessages );

    if ( pMessages )
        printf( "%s\n",(char*)pMessages->GetBufferPointer() );        
    
    if ( FAILED( hr ) )
        return false;
    
    if ( pCode )
    {
        inputs.bytecode.resize( pCode->GetBufferSize() );
        memcpy( inputs.bytecode.data(),pCode->GetBufferPointer(),pCode->GetBufferSize() );

        // try to extract root signature if one is embedded
        CComPtr<ID3DBlob> pEmbeddedRS;
        hr = pfnD3DGetBlobPart( pCode->GetBufferPointer(), pCode->GetBufferSize(),D3D_BLOB_ROOT_SIGNATURE,0, &pEmbeddedRS );
        if ( SUCCEEDED( hr ) )
        {
            inputs.rootsig.resize( pEmbeddedRS->GetBufferSize() );
            memcpy( inputs.rootsig.data(), pEmbeddedRS->GetBufferPointer(), pEmbeddedRS->GetBufferSize() );
        }
        else if( frontend_opts.rs_macro && frontend_opts.rs_profile )
        {
            // try and compile a root signature using user-specified macro name
            CComPtr<ID3DBlob> pRS;
            CComPtr<ID3DBlob> pRSMessages;
            hr = pfnD3DCompile( frontend_opts.input_text.c_str(),
                frontend_opts.input_text.size(),
                frontend_opts.input_file,macros.data(),
                D3D_COMPILE_STANDARD_FILE_INCLUDE,
                frontend_opts.rs_macro,
                frontend_opts.rs_profile,
                frontend_opts.dx_flags,
                0,
                &pRS,
                &pRSMessages );

            if ( pRSMessages )
                printf( "%s\n",(char*)pRSMessages->GetBufferPointer() );

            if ( SUCCEEDED( hr ) )
            {
                inputs.rootsig.resize( pRS->GetBufferSize() );
                memcpy( inputs.rootsig.data(),pRS->GetBufferPointer(),pRS->GetBufferSize() );
            }
        }
    }

    return true;
}
