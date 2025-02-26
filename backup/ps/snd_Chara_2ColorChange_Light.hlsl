
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
	float3 diffuse = float3(1.0, 1.0, 1.0);
	float3 change_color0 = float3(1.0, 0.0, 0.0);
	float3 change_color1 = float3(0.0, 0.0, 1.0);
	float2 normal_amount;              // Offset:   48 Size:     8
//      = 0x3f800000 0x3f800000 
	float specular_pow;                // Offset:   56 Size:     4
//      = 0x42c80000 
	float3 specular_color;             // Offset:   64 Size:    12
//      = 0x3f800000 0x3f800000 0x3f800000 
	float light_smooth;                // Offset:   76 Size:     4
//      = 0x00000000 
	float metallic;                    // Offset:   80 Size:     4
//      = 0x00000000 
	float roughness;                   // Offset:   84 Size:     4
//      = 0x00000000 
	float fall_off_amount;             // Offset:   88 Size:     4
//      = 0x3f800000 
	float fall_off_scale;              // Offset:   92 Size:     4
//      = 0x3f800000 
	float fall_off_offset;             // Offset:   96 Size:     4
//      = 0x00000000 
	float3 pad;
	float3 light_color = float3(1.0, 1.0, 1.0);
	// x is min color, y is add color, z is speed
	float3 light_pow = float3(0.5, 0.75, 1.0);
}

Texture2D albedo;
SamplerState albedo_sampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

Texture2D normal;
SamplerState normal_sampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

Texture2D param_cm0_cm1_occ;
SamplerState param_cm0_cm1_occ_sampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

Texture2D param_reflect_specpow_specint;
SamplerState param_reflect_specpow_specint_sampler
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
	float4 o0 : SV_TARGET0;
	float4 o1 : SV_TARGET1;
	float4 o2 : SV_TARGET2;
	float4 o3 : SV_TARGET3;
	float4 o4 : SV_TARGET4;
	float4 o5 : SV_TARGET5;
	float4 light : SV_TARGET7;
};

PS_OUTPUT main(PS_INPUT In)
{
	PS_OUTPUT v_out;

	float4 s_albedo = albedo.Sample(albedo_sampler, In.texcoord0);
	float3 diffuseColor = s_albedo * diffuse;

	float4 s_cco = param_cm0_cm1_occ.Sample(param_cm0_cm1_occ_sampler, In.texcoord0);
	float3 changeColor0 = change_color0 - 1.0;
	changeColor0 = s_cco.x * changeColor0 + 1.0;
	diffuseColor *= changeColor0;
	
	float3 changeColor1 = change_color1 - 1.0;
	changeColor1 = s_cco.y * changeColor1 + 1.0;
	diffuseColor *= changeColor1;
	v_out.o0.xyz = diffuseColor;
	v_out.o0.w = 1.0;
	
	float4 s_rss = param_reflect_specpow_specint.Sample(param_reflect_specpow_specint_sampler, In.texcoord0);
	// now z is light
	v_out.o1.xyz = s_rss.y * specular_color;
	v_out.o1.w = 1.0;
	//
    float tangent1 = dot(In.tangent, In.tangent);
    tangent1 = rsqrt(tangent1);
    float3 tangent3 = tangent1 * In.tangent;

    float binormal1 = dot(In.binormal, In.binormal);
    binormal1 = rsqrt(binormal1);
    float3 binormal3 = binormal1 * In.binormal;
	
	float4 s_normal = normal.Sample(normal_sampler, In.texcoord0);
    float norX = s_normal.z * s_normal.x;
    float2 nor2;
    nor2.x = norX - 0.5;
    nor2.y = s_normal.y - 0.5;
    nor2 *= 2.0;
	
    float2 normal2 = nor2 * normal_amount;
    float norNZ = 1 - dot(nor2, nor2);
    norNZ = sqrt( max(norNZ, 0.00001) );
    binormal3 = binormal3 * -normal2.y;
    float3 nor3 = normal2.x * tangent3 + binormal3;
	
    float normal1 = dot(In.normal, In.normal);
    normal1 = rsqrt(normal1);
    float3 normal3 = normal1 * In.normal;

    nor3 = norNZ * normal3 + nor3;
    float nor3z = rsqrt(dot(nor3, nor3));
    nor3 *= nor3z;
    v_out.o2.xyz = s_cco.z * nor3;
    v_out.o2.w = 1.0;
	//
    float3 eye3 = rsqrt(dot(In.eye, In.eye));
    eye3 *= In.eye;
    float fall_off = max(fall_off_amount, 0.01);
    float fall_sat = log(saturate(dot(-eye3, nor3))) * fall_off;
    fall_sat = 1.0 - exp(fall_sat);
    fall_sat = saturate(fall_sat * fall_off_scale + fall_off_offset);
    v_out.o3.y = s_rss.x * fall_sat;
    v_out.o3.z = s_rss.y * specular_pow;
    v_out.o3.x = light_smooth;
    v_out.o3.w = 1.0;
	//
    v_out.o4.x = metallic;
    v_out.o4.y = roughness;
    v_out.o4.z = 0.0;
    v_out.o4.w = 1.0;
    v_out.o5.xyz = In.view;
    v_out.o5.w = g_xgl_id;
	//
	float3 lightColor = light_color * s_rss.z;
	lightColor *= max(sin(g_xgl_time * light_pow.z) + light_pow.y, light_pow.x);
	//float time_scroll = sin(g_xgl_time);
	//float3 scrolling = param_cm0_cm1_occ.Sample(param_cm0_cm1_occ_sampler, In.texcoord0 * time_scroll);
	//lightColor *= scrolling.x;
	v_out.light.xyz = lightColor;
	v_out.light.w = 1.0;

	return v_out;
}