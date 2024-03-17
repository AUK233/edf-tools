
cbuffer xgl_system : register( b0 )
{
	float4x4 g_xgl_view;               // Offset:    0 Size:    64 [unused]
	float4x4 g_xgl_view_inverse;       // Offset:   64 Size:    64
	float4x4 g_xgl_projection;         // Offset:  128 Size:    64 [unused]
	float4x4 g_xgl_view_projection;    // Offset:  192 Size:    64 [unused]
	float4x4 g_xgl_view_projection_inverse;// Offset:  256 Size:    64 [unused]
	uint2 g_xgl_target_dimension;      // Offset:  320 Size:     8 [unused]
	float g_xgl_time;                  // Offset:  328 Size:     4
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

// cb 1

cbuffer xgl_transform : register( b1 )
{
	float4x4 g_xgl_world;
}

// cb 2
cbuffer xgl_user_param : register( b2 )
{
	float4 g_color_1;                  // Offset:    0 Size:    16
//      = 0x3f19999a 0x00000000 0x00000000 0x3f000000 
	float4 g_color_2;                  // Offset:   16 Size:    16
//      = 0x3f19999a 0x00000000 0x00000000 0x3e4ccccd 
	float4 g_edge_color;               // Offset:   32 Size:    16
//      = 0x3f19999a 0x00000000 0x00000000 0x3f000000 
	float4 g_start_pos;                // Offset:   48 Size:    16
//      = 0x00000000 0x00000000 0x00000000 0x00000000 
	float4 g_end_pos;                  // Offset:   64 Size:    16
//      = 0x00000000 0x00000000 0x00000000 0x00000000 
	float g_radius;                    // Offset:   80 Size:     4
//      = 0x00000000 
	float g_line_loop;                 // Offset:   84 Size:     4
//      = 0x41200000 
	float g_dense_size;                // Offset:   88 Size:     4
//      = 0x3f000000 
	float g_edge_size;                 // Offset:   92 Size:     4
//      = 0x3dcccccd 
	int g_is_draw_stripe;              // Offset:   96 Size:     4
//      = 0x00000001 
	int g_is_draw_itemcollect;         // Offset:  100 Size:     4
//      = 0x00000000 
	float g_gradation_area;            // Offset:  104 Size:     4 [unused]
//      = 0x3f000000 
}

Texture2D g_xgl_viewXYZ_id_texture : register( t0 );

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 vpos : VIEW_POSITION;
    float3 fade : FOG_FADE;
};

struct PS_OUTPUT
{
    float4 out0 : SV_TARGET0;
    float4 out1 : SV_TARGET1;
    float4 out2 : SV_TARGET2;
    float4 out3 : SV_TARGET3;
    float4 out4 : SV_TARGET4;
    float4 out5 : SV_TARGET5;
    float4 out6 : SV_TARGET6;
};

PS_OUTPUT main(PS_INPUT Input)
{
    PS_OUTPUT v_out;
	float4 temp0, temp1, temp2, temp3, temp4, temp5;
    temp0.xy = int2(Input.pos.xy);
	temp0.wz = float2(0, 0);
    temp0 = g_xgl_viewXYZ_id_texture.Load(temp0.xyz);
    /*
    temp1.x = temp0.w - float1(1);
    if (temp1.x < float1(0))
    {
        discard;
    }*/
    temp0.w = float1(1);
    temp1.x = dot(temp0.xyzw, g_xgl_view_inverse._11_21_31_41);
    temp1.z = dot(temp0.xyzw, g_xgl_view_inverse._13_23_33_43);
    
    // g_is_draw_stripe == 0 and g_is_draw_itemcollect == 1
    temp1.y = dot(temp0.xyzw, g_xgl_view_inverse._12_22_32_42);
    temp0.xyz = (g_start_pos.xyz == g_end_pos.xyz);
    temp0.x = temp0.x && temp0.y;
    temp0.x = temp0.x && temp0.z;
    temp3 = g_end_pos - g_start_pos;
    temp1.w = float1(1);
    temp4 = temp1 - g_start_pos;
    temp0.y = dot(temp3.xyz, temp3.xyz);
    temp0.z = dot(temp3, temp4);
    temp0.y = temp0.z / temp0.y;
    temp0.z = (float1(0) >= temp0.y);
    temp0.w = (temp0.y >= float1(1));
    temp3 = temp0.yyyy * temp3 + g_start_pos;
    if (temp0.w)
    {
        temp3 = g_end_pos;
    }
    temp0.x = temp0.x || temp0.z;
    if (temp0.x)
    {
        temp0 = g_start_pos;
    }
    else
    {
        temp0 = temp3;
    }
    temp0 = temp0 - temp1;
    temp0.x = dot(temp0, temp0);
    temp0.x = sqrt(temp0.x);
    temp0.y = g_radius - temp0.x;
    if (temp0.y < float1(0))
    {
        discard;
    }
    temp0.y = temp0.x / g_radius;
    temp0.zw = g_xgl_time * float2(10, 600);
    temp0.w = (temp0.w >= -temp0.w);
    if (temp0.w)
    {
        temp3.xy = float2(60, 0.016667);
    }
    else
    {
        temp3.xy = float2(-60, -0.016667);
    }
    temp0.z *= temp3.y;
    temp0.z = frac(temp0.z);
    temp0.z *= temp3.x;
    temp0.x += temp0.z * float1(0.266667);
 
    int r0z = temp0.x;
    int r0w = r0z ^ 16;
    r0z = max(r0z, -r0z);
    r0z = r0z >> 4;
    int r3z = 0 - r0z;
    r0w = r0w && 0x80000000;
    if (r0w)
    {
        r0z = r3z;
    }
    temp0.z = r0z;
    temp0.x = temp0.x * 0.062500 - temp0.z;
    temp0.z = temp0.x * 32.0;

    temp3.xyz = g_color_1.xyz;
    temp3.w = 0;
    temp4 = (g_color_1 - temp3) * temp0.zzzz + temp3;
    temp0.zw = (temp0.xx < float2(0.031250, 0.062500));
    temp0.x = (temp0.x * 16.0 - 0.5)*2;
    temp5 = (temp3 - g_color_1) * temp0.xxxx + g_color_1;
    if (temp0.w)
    {
        temp3 = temp5;
    }
    if (temp0.z)
    {
        temp3 = temp4;
    }
    
    temp0.xz = float2(0.7, 1) - g_edge_size;
    // ceil = round_pi
    temp0.w = trunc((temp0.y / temp0.x - 0.5) * 2);
    temp0.w = temp0.w / (temp0.w-0.0);
    temp4.x = 1.0 - temp0.w;
    temp0.x = 1.0 - (temp0.w * temp4.x + temp0.x);
    temp3.w *= temp0.x;
    temp0.x = (temp0.z < temp0.y);
    if (temp0.x)
    {
        temp2 = g_edge_color;
    }
    else
    {
        temp2 = temp3;
    }
    temp0.x = abs(g_start_pos.y - temp1.y) / (g_radius * 0.5);
    temp0.y = temp0.x * -2.0 + 3.0;
    temp0.x = temp0.x * temp0.x * -temp0.y + 1.0;
    temp2.w *= temp0.x;
    
    temp2 += g_color_2;
    if (temp2.w > 0.9999)
    {
        temp2.w = 1.0;
    }
    
    // g_is_draw_stripe == 0 and g_is_draw_itemcollect == 0
    /*
	temp0.xy = (g_start_pos.xz == g_end_pos.xz);
    temp0.x = temp0.x && temp0.y;
    temp0.yzw = g_end_pos.xzw - g_start_pos.xzw;
    temp1.w = float1(1);
    temp3.xyz = temp1.xzw - g_start_pos.xzw;
    temp1.y = dot(temp0.yz, temp0.yz);
    temp3.x = dot(temp0.yzw, temp3.xyz);
    temp1.y = temp3.x / temp1.y;
    temp3.x = (float1(0) >= temp1.y);
    temp3.y = (temp1.y >= float1(1));
    temp0.yzw = temp1.y * temp0.yzw + g_start_pos.xzw;
    if (temp3.y)
    {
        temp0.yzw = g_end_pos.xzw;
    }
    temp0.x = temp0.x || temp3.x;
    if (temp0.x)
    {
        temp0.xyz = g_start_pos.xzw;
    }
    else
    {
        temp0.xyz = temp0.yzw;
    }
    
    temp0.xyz -= temp1.xzw;
    temp0.x = dot(temp0.xyz, temp0.xyz);
    temp0.x = sqrt(temp0.x);
    temp0.y = g_radius - temp0.x;
    if (temp0.y < float1(0))
    {
        discard;
    }
    temp0.x = temp0.x / g_radius;
    temp0.y = float1(1) - g_edge_size;
    if (temp0.y < temp0.x)
    {
        temp2 = g_edge_color;
    }
    else
    {
        temp2 = g_color_1;
    }*/
    
    v_out.out0 = temp2;
    temp2.x = 0;
    v_out.out1 = temp2.xxxw;
    v_out.out2 = temp2.xxxw;
    temp2.xz = float2(0, 10);
    v_out.out3 = temp2.xxzw;
    v_out.out4 = temp2.xxxw;
    v_out.out5.xyz = Input.vpos.xyz;
    v_out.out5.w = g_xgl_id;
    v_out.out6.xyz = float3(1, 1, 1);
    v_out.out6.w = temp2.w;
	//return float4(1.0f, 1.0f, 1.0f, 1.0f);
	return v_out;
}