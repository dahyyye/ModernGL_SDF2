#version 330 core

//=============================================================================
// 출력 변수
//=============================================================================
out vec4 outColor;

//=============================================================================
// 유니폼 변수
//=============================================================================

// SDF 볼륨 관련
uniform sampler3D uSDFVolume;   // 3D SDF 텍스처
uniform vec3 uVolumeMin;        // 볼륨 AABB 최소점 (로컬 좌표)
uniform vec3 uVolumeMax;        // 볼륨 AABB 최대점 (로컬 좌표)

// 카메라 관련
uniform mat4 uView;             // 뷰 행렬
uniform mat4 uProj;             // 투영 행렬
uniform mat4 uInvView;          // 뷰 행렬의 역행렬
uniform mat4 uInvProj;          // 투영 행렬의 역행렬
uniform vec2 uResolution;       // 화면 해상도

// [추가] 모델 변환 관련
uniform mat4 uModel;            // 모델 행렬 (이동 + 회전)
uniform mat4 uModelInverse;     // 모델 행렬의 역행렬

//=============================================================================
// 상수 정의
//=============================================================================
#define MAX_STEPS 256
#define EPS 0.001

//=============================================================================
// 레이-AABB 교차 검사
//-----------------------------------------------------------------------------
// 광선과 축 정렬 바운딩 박스(AABB)의 교차점을 계산
// 반환값: vec2(tNear, tFar)
// tNear > tFar 이면 교차하지 않음
//=============================================================================
vec2 intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) 
{
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar  = min(min(t2.x, t2.y), t2.z);
    
    return vec2(tNear, tFar);
}

//=============================================================================
// SDF 샘플링 함수
//-----------------------------------------------------------------------------
// 로컬 좌표 p에서 SDF 값을 샘플링
//=============================================================================
float mapSDF(vec3 p) 
{
    vec3 volumeRange = uVolumeMax - uVolumeMin;
    vec3 uvw = (p - uVolumeMin) / volumeRange;
    uvw = clamp(uvw, vec3(0.001), vec3(0.999));
    return texture(uSDFVolume, uvw).r;
}

//=============================================================================
// SDF 샘플링 (범위 체크 포함)
//-----------------------------------------------------------------------------
// 볼륨 범위 밖이면 1.0 반환 (레이마칭 탈출용)
//=============================================================================
float mapSDFWithBoundsCheck(vec3 p) 
{
    vec3 volumeRange = uVolumeMax - uVolumeMin;
    vec3 uvw = (p - uVolumeMin) / volumeRange;
    
    if (any(lessThan(uvw, vec3(0.0))) || any(greaterThan(uvw, vec3(1.0)))) {
        return 1.0;
    }
    return texture(uSDFVolume, uvw).r;
}

//=============================================================================
// 카메라 위치 반환
//=============================================================================
vec3 getCameraPosition() 
{ 
    return (uInvView * vec4(0.0, 0.0, 0.0, 1.0)).xyz; 
}

//=============================================================================
// 화면 좌표에서 월드 공간 광선 방향 계산
//=============================================================================
vec3 getRayDirection(vec2 fragCoord) 
{
    // NDC 좌표로 변환
    vec2 ndc = (fragCoord / uResolution) * 2.0 - 1.0;
    
    // 클립 → 뷰 → 월드 변환
    vec4 clip = vec4(ndc, -1.0, 1.0);
    vec4 eye  = uInvProj * clip;
    eye = vec4(eye.xy, -1.0, 0.0);
    
    return normalize((uInvView * eye).xyz);
}

//=============================================================================
// 법선 계산 (중앙 차분법)
//-----------------------------------------------------------------------------
// 로컬 좌표에서 그래디언트를 계산하고 월드 공간으로 변환
//=============================================================================
vec3 calcNormal(vec3 p) 
{
    vec3 volumeRange = uVolumeMax - uVolumeMin;
    float minRange = min(min(volumeRange.x, volumeRange.y), volumeRange.z);
    float e = minRange / 128.0;
    
    // 중앙 차분법으로 그래디언트 계산
    float dx = mapSDF(p + vec3(e, 0, 0)) - mapSDF(p - vec3(e, 0, 0));
    float dy = mapSDF(p + vec3(0, e, 0)) - mapSDF(p - vec3(0, e, 0));
    float dz = mapSDF(p + vec3(0, 0, e)) - mapSDF(p - vec3(0, 0, e));
    
    vec3 n = vec3(dx, dy, dz);
    float len = length(n);
    
    if (len < 0.0001) {
        return vec3(0.0, 1.0, 0.0);
    }
    
    // 로컬 법선을 월드 공간으로 변환
    vec3 localNormal = n / len;
    vec3 worldNormal = normalize(mat3(uModel) * localNormal);
    
    return worldNormal;
}

//=============================================================================
// 메인 함수
//=============================================================================
void main() 
{
    //=========================================================================
    // 1. 월드 공간에서 광선 설정
    //=========================================================================
    vec3 worldRayOrigin = getCameraPosition();
    vec3 worldRayDir    = getRayDirection(gl_FragCoord.xy);

    //=========================================================================
    // 2. 월드 레이를 로컬 공간으로 변환
    //-------------------------------------------------------------------------
    // 볼륨이 회전되어 있으면, 레이를 역방향으로 회전해서
    // 로컬 공간에서 레이마칭을 수행해야 함
    //=========================================================================
    vec3 localRayOrigin = (uModelInverse * vec4(worldRayOrigin, 1.0)).xyz;
    vec3 localRayDir    = normalize((uModelInverse * vec4(worldRayDir, 0.0)).xyz);

    //=========================================================================
    // 3. 로컬 공간에서 레이-박스 교차 검사
    //=========================================================================
    vec2 tHit = intersectAABB(localRayOrigin, localRayDir, uVolumeMin, uVolumeMax);
    
    if (tHit.x > tHit.y || tHit.y < 0.0) {
        discard;
    }
    
    float t    = max(tHit.x, 0.0) + 0.001;
    float tEnd = tHit.y;

    //=========================================================================
    // 4. 레이마칭 (Sphere Tracing)
    //-------------------------------------------------------------------------
    // 로컬 공간에서 SDF를 샘플링하며 표면을 찾음
    //=========================================================================
    bool hit = false;
    vec3 localHitPoint;
    
    for (int i = 0; i < MAX_STEPS; i++) 
    {
        if (t > tEnd) break;
        
        vec3 p = localRayOrigin + localRayDir * t;
        float d = mapSDFWithBoundsCheck(p);
        
        if (abs(d) < EPS) {
            hit = true;
            localHitPoint = p;
            break;
        }
        
        t += max(abs(d) * 0.5, EPS);
    }
    
    if (!hit) { 
        discard; 
    }

    //=========================================================================
    // 5. 로컬 히트 포인트를 월드 공간으로 변환
    //=========================================================================
    vec3 worldHitPoint = (uModel * vec4(localHitPoint, 1.0)).xyz;

    //=========================================================================
    // 6. 셰이딩 (Phong 조명)
    //=========================================================================
    vec3 N = calcNormal(localHitPoint);
    vec3 V = normalize(worldRayOrigin - worldHitPoint);
    vec3 L = V;  // 헤드라이트 조명
    vec3 H = normalize(L + V);
    
    // 양면 조명
    float NdotL = dot(N, L);
    if (NdotL < 0.0) {
        N = -N;
        NdotL = -NdotL;
    }
    
    // 조명 계산
    vec3 baseColor = vec3(0.8, 0.6, 0.4);
    float diff = max(NdotL, 0.0);
    float spec = pow(max(dot(N, H), 0.0), 32.0);
    
    vec3 ambient  = 0.2 * baseColor;
    vec3 diffuse  = diff * baseColor;
    vec3 specular = spec * vec3(0.3);
    
    vec3 finalColor = ambient + diffuse + specular;

    //=========================================================================
    // 7. 깊이 버퍼 설정
    //=========================================================================
    vec4 clipPos = uProj * uView * vec4(worldHitPoint, 1.0);
    float ndcZ = clipPos.z / clipPos.w;
    gl_FragDepth = (ndcZ * 0.5) + 0.5;

    //=========================================================================
    // 8. 최종 출력
    //=========================================================================
    outColor = vec4(finalColor, 1.0);
}
