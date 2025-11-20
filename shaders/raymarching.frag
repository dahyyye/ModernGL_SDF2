#version 330 core

out vec4 outColor; //최종 프래그먼트 색상

//=========================================== SDF 볼륨 유니폼 ===============================================
uniform sampler3D uSDFVolume;
uniform vec3 uVolumeMin;
uniform vec3 uVolumeMax;


//===========================================카메라 조작==============================================

uniform mat4 uView, uProj, uInvView, uInvProj; //카메라 뷰/투영 행렬 및 역행렬
uniform vec2 uResolution; //화면 해상도

float mapSDFd(vec3 p) {
    vec3 volumeRange = uVolumeMax - uVolumeMin;
    vec3 uvw = (p - uVolumeMin) / volumeRange;
    
    if (any(lessThan(uvw, vec3(0.0))) || any(greaterThan(uvw, vec3(1.0)))) {
        return 500.0;
    }
    return texture(uSDFVolume, uvw).r; 
}

vec3 mapColor(vec3 p) {
    return vec3(0.1, 0.4, 0.8);
}

// 화면 좌표에서 월드 공간의 광선 방향 얻기
vec3 getRayDir(vec2 fragCoord){
    // 화면 좌표 (fragCoord)를 NDC (Normalized Device Coordinates) [-1, 1]로 변환
    vec2 ndc = (fragCoord / uResolution) * 2.0 - 1.0; 
    
    // NDC를 클립 공간 좌표로 변환 (z=-1, w=1)
    vec4 clip = vec4(ndc, -1.0, 1.0);
    
    // 클립 좌표를 Eye(View) 공간 좌표로 변환 (역투영 행렬 사용)
    vec4 eye = uInvProj * clip;
    
    // Ray Direction을 계산하기 위해 z=-1, w=0 (무한대의 점)으로 설정
    eye = vec4(eye.xy, -1.0, 0.0);
    
    // Eye 공간에서 World 공간으로 변환하고 정규화
    return normalize((uInvView * eye).xyz);
}

// 카메라 위치 얻기
vec3 getCamPos(){ return (uInvView * vec4(0,0,0,1)).xyz; }

// 법선 계산
vec3 calcNormal(vec3 p){
    const float e = 1.5e-3;
    vec2 k = vec2(1,-1);
    return normalize(
         k.xyy * mapSDFd(p + k.xyy*e) +
        k.yyx * mapSDFd(p + k.yyx*e) +
        k.yxy * mapSDFd(p + k.yxy*e) +
        k.xxx * mapSDFd(p + k.xxx*e)
    );
}





//============================================레이마칭================================================

struct Hit{ 
    bool hit;                                           // 히트 여부
    vec3 hitPoint;                                      // 히트 위치 
    vec3 n;                                             // 히트 위치의 법선
    vec3 color;                                         // 히트 위치의 색상
};

Hit raymarch(vec3 rayOrigin, vec3 rayDir){                // rayOrigin: 광선 시작점, rd: 광선 방향
    float t= 0.0;                                         // t: 광선 거리 누적 변수
    const float EPS = 1e-3;                               // 히트 허용 오차
    const float MIN_STEP = 1e-4;                          // 최소 스텝 크기

    for(int i = 0; i < 256; i++) {
        vec3 p = rayOrigin + rayDir * t;
        float d = mapSDFd(p);

        if (d < EPS) {                                    // 히트 발생
            vec3 n = calcNormal(p);
            vec3 c = mapColor(p);
            return Hit(true, p, n, c);
        }

        t += max(d, MIN_STEP);

        if(t > 500.0) break;                              // 최대 거리 초과 시 종료
    }
    return Hit(false, vec3(0.0), vec3(0.0), vec3(0.0));
}
float depthFromWorld(vec3 worldPos) {
    // 월드 좌표를 클립 공간 좌표로 변환: ClipPos = uProj * uView * WorldPos
    //    (uView, uProj 유니폼은 DgScene.cpp에서 전달됩니다.)
    vec4 clipPos = uProj * uView * vec4(worldPos, 1.0);
    
    // 클립 좌표를 NDC (Normalized Device Coordinates)로 변환: z/w
    //    NDC z 값은 [-1, 1] 범위입니다.
    float ndcZ = clipPos.z / clipPos.w;
    
    // NDC z 값을 [0, 1] 범위의 최종 깊이 값으로 변환하여 반환
    return (ndcZ * 0.5) + 0.5;
}

//=========================================메인 함수====================================================

void main(){
    vec3 rayOrigin = getCamPos();
    vec3 rayDir = getRayDir(gl_FragCoord.xy);
    
    Hit hit = raymarch(rayOrigin, rayDir);                          // 레이마칭으로 히트 검사

    if(!hit.hit) { discard; }                                       // 히트 없으면 프래그먼트 버림

    gl_FragDepth = depthFromWorld(hit.hitPoint);                    // 깊이 버퍼에 표면 깊이 기록
    
    vec3 V = normalize(rayOrigin - hit.hitPoint);                   // 간단 셰이딩(색은 surf.color 사용)
    vec3 L = V;                                                     // 광원 방향 (뷰어 방향과 동일)
    vec3 H = normalize(L+V);  

    float diff = max(dot(hit.n, L), 0.0);
    float spec = pow(max(dot(hit.n, H),0.0),64.0);

    vec3 ambient = 0.20 * hit.color;
    vec3 color   = ambient + diff * hit.color + spec * vec3(1.0);


    outColor = vec4(color, 1.0);
}