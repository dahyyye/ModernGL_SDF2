#version 330 core

in vec3 FragPos;     // 월드 공간에서의 위치
in vec3 Normal;      // 월드 공간에서의 노멀

uniform vec3 uViewPos;      // 카메라 위치
uniform vec3 uLightPos;     // 광원 위치
uniform vec3 uLightColor;   // 광원 색상

uniform vec3 uKa;           // ambient 계수
uniform vec3 uKd;           // diffuse 계수
uniform vec3 uKs;           // specular 계수
uniform float uNs;          // shininess 계수

out vec4 FragColor;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    vec3 viewDir  = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    // Ambient (그대로)
    vec3 ambient = uKa * uLightColor;

    // Diffuse (그대로)
    vec3 diffuse = uKd * max(dot(norm, lightDir), 0.0) * uLightColor;

    // RGB 분해 스펙큘러 효과
    vec3 specularR = uKs.r * pow(max(dot(viewDir, reflectDir + vec3(0.05, 0.0, 0.0)), 0.0), uNs) * vec3(1.0, 0.0, 0.0);
    vec3 specularG = uKs.g * pow(max(dot(viewDir, reflectDir + vec3(-0.05, 0.0, 0.0)), 0.0), uNs) * vec3(0.0, 1.0, 0.0);
    vec3 specularB = uKs.b * pow(max(dot(viewDir, reflectDir), 0.0), uNs) * vec3(0.0, 0.0, 1.0);

    vec3 specular = specularR + specularG + specularB;

    // 최종 색상 합성
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
