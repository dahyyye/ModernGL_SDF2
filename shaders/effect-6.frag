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
    // 파동 기반 노멀 흔들기
    float waveStrength = 0.2;  // 흔들림 강도
    vec3 waveOffset = vec3(
        sin(FragPos.y * 10.0 + uTime * 3.0),
        sin(FragPos.z * 10.0 + uTime * 2.5),
        sin(FragPos.x * 10.0 + uTime * 2.0)
    );
    vec3 wavyNormal = normalize(Normal + waveStrength * waveOffset);

    // 조명 벡터들
    vec3 lightDir = normalize(uLightPos - FragPos);
    vec3 viewDir  = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, wavyNormal);

    // 💡 조명 계산
    vec3 ambient = uKa * uLightColor;
    vec3 diffuse = uKd * max(dot(wavyNormal, lightDir), 0.0) * uLightColor;
    vec3 specular = uKs * pow(max(dot(viewDir, reflectDir), 0.0), uNs) * uLightColor;

    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(result, 1.0);
}
