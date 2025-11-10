#version 330 core

in vec3 Normal;  // 인터폴레이션된 월드 또는 뷰 공간의 노멀
out vec4 FragColor;

void main()
{
    // 노멀 벡터를 [0,1]로 매핑하여 색상화 (기본은 [-1,1] 범위이므로)
    vec3 color = normalize(Normal) * 0.5 + 0.5;
    FragColor = vec4(color, 1.0);
}
