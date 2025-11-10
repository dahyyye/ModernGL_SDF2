#version 330 core

in vec3 FragPos;
in vec3 Normal;

uniform vec3 uViewPos;
uniform vec3 uLightPos;
uniform vec3 uLightColor;

out vec4 FragColor;

void main()
{
    // 정규화
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);

    // Lambert 조명 (diffuse)
    float diff = max(dot(norm, lightDir), 0.0);

    // toon 쉐이딩용 단계 조정
    float intensity;
    if (diff > 0.95)
        intensity = 1.0;
    else if (diff > 0.5)
        intensity = 0.7;
    else if (diff > 0.25)
        intensity = 0.4;
    else
        intensity = 0.1;

    vec3 toonColor = intensity * uLightColor;

    FragColor = vec4(toonColor, 1.0);
}
