
cbuffer xgl_system : register( b0 )
{
	float4x4 g_xgl_view;
	float4x4 g_xgl_view_inverse;
	float4x4 g_xgl_projection;
	float4x4 g_xgl_view_projection;
	float4x4 g_xgl_view_projection_inverse;
	uint2 g_xgl_target_dimension;
	float g_xgl_time = 0;
	int g_xgl_id = 0;
	float3 g_xgl_light_vector0 = {0.699999988,-0.699999988,-0.699999988};
	float3 g_xgl_light_color0 = {1,1,1};
	float3 g_xgl_light_specular_color0  = {1,1,1};
	float3 g_xgl_light_vector1 = {-0.699999988,0.699999988,0.699999988};
	float3 g_xgl_light_color1 = {0.200000003,0.200000003,0.200000003};
	float3 g_xgl_light_specular_color1 = {1,1,1};
	float3 g_xgl_light_vector2 = {0,0,0};
	float3 g_xgl_light_color2 = {0,0,0};
	float3 g_xgl_light_specular_color2 = {1,1,1};
	float3 g_xgl_light_vector3 = {0,0,0};
	float3 g_xgl_light_color3 = {0,0,0};
	float3 g_xgl_light_specular_color3 = {0,0,0};
	float3 g_xgl_ambient_color = {0.200000003,0.200000003,0.200000003};
	float g_pad2;
	float4 g_xgl_fog_fade_param = {0,0,0,0};
	float4 g_xgl_fog_color = {1,1,1,0};
}

// cb 2
cbuffer xgl_user_param : register( b2 )
{
	float3 diffuse = float3(1.0, 1.0, 1.0);
	float3 specular_color;             // Specular color
    float specular_diffuse_blend;      // Specular-diffuse blend factor
    float3 light_color;                // Light color
    float normal_amount;               // Normal map intensity
    float metallic;                    // Metallic factor
    float roughness;                   // Roughness factor
    float damage_test;                 // Damage test (unused)
    float2 damage_uv_mul;              // Damage UV multiplier
    float2 damage_mask_uv_mul;         // Damage mask UV multiplier
    float damage_normal_amount;        // Damage normal intensity
    float3 damage_specular_color;      // Damage specular color
    float3 damage_edge_color;          // Damage edge color
    float3 damage_max_color;           // Damage max color
    float damage_metallic;             // Damage metallic factor
    float damage_roughness;            // Damage roughness factor
    float damage_light_mask;           // Damage light mask

    struct DamageMapParam
    {
        float3 m_org;                  // Origin of damage map
        uint m_line_pitch;             // Line pitch of damage map
        float3 m_inv_grid_size;        // Inverse grid size of damage map
        uint m_slice_pitch;            // Slice pitch of damage map
    } sys_damage_map_param;            // Damage map parameters
}

Texture2D albedo;
SamplerState albedo_sampler;

Texture2D normal;
SamplerState normal_sampler;

Texture2D param_r_m_occ_light;
SamplerState param_r_m_occ_light_sampler;

Texture2D damage_dist;
SamplerState damage_dist_sampler;

Texture2D damage_diffuse;
SamplerState damage_diffuse_sampler;

Texture2D damage_normal;
SamplerState damage_normal_sampler;

StructuredBuffer<uint> sys_damage_map;

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float3 eye : EYE_VECTOR;
	float3 view_position : VIEW_POSITION;
    float3 local_position : LOCAL_POSITION;
    float3 normal : NORMAL;
    float3 binormal : BINORMAL;
    float3 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
};

struct PS_OUTPUT
{
	
	float4 albedo : SV_TARGET0;
    float4 specular : SV_TARGET1;
    float4 normal : SV_TARGET2;
    float4 o3 : SV_TARGET3;
    float4 o4 : SV_TARGET4;
    float4 o5 : SV_TARGET5;
    float4 light_color : SV_TARGET7;
};

// Helper function to calculate the final normal vector
float3 CalculateNormal(float3 INnormal_sample, float3 INnormal, float3 INbinormal, float3 INtangent)
{
    // Unpack the normal map sample
    float3 normal_map = INnormal_sample * 2.0 - 1.0;
    normal_map.z = sqrt(1.0 - dot(normal_map.xy, normal_map.xy));

    // Calculate the final normal vector
    float3 v_normal = normalize(INtangent * normal_map.x + INbinormal * normal_map.y + INnormal * normal_map.z);
    return v_normal;
}

// Helper function to calculate the specular color
float3 CalculateSpecular(float3 INspecular, float INroughness, float INmetallic)
{
    // Calculate the specular color based on roughness and metallic
    float3 v_specular = INspecular * (1.0 - INroughness) * INmetallic;
    return v_specular;
}

PS_OUTPUT main(PS_INPUT input)
{
	PS_OUTPUT output;

    // Sample the parameter texture (roughness, metallic, occlusion, etc.)
    float4 param_sample = param_r_m_occ_light.Sample(param_r_m_occ_light_sampler, input.texcoord);
    float v_roughness = param_sample.x;
    float v_metallic = param_sample.y;
    float occlusion = param_sample.z;

    // Sample the albedo texture and apply the diffuse color
    float3 albedo_sample = albedo.Sample(albedo_sampler, input.texcoord).rgb;
    float3 diffuse_color = albedo_sample * diffuse;

    // Calculate the final diffuse color with occlusion
    float3 final_diffuse = diffuse_color * occlusion;

    // Sample the normal map and calculate the final normal vector
    float3 s_normal = normal.Sample(normal_sampler, input.texcoord).rgb;
   	float3 final_normal = CalculateNormal(s_normal, normalize(input.normal), normalize(input.binormal), normalize(input.tangent));

    // Calculate the specular color based on roughness and metallic
    float3 final_specular = CalculateSpecular(specular_color, v_roughness, v_metallic);

    float3 damage_dist_sample = damage_dist.Sample(damage_dist_sampler, input.texcoord).rgb;
    float3 damage_diffuse_sample = damage_diffuse.Sample(damage_diffuse_sampler, input.texcoord).rgb;
    float3 damage_normal_sample = damage_normal.Sample(damage_normal_sampler, input.texcoord).rgb;
    // 计算损伤相关的逻辑
    float3 damage_uv = input.local_position - sys_damage_map_param.m_org;
    damage_uv *= sys_damage_map_param.m_inv_grid_size;

    float3 damage_uv_frac = frac(damage_uv);
    int3 damage_uv_int = int3(damage_uv);

    // 从损伤贴图中读取损伤数据
    uint damage_value0 = sys_damage_map.Load(damage_uv_int.x);
    uint damage_value1 = sys_damage_map.Load(damage_uv_int.y);
    uint damage_value2 = sys_damage_map.Load(damage_uv_int.z);
    uint damage_value3 = sys_damage_map.Load(damage_uv_int.x);

    // 插值计算损伤值
    float4 damage_values = float4(
        float(damage_value0 & 0x3),
        float(damage_value1 & 0x3),
        float(damage_value2 & 0x3),
        float(damage_value3 & 0x3)
    ) * 0.333333;

    float4 damage_interp = lerp(damage_values.xyzw, damage_values.zwxy, damage_uv_frac.z);
    float2 damage_final = lerp(damage_interp.xy, damage_interp.zw, damage_uv_frac.y);
    float damage = lerp(damage_final.x, damage_final.y, damage_uv_frac.x);

    // 计算损伤对漫反射和法线的影响
    float damage_amount = damage_dist_sample.r;
    damage_amount = saturate(damage_amount - param_sample.r);

    if (damage_amount)
    {
        float3 vDamageNormal = damage_normal_sample.rgb * 2.0 - 1.0;
        vDamageNormal.xy *= damage_normal_amount;

        float3 damaged_diffuse = lerp(damage_diffuse_sample.rgb, damage_max_color, damage_amount);
        float3 damaged_specular = lerp(damage_specular_color, damage_edge_color, damage_amount);

        // 更新漫反射和法线
        final_diffuse = lerp(final_diffuse, damaged_diffuse, damage_amount);
        final_normal = normalize(final_normal + vDamageNormal);
        final_specular = lerp(final_specular, damaged_specular, damage_amount);
    }

    // 光照计算
    float3 light_dir = normalize(g_xgl_light_vector0);
    float3 view_dir = normalize(input.view_position - input.local_position);
    float3 half_dir = normalize(light_dir + view_dir);

    float NdotL = max(dot(final_normal, light_dir), 0.0);
    float NdotH = max(dot(final_normal, half_dir), 0.0);

    // 漫反射光照
    float3 diffuse_light = final_diffuse * light_color * NdotL;

    // 镜面反射光照（Blinn-Phong模型）
    float specular_power = exp2(10.0 * (1.0 - roughness) + 1.0);
    float3 specular_light = final_specular * light_color * pow(NdotH, specular_power);

    // 最终颜色计算
    float3 final_color = diffuse_light + specular_light;


    // Output the final values
    output.albedo = float4(final_diffuse, 1.0);
    output.specular = float4(final_specular, 1.0);
    output.normal = float4(final_normal, 1.0);
	// other
	output.o3 = 1;
    output.o4 = float4(roughness, metallic, 0.0, 1.0);
	output.o5.xyz = input.view_position;
    output.o5.w = g_xgl_id;
    output.light_color = float4(final_color, 1.0);

    return output;
}