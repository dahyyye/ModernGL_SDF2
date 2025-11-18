#version 330 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler3D uSdfTex;      // 3D SDF 텍스처
uniform vec3 uVolumeMin;        // 볼륨 AABB 최소 (월드 좌표)
uniform vec3 uVolumeMax;        // 볼륨 AABB 최대 (월드 좌표)
uniform vec3 uCameraPos;        // 카메라 월드 위치
uniform mat4 uInvViewProj;      // inverse(ViewProj)

//----------------------
// 레이 vs 박스 교차
//----------------------
bool RayBoxIntersect(vec3 ro, vec3 rd, vec3 boxMin, vec3 boxMax, out float tMin, out float tMax)
{
    vec3 t1 = (boxMin - ro) / rd;
    vec3 t2 = (boxMax - ro) / rd;

    vec3 tNear = min(t1, t2);
    vec3 tFar  = max(t1, t2);

    tMin = max(max(tNear.x, tNear.y), tNear.z);
    tMax = min(min(tFar.x,  tFar.y),  tFar.z);

    return tMax > max(tMin, 0.0);
}

//----------------------
// 월드 좌표에서 SDF 샘플
//----------------------
float SampleSdfWorld(vec3 p)
{
    vec3 size = uVolumeMax - uVolumeMin;
    vec3 uvw  = (p - uVolumeMin) / size;  // 0~1로 매핑

    // 박스 밖이면 큰 양수 리턴해서 멀리 있다고 생각
    if (any(lessThan(uvw, vec3(0.0))) || any(greaterThan(uvw, vec3(1.0))))
        return 1.0;

    return texture(uSdfTex, uvw).r;
}

//----------------------
// SDF gradient로 법선 추정
//----------------------
vec3 EstimateNormal(vec3 p)
{
    float eps = 0.001;
    float dx = SampleSdfWorld(p + vec3(eps,0,0)) - SampleSdfWorld(p - vec3(eps,0,0));
    float dy = SampleSdfWorld(p + vec3(0,eps,0)) - SampleSdfWorld(p - vec3(0,eps,0));
    float dz = SampleSdfWorld(p + vec3(0,0,eps)) - SampleSdfWorld(p - vec3(0,0,eps));
    return normalize(vec3(dx, dy, dz));
}

vec3 ShadeIsoSurface(vec3 p, vec3 rd)
{
    vec3 N = EstimateNormal(p);
    vec3 L = normalize(vec3(0.5, 0.8, 0.3)); // 대충 위에서 비추는 방향광
    vec3 V = -rd;

    float diff = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 64.0);

    vec3 baseColor = vec3(0.2, 0.7, 1.0);
    vec3 color = baseColor * diff + vec3(0.1) + spec * vec3(1.0);

    return color;
}

//----------------------
// main
//----------------------
void main()
{
    // 1. 화면 좌표(-1~1) 만들기
    vec2 ndc = vTexCoord * 2.0 - 1.0;
    vec4 clipPos = vec4(ndc, 0.0, 1.0);

    // 2. clip → world (near plane 상의 점)
    vec4 worldPos = uInvViewProj * clipPos;
    worldPos /= worldPos.w;

    vec3 ro = uCameraPos;
    vec3 rd = normalize(worldPos.xyz - uCameraPos);

    // 3. 레이와 볼륨 박스 교차
    float tEnter, tExit;
    if (!RayBoxIntersect(ro, rd, uVolumeMin, uVolumeMax, tEnter, tExit))
    {
        FragColor = vec4(0.0); // 볼륨 안 지나가면 그냥 검정
        return;
    }

    // 4. ray marching (sphere tracing 방식)
    float t = tEnter;
    const int MAX_STEPS = 128;
    const float EPS = 0.001;

    for (int i = 0; i < MAX_STEPS && t < tExit; ++i)
    {
        vec3 p = ro + rd * t;
        float sdf = SampleSdfWorld(p);

        if (abs(sdf) < EPS)
        {
            vec3 col = ShadeIsoSurface(p, rd);
            FragColor = vec4(col, 1.0);
            return;
        }

        // 너무 작은 스텝으로 무한루프 방지
        float stepLen = max(abs(sdf), 0.001);
        t += stepLen;
    }

    // 못 찾으면 배경
    FragColor = vec4(0.0);
}
