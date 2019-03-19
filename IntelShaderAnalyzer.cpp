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

#define _CRT_SECURE_NO_WARNINGS

#include "IntelShaderAnalyzer.h"

#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <d3dcompiler.h>

#ifdef _WIN64
#define DLL_NAME  "IntelGpuCompiler64.dll"
#else
#define DLL_NAME  "IntelGpuCompiler32.dll"
#endif

using namespace IntelGPUCompiler;



int sizeOfFile(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == -1)
        return -1;
    return st.st_size;
}

bool readAllBytes( std::vector<uint8_t>& bytes, const char * filename )
{
    size_t length = sizeOfFile( filename );
    if ( length == -1 )
        return false;

    std::ifstream is(filename, std::ifstream::binary);
    
    bytes.resize( length );
    is.read((char*)bytes.data(), length);

    return true;
}

bool readAllText( std::string& text,const char * filename )
{
    std::ifstream file( filename );
    if ( !file.good() )
        return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    text = std::move( buffer.str() );
    return true;
}


class API
{
public:
    virtual bool CanRun( ToolInputs& opts ) = 0;
    virtual bool CreateCompiler( Platform platformID,SFunctionTable& functionTable,OpaqueCompiler& compiler ) = 0;    
    virtual bool CreateShader( SFunctionTable& functionTable,OpaqueCompiler& compiler,OpaqueShader& output,ToolInputs& opts ) = 0;
    virtual void DeleteShader( SFunctionTable& functionTable,OpaqueShader& shader ) = 0;
    virtual void DeleteCompiler( SFunctionTable& functionTable,OpaqueCompiler& compiler ) = 0;
};

class API_DX11 : public API
{
public:
    virtual bool CanRun( ToolInputs& opts ) override
    {
        if ( opts.bytecode.empty() )
        {
            printf( "Missing shader bytecode\n" );
            return false;
        }
        return true;
    }

    virtual bool CreateCompiler( Platform platformID, SFunctionTable& functionTable,OpaqueCompiler& compiler ) override
    {
        return functionTable.interface1.pfnDX11.pfnCreateCompiler( platformID,compiler );
    }

    virtual bool CreateShader( SFunctionTable& functionTable, OpaqueCompiler& compiler, OpaqueShader& output,ToolInputs& opts ) override
    {
        ShaderInput_DX11_V1 input;
        input.DXBCBin = (void*)opts.bytecode.data();
        return functionTable.interface1.pfnDX11.pfnCreateShader( compiler,input,output );
    }

    virtual void DeleteCompiler( SFunctionTable& functionTable, OpaqueCompiler& compiler ) override
    {
        functionTable.interface1.pfnDX11.pfnDeleteShaderCompiler( compiler );
    }

    virtual void DeleteShader( SFunctionTable& functionTable,OpaqueShader& shader ) override
    {
        functionTable.interface1.pfnDX11.pfnDeleteShader( shader );
    }
};

class API_DX12 : public API
{
public:
    virtual bool CanRun( ToolInputs& opts ) override
    {
        if ( opts.bytecode.empty() )
        {
            printf( "Missing shader bytecode\n" );
            return false;
        }
        if ( opts.rootsig.empty() )
        {
            printf( "Missing root signature\n" );
            return false;
        }
        return true;
    }

    virtual bool CreateCompiler( Platform platformID,SFunctionTable& functionTable,OpaqueCompiler& compiler ) override
    {
        return functionTable.interface1.pfnDX12.pfnCreateCompiler( platformID,compiler );
    }

    virtual bool CreateShader( SFunctionTable& functionTable,OpaqueCompiler& compiler,OpaqueShader& output,ToolInputs& opts ) override
    {
        ShaderInput_DX12_V1 input;
        input.DXBCBin = (void*)opts.bytecode.data();
        input.rootSignature = (void*)opts.rootsig.data();
        input.rootSignatureSize = opts.rootsig.size();
        return functionTable.interface1.pfnDX12.pfnCreateShader( compiler,input,output );
    }

    virtual void DeleteCompiler( SFunctionTable& functionTable,OpaqueCompiler& compiler ) override
    {
        functionTable.interface1.pfnDX12.pfnDeleteShaderCompiler( compiler );
    }

    virtual void DeleteShader( SFunctionTable& functionTable,OpaqueShader& shader ) override
    {
        functionTable.interface1.pfnDX12.pfnDeleteShader( shader );
    }
};




bool RunTool( SFunctionTable& functionTable, ToolInputs& opts, API& api )
{
    if ( !api.CanRun( opts ) )
        return false;

    for ( PlatformInfo& platform : opts.asics )
    {
        /// create compiler context
        OpaqueCompiler pCompiler;
        if ( !api.CreateCompiler( platform.Identifier,functionTable,pCompiler ) )
        {
            printf( "ERROR: %s\n",functionTable.interface1.pfnGetLastError(  ) );
            return false;
        }

        OpaqueShader output;
        if( api.CreateShader( functionTable, pCompiler, output, opts ) )
        {
            size_t isaSize = 0;
            const char* isaText = functionTable.interface1.pfnGetIsaText( output,isaSize );
            if ( isaText )
            {
                std::stringstream isaFile;
                if ( opts.isa_prefix )
                    isaFile << opts.isa_prefix;

                isaFile << platform.platformName << ".asm";

                std::string isaFileName = isaFile.str();

                FILE* fp = fopen( isaFileName.c_str(), "w" );
                if ( fp )
                {
                    fprintf( fp,"%s",isaText );
                    fclose( fp );
                }
                else
                {
                    printf( "Failed to open output file: %s\n",isaFileName.c_str() );
                    return false;
                }
            }
            else
            {
                printf( "ERROR: %s\n", functionTable.interface1.pfnGetLastError(  ) );
                return false;
            }

            /// free up memory
            api.DeleteShader( functionTable,output );
        }
        else
        {
            printf( "ERROR: %s\n", functionTable.interface1.pfnGetLastError(  ) );
            return false;
        }

        api.DeleteCompiler( functionTable,pCompiler );
    }

    return true;
}

void GetAsicList( SFunctionTable& functionTable, std::vector< IntelGPUCompiler::PlatformInfo >& asics )
{    
    size_t nPlatforms = functionTable.interface1.pfnEnumPlatforms( nullptr,0 );
    asics.resize( nPlatforms );
    functionTable.interface1.pfnEnumPlatforms( asics.data(),nPlatforms );   
}

bool ListAsics()
{
    HINSTANCE compilerInstance = (HINSTANCE)LoadLibraryA( DLL_NAME );
    if ( !compilerInstance )
    {
        printf( "Failed to load: %s\n",DLL_NAME );
        return false;
    }

    PFNOPENCOMPILER pfnOpenCompiler = (PFNOPENCOMPILER)GetProcAddress( compilerInstance,g_cOpenCompilerFnName );
    if ( !pfnOpenCompiler )
    {
        printf( "GetProcAddress failed for: %s\n",g_cOpenCompilerFnName );
        return false;
    }

    SFunctionTable functionTable;
    SOpenCompiler desc;
    desc.InterfaceVersion = 1;
    desc.pCompilerFuncs = &functionTable;

    /// get the right set of callbacks
    if ( !pfnOpenCompiler( desc ) )
    {
        printf( "OpenCompiler failed\n" );
        return false;
    }

    std::vector< IntelGPUCompiler::PlatformInfo > asics;
    GetAsicList( functionTable,asics );
    
    for ( size_t i=0; i<asics.size(); i++ )
        printf( "%s\n", asics[i].platformName );   
    return true;
}

void ShowHelp()
{
    printf( "To compile hlsl use:  -s hlsl -p <profile> -f <function> <filename>\n" );
    printf( "To compile dxbc use:  -s dxbc  <filename>\n" );
    printf( "For details, read the readme\n" );
}




int main(int argc, char *argv[])
{
    std::vector<const char*> asicNames;
    
    const char* api           = "dx11";
    const char* rootsig_file  = nullptr;
    const char* source_lang   = "dxbc";

    ToolInputs opts;
    FrontendOptions frontend_opts;

    // parse the command line.  Where possible we have tried to match the syntax of AMD's RGA
    int i=1;
    while( i < argc )
    {
        if ( _stricmp( argv[i], "-l" ) == 0 ||
             _stricmp( argv[i], "--list-asics" ) == 0 )
        {
            if ( ListAsics() )
                return 0;
            else
                return 1;
        }
        else if ( _stricmp( argv[i], "-h" ) == 0 ||
                  _stricmp( argv[i], "--help" ) == 0 )
        {
            ShowHelp();
            return 0;
        }
        else if ( _stricmp( argv[i], "-c" ) == 0 ||
                  _stricmp( argv[i], "--asic" ) == 0 )
        {
            if ( i == argc-1 )
            {
                printf( "Missing argument for --asic\n" );
                return 1;
            }
            asicNames.push_back( argv[++i] );
        }
        else if ( _stricmp( argv[i],"--api" ) == 0 )
        {
            if ( i == argc-1 )
            {
                printf( "Missing argument for --api\n" );
                return 1;
            }
            api = argv[++i];
        }        
        else if ( _stricmp( argv[i],"--rootsig_file" ) == 0 )
        {
            if ( i == argc-1 )
            {
                printf( "Missing argument for --rootsig_file\n" );
                return 1;
            }
            rootsig_file = argv[++i];
        }
        else if ( _stricmp( argv[i],"--rootsig_profile" ) == 0 )
        {
            if ( i == argc-1 )
            {
                printf( "Missing argument for %s\n", argv[i] );
                return 1;
            }
            frontend_opts.rs_profile = argv[++i];
        }
        else if ( _stricmp( argv[i],"--rootsig_macro" ) == 0 )
        {
            if ( i == argc-1 )
            {
                printf( "Missing argument for %s\n",argv[i] );
                return 1;
            }
            frontend_opts.rs_macro = argv[++i];
        }
        else if ( _stricmp( argv[i],"--isa" ) == 0 )
        {
            if ( i == argc-1 )
            {
                printf( "Missing argument for --isa\n" );
                return 1;
            }
            opts.isa_prefix = argv[++i];
        }
        else if ( strcmp( argv[i],"-s" ) == 0 )
        {
            if ( i == argc-1 )
            {
                printf( "Missing argument for -s\n" );
                return 1;
            }
            source_lang = argv[++i];
        }
        else if ( strcmp( argv[i],"-D" ) == 0 )
        {
            if ( i == argc-1 )
            {
                printf( "Missing argument for %s\n", argv[i] );
                return 1;
            }

            char* def = argv[++i];
            char* value = def;
            while ( *value )
            {
                if ( *(value++) == '=' )
                {
                    value[-1] = '\0'; // replace '=' with null and stop scanning
                    break;
                }
            }

            frontend_opts.defines.push_back( std::pair<char*,char*>( def,value ) );
        }
        else if ( _stricmp( argv[i],"--profile" ) == 0 ||
                  _stricmp( argv[i], "-p" ) == 0 )
        {
            if ( i == argc-1 )
            {
                printf( "Missing argument for %s\n",argv[i] );
                return 1;
            }

            frontend_opts.profile = argv[++i];
        }
        else if ( _stricmp( argv[i],"--function" ) == 0 ||
                  _stricmp( argv[i],"-f" ) == 0 )
        {
            if ( i == argc-1 )
            {
                printf( "Missing argument for %s\n",argv[i] );
                return 1;
            }

            frontend_opts.entry = argv[++i];
        }
        else if ( _stricmp( argv[i], "--DXFlags" ) == 0 )
        {
            if ( i == argc-1 )
            {
                printf( "Missing argument for %s\n",argv[i] );
                return 1;
            }

            frontend_opts.dx_flags = strtoul( argv[++i], nullptr, 0 );
            
        }
        else if ( _stricmp( argv[i],"--DXLocation" ) == 0 )
        {
            if ( i == argc-1 )
            {
                printf( "Missing argument for %s\n",argv[i] );
                return 1;
            }
            frontend_opts.dx_location = argv[++i];
        }
        else if ( argv[i][0] == '-' )
        {
            printf( "Don't understand what: '%s' means\n",argv[i] );
            ShowHelp();
            return 1;
        }
        else
        {
            frontend_opts.input_file = argv[i];
        }

        ++i;
    }

    if ( frontend_opts.input_file == nullptr )
    {
        printf( "No input filename\n" );
        return 1;
    }

    
    if ( _stricmp( source_lang,"hlsl" ) == 0 )
    {
        if ( !readAllText( frontend_opts.input_text,frontend_opts.input_file ) )
        {
            printf( "Failed to read source from: %s\n",frontend_opts.input_file );
            return 1;
        }

        if ( frontend_opts.profile == nullptr )
        {
            printf( "Missing --profile\n" );
            return 1;
        }

        if ( !CompileHLSL( frontend_opts, opts ) )
            return 1;
    }
    else if ( _stricmp( source_lang,"dxbc" ) == 0 )
    {
        // load bytecode        
        if ( frontend_opts.input_file != nullptr )
        {
            if ( !readAllBytes( opts.bytecode, frontend_opts.input_file ) )
            {
                printf( "Unable to load bytecode from: %s\n", frontend_opts.input_file );
                return 1;
            }
        }

        // try to extract a root signature
        if( opts.rootsig.empty() )
        {
            if( !GetRootSignatureFromDXBC( frontend_opts, opts ) )
            {
                // failure here indicates DX compiler DLL problems
                //    diagnostics will happen elsewhere
                // Simply not having a root signature is considered success here...
                return 1;
            }
        }
    }
    else
    {
        printf( "Source language: '%s' not recognized\n",source_lang );
        return 1;
    }

    // load root signature if we're missing one
    if ( opts.rootsig.empty() )
    {
        if ( rootsig_file != nullptr )
        {
            if ( !readAllBytes( opts.rootsig, rootsig_file ) )
            {
                printf( "Unable to load root signature from: %s\n",rootsig_file );
                return 1;
            }
        }
    }

    // Load compiler DLL
    HINSTANCE compilerInstance = (HINSTANCE)LoadLibraryA( DLL_NAME );
    if ( !compilerInstance )
    {
        printf( "Failed to load: %s\n",DLL_NAME );
        return 1;
    }

    PFNOPENCOMPILER pfnOpenCompiler = (PFNOPENCOMPILER)GetProcAddress( compilerInstance,g_cOpenCompilerFnName );
    if ( !pfnOpenCompiler )
    {
        printf( "GetProcAddress failed for: %s\n",g_cOpenCompilerFnName );
        return 1;
    }


    SFunctionTable functionTable;
    SOpenCompiler desc;
    desc.InterfaceVersion = 1;
    desc.pCompilerFuncs = &functionTable;

    // get the right set of callbacks
    if ( !pfnOpenCompiler( desc ) )
    {
        printf( "OpenCompiler failed\n" );
        return 1;
    }

    // get list of supported asics
    GetAsicList( functionTable,opts.asics );

    // filter asic list down to the ones we've been asked to use.  By default use all of them
    if ( !asicNames.empty() )
    {
        size_t nAsicsToKeep=0;
        for ( size_t i=0; i<opts.asics.size(); i++ )
        {
            for ( const char* name : asicNames )
                if ( _stricmp( name,opts.asics[i].platformName ) == 0 )
                    opts.asics[nAsicsToKeep++] = opts.asics[i];
        }
        opts.asics.resize( nAsicsToKeep );
    }

    // run the tool
    if ( _stricmp( api,"dx11" ) == 0 )
    {
        API_DX11 api;
        if( !RunTool( functionTable, opts, api ) )
            return 1;
    }
    else if ( _stricmp( api,"dx12" ) == 0 )
    {
        API_DX12 api;
        if( !RunTool( functionTable, opts, api ) )
            return 1;
    }
    else
    {
        printf( "Unrecognized API: %s\n",api );
        return 1;
    }

    return 0;
}
