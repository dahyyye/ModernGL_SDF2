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

uniform float uTime;        // 시간

out vec4 FragColor;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    vec3 viewDir  = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    // Ambient
    vec3 ambient = uKa * uLightColor;

    // Diffuse
    vec3 diffuse = uKd * max(dot(norm, lightDir), 0.0) * uLightColor;

    // 크리스탈 깜빡임 스펙큘러 (각도 + 시간 기반)
    float angleFactor = max(dot(viewDir, reflectDir), 0.0);  // 시점과 반사 방향의 유사도
    float flicker = abs(sin(uNs * angleFactor + uTime * 5.0)); // 시간에 따른 깜빡임
    float intensity = pow(flicker, uNs); // 깜빡임을 샤이니하게

    vec3 specular = uKs * intensity * uLightColor;

    // 최종 합성
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
