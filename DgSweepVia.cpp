#include "DgSweepVia.h"

namespace DgSweepVia
{
	// 초기 시드 찾기
	void findInitialSeeds()
	{

	}

	// 경사하강법으로 t* 찾기
	void gradientDescent(const std::function<double(double)>& f, const std::function<double(double)>& df, double t0, double& f_min, double& t_star, IntervalCache& cache)
	{

	}

	// 메인
	void sweepContinuation(DgVolume* brush, const DgTrajectory& trajectory, const glm::vec3& gridMin, const glm::ivec3& gridDim, float gridSpacing, const std::vector<QueueData>& seeds, std::vector<VertexData>& vertexData, std::vector<std::array<int, 3>>& surfaceVoxels)
	{

	}

	// 후처리: 빈 voxel 채우기
	void fillEmptyVoxels()
	{

	}

	
	// 정점을 공유하는 voxel들 반환
	void getVoxelsIncidentOnVertex(const glm::ivec3& vertex, std::vector<glm::ivec3>& voxels)
	{
		voxels.clear();

		for (int di = -1; di <= 0; di++) {
			for (int dj = -1; dj <= 0; dj++) {
				for (int dk = -1; dk <= 0; dk++) {
					voxels.push_back({ vertex.x + di, vertex.y + dj, vertex.z + dk });
				}
			}
		}
	}

	// 모서리를 공유하는 voxel들 반환
	void getVoxelsIncidentOnEdge(const Edge& edge, std::vector<glm::ivec3>& voxels)
	{
		voxels.clear();

		// 1. 모서리 방향 찾기
		int dx = edge.v2.x - edge.v1.x;
		int dy = edge.v2.y - edge.v1.y;
		int dz = edge.v2.z - edge.v1.z;

		// 2. 모서리 시작점
		int minX = std::min(edge.v1.x, edge.v2.x);
		int minY = std::min(edge.v1.y, edge.v2.y);
		int minZ = std::min(edge.v1.z, edge.v2.z);

		// 3. 방향에 따라 4개 voxel 찾기
		if (dx != 0) {
			for (int dj = -1; dj <= 0; dj++) {
				for (int dk = -1; dk <= 0; dk++) {
					voxels.push_back({ minX, minY + dj, minZ + dk });
				}
			}
		}
		else if (dy != 0) {
			for (int di = -1; di <= 0; di++) {
				for (int dk = -1; dk <= 0; dk++) {
					voxels.push_back({ minX + di, minY, minZ + dk });
				}
			}
		}
		else if (dz != 0) {
			for (int di = -1; di <= 0; di++) {
				for (int dj = -1; dj <= 0; dj++) {
					voxels.push_back({ minX + di, minY + dj, minZ });
				}
			}
		}
	}

	// voxel의 8개 코너 좌표 반환
	void getVoxelCorners(const int voxelIdx[3], std::array<glm::ivec3, 8>& corners)
	{
		int i = voxelIdx[0];
		int j = voxelIdx[1];
		int k = voxelIdx[2];

		corners[0] = { i,     j,     k };
		corners[1] = { i + 1, j,     k };
		corners[2] = { i,     j + 1, k };
		corners[3] = { i + 1, j + 1, k };
		corners[4] = { i,     j,     k + 1 };
		corners[5] = { i + 1, j,     k + 1 };
		corners[6] = { i,     j + 1, k + 1 };
		corners[7] = { i + 1, j + 1, k + 1 };
	}

	// voxel의 12개 모서리 반환
	void getVoxelEdges(const int voxelIdx[3], std::array<Edge, 12>& edges)
	{
		std::array<glm::ivec3, 8> corners;
		getVoxelCorners(voxelIdx, corners);

		edges[0] = { corners[0], corners[1] };
		edges[1] = { corners[2], corners[3] };
		edges[2] = { corners[4], corners[5] };
		edges[3] = { corners[6], corners[7] };

		edges[4] = { corners[0], corners[2] };
		edges[5] = { corners[1], corners[3] };
		edges[6] = { corners[4], corners[6] };
		edges[7] = { corners[5], corners[7] };

		edges[8] = { corners[0], corners[4] };
		edges[9] = { corners[1], corners[5] };
		edges[10] = { corners[2], corners[6] };
		edges[11] = { corners[3], corners[7] };
	}  
}