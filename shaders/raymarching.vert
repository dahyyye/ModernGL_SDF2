#version 330 core

// [-1,1] 정점 2D 위치
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vTexCoord;

void main()
{
    vTexCoord = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}