/*
  @DO $EXE$ -s hlsl -f main -p ps_5_0 --api dx12 $PATH$
  @DO rm -rf *.asm
  @END
*/

#define MyRS1 "DescriptorTable(SRV(t0))," \
              "StaticSampler(s0, addressU = TEXTURE_ADDRESS_CLAMP, " \
                                "filter = FILTER_MIN_MAG_MIP_LINEAR )"
                                 
Texture2D<float4> tx : register(t0);
sampler SS : register(s0);

[RootSignature(MyRS1)]
float4 main( float4 uv : uv ) : SV_Target
{
   return tx.Sample( SS, uv );
}
