#version 330 core

//=============================================================================
// 출력 변수
//=============================================================================
out vec4 outColor;  // 최종 프래그먼트 색상

//=============================================================================
// 유니폼 변수
//=============================================================================
// SDF 볼륨 관련
uniform sampler3D uSDFVolume;   // 3D SDF 텍스처
uniform vec3 uVolumeMin;        // 볼륨 바운딩 박스 최소점
uniform vec3 uVolumeMax;        // 볼륨 바운딩 박스 최대점

// 카메라 관련
uniform mat4 uView;             // 뷰 행렬
uniform mat4 uProj;             // 투영 행렬
uniform mat4 uInvView;          // 뷰 행렬의 역행렬
uniform mat4 uInvProj;          // 투영 행렬의 역행렬
uniform vec2 uResolution;       // 화면 해상도

//=============================================================================
// 상수 정의
//=============================================================================
#define MAX_STEPS 256           // 레이마칭 최대 반복 횟수
#define EPS 0.001              // 표면 히트 판정 허용 오차

//=============================================================================
// 레이-AABB 교차 검사
//=============================================================================
// 광선과 축 정렬 바운딩 박스(AABB)의 교차점을 계산합니다.
// 반환값: vec2(tNear, tFar) - 광선이 박스에 들어가는/나가는 거리
// tNear > tFar 이면 교차하지 않음
vec2 intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) {
    // 각 축에 대해 박스 면과의 교차 거리 계산
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    
    // rayDir이 음수인 경우를 처리하기 위해 min/max 재정렬
    vec3 t1 = min(tMin, tMax);  // 각 축의 가까운 교차점
    vec3 t2 = max(tMin, tMax);  // 각 축의 먼 교차점
    
    // 모든 축에서 가장 늦게 들어가는 점 = 박스 진입점
    float tNear = max(max(t1.x, t1.y), t1.z);
    // 모든 축에서 가장 먼저 나가는 점 = 박스 탈출점
    float tFar = min(min(t2.x, t2.y), t2.z);
    
    return vec2(tNear, tFar);
}

//=============================================================================
// SDF 샘플링 함수
//=============================================================================
// 월드 좌표 p에서 SDF 값을 샘플링합니다.
// 반환값: 해당 위치에서 가장 가까운 표면까지의 부호 있는 거리
float mapSDFd(vec3 p) {
    vec3 volumeRange = uVolumeMax - uVolumeMin;
    vec3 uvw = (p - uVolumeMin) / volumeRange;
    
    // 범위 밖이면 clamp하여 샘플링 (경계 값 사용)
    uvw = clamp(uvw, vec3(0.001), vec3(0.999));
    
    return texture(uSDFVolume, uvw).r;
}

// 범위 체크 포함 버전 (레이마칭용)
float mapSDFdWithBoundsCheck(vec3 p) {
    vec3 volumeRange = uVolumeMax - uVolumeMin;
    vec3 uvw = (p - uVolumeMin) / volumeRange;
    
    if (any(lessThan(uvw, vec3(0.0))) || any(greaterThan(uvw, vec3(1.0)))) {
        return 1.0;
    }
    
    return texture(uSDFVolume, uvw).r;
}

//=============================================================================
// 카메라 유틸리티 함수
//=============================================================================
// 화면 좌표에서 월드 공간의 광선 방향을 계산합니다.
vec3 getRayDir(vec2 fragCoord) {
    // 1. 화면 좌표를 NDC(-1 ~ 1)로 변환
    vec2 ndc = (fragCoord / uResolution) * 2.0 - 1.0;
    
    // 2. NDC를 클립 공간 좌표로 변환
    vec4 clip = vec4(ndc, -1.0, 1.0);
    
    // 3. 클립 좌표를 뷰(Eye) 공간으로 변환
    vec4 eye = uInvProj * clip;
    eye = vec4(eye.xy, -1.0, 0.0);  // 방향 벡터로 설정 (w=0)
    
    // 4. 뷰 공간에서 월드 공간으로 변환 후 정규화
    return normalize((uInvView * eye).xyz);
}

// 카메라 월드 좌표를 반환합니다.
vec3 getCamPos() { 
    // 역뷰 행렬의 이동 성분 = 카메라 위치
    return (uInvView * vec4(0, 0, 0, 1)).xyz; 
}

//=============================================================================
// 법선 계산 함수
//=============================================================================
// SDF의 수치적 그래디언트를 이용하여 표면 법선을 계산합니다.
// 테트라헤드론 기법 사용 (4번의 샘플링으로 정확한 그래디언트 계산)
vec3 calcNormal(vec3 p) {
    // 볼륨 크기에 비례한 e 값 계산
    vec3 volumeRange = uVolumeMax - uVolumeMin;
    float minRange = min(min(volumeRange.x, volumeRange.y), volumeRange.z);
    float e = minRange / 128.0;  // 볼륨 크기의 1/128
    
    // 중앙 차분법 사용 (더 안정적)
    float dx = mapSDFd(p + vec3(e, 0, 0)) - mapSDFd(p - vec3(e, 0, 0));
    float dy = mapSDFd(p + vec3(0, e, 0)) - mapSDFd(p - vec3(0, e, 0));
    float dz = mapSDFd(p + vec3(0, 0, e)) - mapSDFd(p - vec3(0, 0, e));
    
    vec3 n = vec3(dx, dy, dz);
    
    // NaN 방지
    float len = length(n);
    if (len < 0.0001) {
        return vec3(0.0, 1.0, 0.0);  // 기본 법선
    }
    
    return n / len;
}

//=============================================================================
// 메인 함수
//=============================================================================
void main() {

    // 1. 광선 설정
    vec3 rayOrigin = getCamPos();
    vec3 rayDir = getRayDir(gl_FragCoord.xy);

    // 2. 레이-박스 교차 검사
    vec2 tHit = intersectAABB(rayOrigin, rayDir, uVolumeMin, uVolumeMax);
    
    if (tHit.x > tHit.y || tHit.y < 0.0) {
        discard;
    }
    
    float t = max(tHit.x, 0.0) + 0.001;  // 약간 안쪽에서 시작
    float tEnd = tHit.y;

    // 3. 레이마칭 (Sphere Tracing)
    bool hit = false;
    vec3 hitPoint;
    
    for (int i = 0; i < MAX_STEPS; i++) {
        if (t > tEnd) break;
        
        vec3 p = rayOrigin + rayDir * t;
        float d = mapSDFdWithBoundsCheck(p);
        
        // 표면 근처 판정 (절대값 사용)
        if (abs(d) < EPS) {
            hit = true;
            hitPoint = p;
            break;
        }
        
        // SDF 값만큼 전진 (절대값 사용, 음수 SDF도 처리)
        t += max(abs(d) * 0.5, EPS);  // 0.5 계수로 오버슈팅 방지
    }
    
    if (!hit) { 
        discard; 
    }

    // 4. 셰이딩
    vec3 n = calcNormal(hitPoint);
    vec3 baseColor = vec3(0.8, 0.6, 0.4);
    vec3 V = normalize(rayOrigin - hitPoint);
    vec3 L = V;
    vec3 H = normalize(L + V);
    
    // 양면 조명
    float NdotL = dot(n, L);
    if (NdotL < 0.0) {
        n = -n;  // 법선 뒤집기
        NdotL = -NdotL;
    }
    
    float diff = max(NdotL, 0.0);
    float spec = pow(max(dot(n, H), 0.0), 32.0);
    
    vec3 ambient = 0.2 * baseColor;
    vec3 finalColor = ambient + diff * baseColor + spec * vec3(0.3);

    // 5. 깊이 버퍼 설정
    vec4 clipPos = uProj * uView * vec4(hitPoint, 1.0);
    float ndcZ = clipPos.z / clipPos.w;
    gl_FragDepth = (ndcZ * 0.5) + 0.5;

    // 6. 최종 출력
    outColor = vec4(finalColor, 1.0);
}