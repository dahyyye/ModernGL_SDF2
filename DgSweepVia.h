#pragma once
#include "DgViewer.h"

class DgVolume;
class DgTrajectory;

namespace DgSweepVia {

    /*!
     *  \brief  Interval Basin Caching (gradient descent 최적화용)
     *
     *  같은 정점을 여러 번 방문할 때 중복 계산 방지
     *  이미 탐색한 t 구간이면 저장된 결과를 바로 반환
     */
    struct IntervalCache {
        std::vector<double> intervals;  // [min0, max0, min1, max1, ...]
        std::vector<double> values;     // 각 구간의 최소 f값
        std::vector<double> minima;     // 각 구간의 t*
    };

	// 정점 데이터
    struct VertexData {
        double f_star = 1e10;           // 최소 SDF값
        double t_star = 0.0;            // 최적 시간
        bool visited = false;           // 방문 여부
        IntervalCache cache;            // basin caching 정보
    };

    // 큐 데이터
    struct QueueData {
        int voxelIdx[3];                // voxel 인덱스 (i, j, k)
        double t_seed;                  // 초기 t값
    };

    // 모서리 구조체
    struct Edge {
        glm::ivec3 v1;  // 시작 정점
        glm::ivec3 v2;  // 끝 정점
    };

    /*!
     *  \brief  초기 시드 찾기
     *
     *  \param[in]  brush           원본 SDF 볼륨
     *  \param[in]  trajectory      이동 궤적
     *  \param[in]  gridMin         결과 볼륨의 최소점
     *  \param[in]  gridSpacing     결과 볼륨의 격자 간격
     *  \param[in]  numTimeSamples  시간 샘플 수 (기본값: 10)
     *  \param[out] seeds           초기 시드 목록
     */
    void findInitialSeeds(
        DgVolume* brush,
        const DgTrajectory& trajectory,
        const glm::vec3& gridMin,
        float gridSpacing,
        int numTimeSamples,
        std::vector<QueueData>& seeds
    );

    /*!
     *  \brief  경사하강법으로 t* 찾기
     *
     *  \param[in]  f           f(t): 시간 t에서의 SDF값
     *  \param[in]  df          df/dt: f의 시간 미분
     *  \param[in]  t0          초기 t값 (이웃에서 전파된 값)
     *  \param[out] f_min        최소 SDF값 f(t*)
     *  \param[out] t_star       최적 시간 t*
     *  \param[in,out] cache    Interval Basin Cache
     */
    void gradientDescent(
        const std::function<double(double)>& f,
        const std::function<double(double)>& df,
        double t0,
        double& f_min,
        double& t_star,
        IntervalCache& cache
    );

    /*!
     *  \brief  Sweep Continuation 메인 알고리즘
     *
     *  \param[in]  brush           원본 SDF 볼륨
     *  \param[in]  trajectory      이동 궤적
     *  \param[in]  gridMin         결과 볼륨의 최소점
     *  \param[in]  gridDim         결과 볼륨의 해상도 (nx, ny, nz)
     *  \param[in]  gridSpacing     결과 볼륨의 격자 간격
     *  \param[in]  seeds           초기 시드 목록
     *  \param[out] vertexData      정점별 데이터 (f_star, t_star)
     *  \param[out] surfaceVoxels   표면을 포함하는 voxel 목록
     */
    void sweepContinuation(
        DgVolume* brush,
        const DgTrajectory& trajectory,
        const glm::vec3& gridMin,
        const glm::ivec3& gridDim,
        float gridSpacing,
        const std::vector<QueueData>& seeds,
        std::vector<VertexData>& vertexData,
        std::vector<std::array<int, 3>>& surfaceVoxels
    );

    /*!
     *  \brief  표면 근처만 채워진 그리드를 레이마칭용 full volume으로 변환
     *
     *  \param[in]    
     *  \param[out] 
     */
    void fillEmptyVoxels();

    /*!
     *  \brief  정점을 공유하는 voxel들 반환
     *
     *  \param[in]  vi, vj, vk      정점 인덱스
     *  \param[out] voxels          해당 정점을 공유하는 voxel들 (최대 8개)
     */
    void getVoxelsIncidentOnVertex(const glm::ivec3& vertex, std::vector<glm::ivec3>& voxels );

    /*!
     *  \brief  모서리를 공유하는 voxel들 반환
     *
     *  \param[in]  v1              모서리 시작 정점 인덱스 [3]
     *  \param[in]  v2              모서리 끝 정점 인덱스 [3]
     *  \param[out] voxels          해당 모서리를 공유하는 voxel들 (최대 4개)
     */
    void getVoxelsIncidentOnEdge(const Edge& edge, std::vector<glm::ivec3>& voxels);

    /*!
     *  \brief  정점 인덱스 → 1D 배열 인덱스 변환
     *
     *  \param[in]  vi, vj, vk      정점 인덱스
     *  \param[in]  dim             볼륨 해상도
     *  \return     1D 배열 인덱스
     */
    inline int vertexToIndex(int vi, int vj, int vk, const glm::ivec3& dim) {
        return vi + vj * dim.x + vk * dim.x * dim.y;
    }

    /*!
     *  \brief  voxel의 8개 코너 정점 인덱스 반환
     *
     *  \param[in]  voxelIdx        voxel 인덱스 [3]
     *  \param[out] corners         8개 코너의 정점 인덱스
     */
    void getVoxelCorners( const int voxelIdx[3], std::array<glm::ivec3, 8>& corners );

    /*!
     *  \brief  voxel의 12개 모서리 반환
     *
     *  \param[in]  voxelIdx        voxel 인덱스 [3]
     *  \param[out] edges           12개 모서리
     */
    void getVoxelEdges( const int voxelIdx[3], std::array<Edge, 12>& edges );
}