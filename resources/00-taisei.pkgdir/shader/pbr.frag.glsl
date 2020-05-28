#version 330 core

#include "lib/util.glslh"
#include "lib/render_context.glslh"
#include "interface/pbr.glslh"

UNIFORM(1) sampler2D roughness_map; 
UNIFORM(2) sampler2D normal_map; 
UNIFORM(3) sampler2D ambient_map; 
UNIFORM(4) float metallic;
UNIFORM(5) vec3 camera_pos; 

UNIFORM(6) vec3 light_positions[2];
UNIFORM(8) vec3 light_colors[2];

// taken from https://learnopengl.com/PBR/Lighting
float distribution_ggx(vec3 n, vec3 h, float roughness) {
	float a = roughness*roughness;

	float a2 = a*a;
	float ndoth = max(dot(n, h), 0.0);
	float ndoth2 = ndoth*ndoth;

	float num = a2;
	float denom = (ndoth2 * (a2 - 1.0) + 1.0);
	denom = pi * denom * denom;

	return num / denom;
}

float geometry_schlick_ggx(float ndotv, float roughness) {
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float num   = ndotv;
	float denom = ndotv * (1.0 - k) + k;

	return num / denom;
}

float geometry_smith(vec3 n, vec3 v, vec3 l, float roughness) {
	float ndotv = max(dot(n, v), 0.0);
	float ndotl = max(dot(n, l), 0.0);
	float ggx2  = geometry_schlick_ggx(ndotv, roughness);
	float ggx1  = geometry_schlick_ggx(ndotl, roughness);

	return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}  

// http://www.thetenthplanet.de/archives/1180
void main(void) {
	vec3 albedo = (r_color * texture(tex, texCoord)).rgb;
	float roughness = texture(roughness_map, texCoord).r;
	vec3 tbn_normal = texture(normal_map, texCoord).rgb*2-1;
	vec3 ambient = texture(ambient_map, texCoord).rgb;

	vec2 duvdx = dFdx(texCoord);
	vec2 duvdy = dFdy(texCoord);

	vec3 dposdxperp = -cross(normal,dFdx(pos));
	vec3 dposdyperp = -cross(dFdy(pos),normal);

	vec3 tangent = dposdyperp*duvdx.x + dposdxperp*duvdy.x;
	vec3 bitangent = dposdyperp*duvdx.y + dposdxperp*duvdy.y;

	float invmax = 1/max(length(tangent), length(bitangent));
	
	vec3 n = normalize(mat3(bitangent*invmax, tangent*invmax, normal)*tbn_normal);
	vec3 v = normalize(camera_pos - pos);

	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, albedo, metallic);
	           
	vec3 Lo = vec3(0.0);
	for(int i = 0; i < 2; ++i) {
		vec3 l = normalize(light_positions[i] - pos);
		vec3 h = normalize(v + l);
		float distance = length(light_positions[i] - pos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = light_colors[i] * attenuation;        

		float NDF = distribution_ggx(n, h, roughness);        
		float G = geometry_smith(n, v, l, roughness);      
		vec3 F = fresnel_schlick(max(dot(h, v), 0.0), F0);       

		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;	  

		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0);
		vec3 specular = numerator / max(denominator, 0.001);  
		    
		float NdotL = max(dot(n, l), 0.0);                
		Lo += (kD * albedo / pi + specular) * radiance * NdotL; 
	}   

	//vec3 ambient = vec3(0.03) * albedo * ao;
	vec3 color = ambient + Lo;

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));  

	fragColor = vec4(color,1.0);
}
