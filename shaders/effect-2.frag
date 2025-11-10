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

// 간단한 해시 기반 노이즈 함수
float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    // 파동 기반 노이즈 추가 (시간에 따라 움직임)
    float wave = sin(length(FragPos.xy) * 10.0 - uTime * 3.0);
    float waveStrength = 0.2 * wave;

    float noiseX = rand(FragPos.yz * 10.0 + uTime) * 2.0 - 1.0;
    float noiseY = rand(FragPos.zx * 10.0 + uTime) * 2.0 - 1.0;
    float noiseZ = rand(FragPos.xy * 10.0 + uTime) * 2.0 - 1.0;

    vec3 noisyNormal = normalize(Normal + waveStrength * vec3(noiseX, noiseY, noiseZ));

    // 광원, 뷰 벡터
    vec3 lightDir = normalize(uLightPos - FragPos);
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, noisyNormal);

    // 조명 모델
    vec3 ambient = uKa * uLightColor;
    vec3 diffuse = uKd * max(dot(noisyNormal, lightDir), 0.0) * uLightColor;
    vec3 specular = uKs * pow(max(dot(viewDir, reflectDir), 0.0), uNs) * uLightColor;

    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(result, 1.0);
}
