#version 330 core

in vec3 FragPos;
in vec3 Normal;

uniform vec3 uViewPos;
uniform vec3 uLightPos;
uniform vec3 uLightColor;

uniform vec3 uKa;
uniform vec3 uKd;
uniform vec3 uKs;
uniform float uNs;

out vec4 FragColor;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = uKa * uLightColor;
    vec3 diffuse = uKd * max(dot(norm, lightDir), 0.0) * uLightColor;
    vec3 specular = uKs * pow(max(dot(viewDir, reflectDir), 0.0), uNs) * uLightColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
