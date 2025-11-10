#version 330 core

in vec3 FragPos;     // 월드 공간에서의 위치
in vec3 Normal;      // 월드 공간에서의 노멀

uniform vec3 uViewPos;      // 카메라 위치 (월드 공간)
uniform vec3 uLightPos;     // 광원 위치 (월드 공간)
uniform vec3 uLightColor;   // 광원 색

uniform vec3 uKa;
uniform vec3 uKd;
uniform vec3 uKs;
uniform float uNs;

out vec4 FragColor;

// 간단한 해시 기반 노이즈 함수
float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    // 노멀 벡터에 작은 노이즈 추가 (노이즈 강도 조절 가능)
    float noiseStrength = 0.1;
    float noiseX = rand(FragPos.yz * 10.0) * 2.0 - 1.0; // [-1, 1]
    float noiseY = rand(FragPos.zx * 10.0) * 2.0 - 1.0;
    float noiseZ = rand(FragPos.xy * 10.0) * 2.0 - 1.0;
    
    vec3 noisyNormal = normalize(Normal + noiseStrength * vec3(noiseX, noiseY, noiseZ));

    // 광원 방향
    vec3 lightDir = normalize(uLightPos - FragPos);

    // 뷰 방향
    vec3 viewDir = normalize(uViewPos - FragPos);

    // 반사 방향
    vec3 reflectDir = reflect(-lightDir, noisyNormal);

    // Ambient
    vec3 ambient = uKa * uLightColor;

    // Diffuse
    vec3 diffuse = uKd * max(dot(noisyNormal, lightDir), 0.0) * uLightColor;

    // Specular
    vec3 specular = uKs * pow(max(dot(viewDir, reflectDir), 0.0), uNs) * uLightColor;

    // 최종 색상 계산
    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
