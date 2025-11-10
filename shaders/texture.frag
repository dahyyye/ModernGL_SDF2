#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 uViewPos;
uniform vec3 uLightPos;
uniform vec3 uLightColor;

uniform vec3 uKa;
uniform vec3 uKd;
uniform vec3 uKs;
uniform float uNs;
uniform sampler2D uTex;

out vec4 FragColor;

void main()
{
    vec3 texColor = texture(uTex, TexCoord).rgb;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = uKa * texColor;
    vec3 diffuse = uKd * max(dot(norm, lightDir), 0.0) * texColor;
    vec3 specular = uKs * pow(max(dot(viewDir, reflectDir), 0.0), uNs) * uLightColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
