
cbuffer xgl_system : register( b0 )
{
	float4x4 g_xgl_view;               // Offset:    0 Size:    64 [unused]
    float4x4 g_xgl_view_inverse;       // Offset:   64 Size:    64 [unused]
    float4x4 g_xgl_projection;         // Offset:  128 Size:    64 [unused]
    float4x4 g_xgl_view_projection;    // Offset:  192 Size:    64 [unused]
    float4x4 g_xgl_view_projection_inverse;// Offset:  256 Size:    64 [unused]
    uint2 g_xgl_target_dimension;      // Offset:  320 Size:     8 [unused]
    float g_xgl_time;                  // Offset:  328 Size:     4 [unused]
//      = 0x00000000 
    int g_xgl_id;                      // Offset:  332 Size:     4
//      = 0x00000000 
    float3 g_xgl_light_vector0;        // Offset:  336 Size:    12 [unused]
//      = 0x3f333333 0xbf333333 0xbf333333 
    float3 g_xgl_light_color0;         // Offset:  352 Size:    12 [unused]
//      = 0x3f800000 0x3f800000 0x3f800000 
    float3 g_xgl_light_specular_color0;// Offset:  368 Size:    12 [unused]
//      = 0x3f800000 0x3f800000 0x3f800000 
    float3 g_xgl_light_vector1;        // Offset:  384 Size:    12 [unused]
//      = 0xbf333333 0x3f333333 0x3f333333 
    float3 g_xgl_light_color1;         // Offset:  400 Size:    12 [unused]
//      = 0x3e4ccccd 0x3e4ccccd 0x3e4ccccd 
    float3 g_xgl_light_specular_color1;// Offset:  416 Size:    12 [unused]
//      = 0x3f800000 0x3f800000 0x3f800000 
    float3 g_xgl_light_vector2;        // Offset:  432 Size:    12 [unused]
//      = 0x00000000 0x00000000 0x00000000 
    float3 g_xgl_light_color2;         // Offset:  448 Size:    12 [unused]
//      = 0x00000000 0x00000000 0x00000000 
    float3 g_xgl_light_specular_color2;// Offset:  464 Size:    12 [unused]
//      = 0x3f800000 0x3f800000 0x3f800000 
    float3 g_xgl_light_vector3;        // Offset:  480 Size:    12 [unused]
//      = 0x00000000 0x00000000 0x00000000 
    float3 g_xgl_light_color3;         // Offset:  496 Size:    12 [unused]
//      = 0x00000000 0x00000000 0x00000000 
    float3 g_xgl_light_specular_color3;// Offset:  512 Size:    12 [unused]
//      = 0x00000000 0x00000000 0x00000000 
    float3 g_xgl_ambient_color;        // Offset:  528 Size:    12 [unused]
//      = 0x3e4ccccd 0x3e4ccccd 0x3e4ccccd 
    float g_pad2;                      // Offset:  540 Size:     4 [unused]
    float4 g_xgl_fog_fade_param;       // Offset:  544 Size:    16 [unused]
//      = 0x00000000 0x00000000 0x00000000 0x00000000 
    float4 g_xgl_fog_color;            // Offset:  560 Size:    16 [unused]
//      = 0x3f800000 0x3f800000 0x3f800000 0x00000000 
}

// cb 2
cbuffer xgl_user_param : register( b2 )
{
	float3 diffuse;                    // Offset:    0 Size:    12
//      = 0x3f800000 0x3f800000 0x3f800000 
    float2 normal_amount;              // Offset:   16 Size:     8
//      = 0x3f800000 0x3f800000 
    float specular_pow;                // Offset:   24 Size:     4
//      = 0x42c80000 
    float3 specular_color;             // Offset:   32 Size:    12
//      = 0x3f800000 0x3f800000 0x3f800000 
    float light_smooth;                // Offset:   44 Size:     4
//      = 0x00000000 
    float3 light_color;                // Offset:   48 Size:    12
//      = 0x40800000 0x3f000000 0x3f000000 
    float metallic;                    // Offset:   60 Size:     4
//      = 0x00000000 
    float roughness;                   // Offset:   64 Size:     4
//      = 0x00000000 
    float fall_off_amount;             // Offset:   68 Size:     4
//      = 0x3f800000 
    float fall_off_scale;              // Offset:   72 Size:     4
//      = 0x3f800000 
    float fall_off_offset;             // Offset:   76 Size:     4
//      = 0x00000000 
}

Texture2D albedo;
Texture2D normal;
Texture2D param_reflect_spec_lightmask_hlightmask;

SamplerState albedo_sampler 
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 eye : EYE_VECTOR;
    float3 view : VIEW_POSITION;
    float3 normal : NORMAL;
    float3 binormal : BINORMAL;
    float3 tangent : TANGENT;
    float2 texcoord0 : TEXCOORD0;
};

struct PS_OUTPUT
{
    float4 out0 : SV_TARGET0;
    float4 out1 : SV_TARGET1;
    float4 out2 : SV_TARGET2;
    float4 out3 : SV_TARGET3;
    float4 out4 : SV_TARGET4;
    float4 out5 : SV_TARGET5;
    float4 out7 : SV_TARGET7;
};

PS_OUTPUT main(PS_INPUT In)
{
    PS_OUTPUT v_out;
    float4 light = albedo.Sample(albedo_sampler, In.texcoord0);
    float4 color;
    color.xyz = light_color;
    color.w = light.x;

    v_out.out0 = color;
    v_out.out1 = color;
    v_out.out2 = color;
	v_out.out3 = color;
	v_out.out4 = color;
	v_out.out5.xyz = In.view;
    v_out.out5.w = g_xgl_id;
	v_out.out7 = color;

	return v_out;
}