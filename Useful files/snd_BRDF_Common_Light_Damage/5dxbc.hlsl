// ---- Created with 3Dmigoto v1.3.16 on Wed Mar 12 15:30:52 2025

cbuffer xgl_system : register(b0)
{
  float4x4 g_xgl_view : packoffset(c0);
  float4x4 g_xgl_view_inverse : packoffset(c4);
  float4x4 g_xgl_projection : packoffset(c8);
  float4x4 g_xgl_view_projection : packoffset(c12);
  float4x4 g_xgl_view_projection_inverse : packoffset(c16);
  uint2 g_xgl_target_dimension : packoffset(c20);
  float g_xgl_time : packoffset(c20.z) = {0};
  int g_xgl_id : packoffset(c20.w) = {0};
  float3 g_xgl_light_vector0 : packoffset(c21) = {0.699999988,-0.699999988,-0.699999988};
  float3 g_xgl_light_color0 : packoffset(c22) = {1,1,1};
  float3 g_xgl_light_specular_color0 : packoffset(c23) = {1,1,1};
  float3 g_xgl_light_vector1 : packoffset(c24) = {-0.699999988,0.699999988,0.699999988};
  float3 g_xgl_light_color1 : packoffset(c25) = {0.200000003,0.200000003,0.200000003};
  float3 g_xgl_light_specular_color1 : packoffset(c26) = {1,1,1};
  float3 g_xgl_light_vector2 : packoffset(c27) = {0,0,0};
  float3 g_xgl_light_color2 : packoffset(c28) = {0,0,0};
  float3 g_xgl_light_specular_color2 : packoffset(c29) = {1,1,1};
  float3 g_xgl_light_vector3 : packoffset(c30) = {0,0,0};
  float3 g_xgl_light_color3 : packoffset(c31) = {0,0,0};
  float3 g_xgl_light_specular_color3 : packoffset(c32) = {0,0,0};
  float3 g_xgl_ambient_color : packoffset(c33) = {0.200000003,0.200000003,0.200000003};
  float g_pad2 : packoffset(c33.w);
  float4 g_xgl_fog_fade_param : packoffset(c34) = {0,0,0,0};
  float4 g_xgl_fog_color : packoffset(c35) = {1,1,1,0};
}

cbuffer xgl_user_param : register(b2)
{
  float3 diffuse : packoffset(c0) = {1,1,1};
  float2 normal_amount : packoffset(c1) = {1,1};
  float specular_pow : packoffset(c1.z) = {100};
  float3 specular_color : packoffset(c2) = {1,1,1};
  float light_smooth : packoffset(c2.w) = {0};
  float metallic : packoffset(c3) = {0};
  float roughness : packoffset(c3.y) = {0};
  float fall_off_amount : packoffset(c3.z) = {1};
  float fall_off_scale : packoffset(c3.w) = {1};
  float fall_off_offset : packoffset(c4) = {0};
  float damage_test : packoffset(c4.y) = {0};
  float2 damage_uv_mul : packoffset(c4.z) = {10,10};
  float2 damage_mask_uv_mul : packoffset(c5) = {10,10};
  float damage_normal_amount : packoffset(c5.z) = {1};
  float damage_specular_pow : packoffset(c5.w) = {100};
  float3 damage_specular_color : packoffset(c6) = {1,1,1};
  float3 damage_edge_color : packoffset(c7) = {1,1,1};
  float3 damage_max_color : packoffset(c8) = {0.200000003,0.100000001,0.100000001};
  float damage_reflection : packoffset(c8.w) = {0};
  float damage_metallic : packoffset(c9) = {0};

  struct
  {
    float3 m_org;
    uint m_line_pitch;
    float3 m_inv_grid_size;
    uint m_slice_pitch;
  } sys_damage_map_param : packoffset(c10);

}

SamplerState albedo_sampler_s : register(s0);
SamplerState normal_sampler_s : register(s1);
SamplerState param_reflect_specpow_specint_occ_sampler_s : register(s2);
SamplerState damage_dist_sampler_s : register(s3);
SamplerState damage_diffuse_sampler_s : register(s4);
SamplerState damage_normal_sampler_s : register(s5);
Texture2D<float4> albedo : register(t0);
Texture2D<float4> normal : register(t1);
Texture2D<float4> param_reflect_specpow_specint_occ : register(t2);
Texture2D<float4> damage_dist : register(t3);
Texture2D<float4> damage_diffuse : register(t4);
Texture2D<float4> damage_normal : register(t5);
StructuredBuffer<uint> sys_damage_map : register(t6);


// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float3 v1 : EYE_VECTOR0,
  float3 v2 : VIEW_POSITION0,
  float3 v3 : LOCAL_POSITION0,
  float3 v4 : NORMAL0,
  float3 v5 : BINORMAL0,
  float3 v6 : TANGENT0,
  float2 v7 : TEXCOORD0,
  out float4 o0 : SV_TARGET0,
  out float4 o1 : SV_TARGET1,
  out float4 o2 : SV_TARGET2,
  out float4 o3 : SV_TARGET3,
  out float4 o4 : SV_TARGET4,
  out float4 o5 : SV_TARGET5)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = dot(v4.xyz, v4.xyz);
  r0.x = rsqrt(r0.x);
  r0.xyz = v4.xyz * r0.xxx;
  r0.w = dot(v5.xyz, v5.xyz);
  r0.w = rsqrt(r0.w);
  r1.xyz = v5.xyz * r0.www;
  r0.w = dot(v6.xyz, v6.xyz);
  r0.w = rsqrt(r0.w);
  r2.xyz = v6.xyz * r0.www;
  r0.w = dot(v1.xyz, v1.xyz);
  r0.w = rsqrt(r0.w);
  r3.xyz = v1.xyz * r0.www;
  r4.xyz = normal.Sample(normal_sampler_s, v7.xy).xyw;
  r4.x = r4.x * r4.z;
  r4.xy = float2(-0.5,-0.5) + r4.xy;
  r4.xy = r4.xy + r4.xy;
  r0.w = dot(r4.xy, r4.xy);
  r0.w = 1 + -r0.w;
  r0.w = max(9.99999975e-06, r0.w);
  r0.w = sqrt(r0.w);
  r4.xy = normal_amount.xy * r4.xy;
  r4.yzw = -r4.yyy * r1.xyz;
  r4.xyz = r4.xxx * r2.xyz + r4.yzw;
  r4.xyz = r0.www * r0.xyz + r4.xyz;
  r0.w = dot(r4.xyz, r4.xyz);
  r0.w = rsqrt(r0.w);
  r4.xyz = r4.xyz * r0.www;
  r5.xyzw = param_reflect_specpow_specint_occ.Sample(param_reflect_specpow_specint_occ_sampler_s, v7.xy).xyzw;
  r0.w = saturate(dot(-r3.xyz, r4.xyz));
  r1.w = max(0.00999999978, fall_off_amount);
  r0.w = log2(r0.w);
  r0.w = r1.w * r0.w;
  r0.w = exp2(r0.w);
  r0.w = 1 + -r0.w;
  r0.w = saturate(r0.w * fall_off_scale + fall_off_offset);
  r3.y = r0.w * r5.x;
  r3.x = specular_pow * r5.y;
  r5.xyz = specular_color.xyz * r5.zzz;
  r6.xyz = -sys_damage_map_param.m_org.xyz + v3.xyz;
  r6.xyz = sys_damage_map_param.m_inv_grid_size.xyz * r6.xyz;
  r7.xyz = frac(r6.xyz);
  r6.xyz = (int3)r6.xyz;
  r8.xy = (int2)r6.xy + int2(1,1);
  r6.w = r8.y;
  r8.z = r6.x;
  r9.xyzw = mad((int4)r6.yyww, (int12)sys_damage_map_param.m_line_pitch, (int4)r8.zxzx);
  r9.xyzw = mad((int4)r6.zzzz, (int13)sys_damage_map_param.m_slice_pitch, (int4)r9.xyzw);
  r10.xyzw = (uint4)r9.xyzw >> int4(4,4,4,4);
  r11.x = sys_damage_map[r10.x].x;
  r11.y = sys_damage_map[r10.y].x;
  r11.z = sys_damage_map[r10.z].x;
  r11.w = sys_damage_map[r10.w].x;
  bitmask.x = ((~(-1 << 4)) << 1) & 0xffffffff;  r9.x = (((uint)r9.x << 1) & bitmask.x) | ((uint)0 & ~bitmask.x);
  bitmask.y = ((~(-1 << 4)) << 1) & 0xffffffff;  r9.y = (((uint)r9.y << 1) & bitmask.y) | ((uint)0 & ~bitmask.y);
  bitmask.z = ((~(-1 << 4)) << 1) & 0xffffffff;  r9.z = (((uint)r9.z << 1) & bitmask.z) | ((uint)0 & ~bitmask.z);
  bitmask.w = ((~(-1 << 4)) << 1) & 0xffffffff;  r9.w = (((uint)r9.w << 1) & bitmask.w) | ((uint)0 & ~bitmask.w);
  r9.xyzw = (uint4)r11.xyzw >> (uint4)r9.xyzw;
  r9.xyzw = (int4)r9.xyzw & int4(3,3,3,3);
  r9.xyzw = (uint4)r9.xyzw;
  r9.xyzw = float4(0.333333343,0.333333343,0.333333343,0.333333343) * r9.xyzw;
  r6.xyz = (int3)r6.xyz + int3(0,0,1);
  r8.w = r6.x;
  r8.xyzw = mad((int4)r6.yyww, (int12)sys_damage_map_param.m_line_pitch, (int4)r8.wxwx);
  r6.xyzw = mad((int4)r6.zzzz, (int13)sys_damage_map_param.m_slice_pitch, (int4)r8.xyzw);
  r8.xyzw = (uint4)r6.xyzw >> int4(4,4,4,4);
  r10.x = sys_damage_map[r8.x].x;
  r10.y = sys_damage_map[r8.y].x;
  r10.z = sys_damage_map[r8.z].x;
  r10.w = sys_damage_map[r8.w].x;
  bitmask.x = ((~(-1 << 4)) << 1) & 0xffffffff;  r6.x = (((uint)r6.x << 1) & bitmask.x) | ((uint)0 & ~bitmask.x);
  bitmask.y = ((~(-1 << 4)) << 1) & 0xffffffff;  r6.y = (((uint)r6.y << 1) & bitmask.y) | ((uint)0 & ~bitmask.y);
  bitmask.z = ((~(-1 << 4)) << 1) & 0xffffffff;  r6.z = (((uint)r6.z << 1) & bitmask.z) | ((uint)0 & ~bitmask.z);
  bitmask.w = ((~(-1 << 4)) << 1) & 0xffffffff;  r6.w = (((uint)r6.w << 1) & bitmask.w) | ((uint)0 & ~bitmask.w);
  r6.xyzw = (uint4)r10.xyzw >> (uint4)r6.xyzw;
  r6.xyzw = (int4)r6.xyzw & int4(3,3,3,3);
  r6.xyzw = (uint4)r6.xyzw;
  r6.xyzw = r6.xyzw * float4(0.333333343,0.333333343,0.333333343,0.333333343) + -r9.xyzw;
  r6.xyzw = r7.zzzz * r6.xyzw + r9.xyzw;
  r6.zw = r6.zw + -r6.xy;
  r6.xy = r7.yy * r6.zw + r6.xy;
  r0.w = r6.y + -r6.x;
  r0.w = r7.x * r0.w + r6.x;
  r6.xy = damage_uv_mul.xy * v7.xy;
  r6.zw = damage_mask_uv_mul.xy * v7.xy;
  r1.w = damage_dist.Sample(damage_dist_sampler_s, r6.zw).x;
  r0.w = -r1.w + r0.w;
  r1.w = cmp(0 < r0.w);
  r7.xyz = damage_normal.Sample(damage_normal_sampler_s, r6.xy).xyw;
  r6.xyz = damage_diffuse.Sample(damage_diffuse_sampler_s, r6.xy).xyz;
  if (r1.w != 0) {
    r7.x = r7.x * r7.z;
    r7.xy = float2(-0.5,-0.5) + r7.xy;
    r7.xy = r7.xy + r7.xy;
    r1.w = dot(r7.xy, r7.xy);
    r1.w = 1 + -r1.w;
    r1.w = max(9.99999975e-06, r1.w);
    r1.w = sqrt(r1.w);
    r7.xy = damage_normal_amount * r7.xy;
    r1.xyz = -r7.yyy * r1.xyz;
    r1.xyz = r7.xxx * r2.xyz + r1.xyz;
    r0.xyz = r1.www * r0.xyz + r1.xyz;
    r1.x = dot(r0.xyz, r0.xyz);
    r1.x = rsqrt(r1.x);
    r4.xyz = r1.xxx * r0.xyz;
    r0.x = saturate(r0.w);
    r1.xyz = damage_max_color.xyz + -r6.xyz;
    r0.xyz = r0.xxx * r1.xyz + r6.xyz;
    r0.w = cmp(r0.w < 0.100000001);
    r0.xyz = r0.www ? damage_edge_color.xyz : r0.xyz;
    r5.xyz = r0.www ? r5.xyz : damage_specular_color.xyz;
    r1.x = metallic;
    r3.x = r0.w ? r3.x : damage_specular_pow;
    r3.y = r0.w ? r3.y : damage_reflection;
    r1.x = r0.w ? r1.x : damage_metallic;
  } else {
    r2.xyz = albedo.Sample(albedo_sampler_s, v7.xy).xyz;
    r0.xyz = diffuse.xyz * r2.xyz;
    r1.x = metallic;
  }
  o2.xyz = r4.xyz * r5.www;
  o5.w = g_xgl_id;
  o0.xyz = r0.xyz;
  o0.w = 1;
  o1.xyz = r5.xyz;
  o1.w = 1;
  o2.w = 1;
  r3.z = light_smooth;
  r3.w = 1;
  o3.xyzw = r3.zyxw;
  r1.y = roughness;
  r1.zw = float2(0,1);
  o4.xyzw = r1.xyzw;
  o5.xyz = v2.xyz;
  return;
}