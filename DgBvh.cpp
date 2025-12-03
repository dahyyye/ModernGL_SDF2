#include "DgViewer.h"

using Dist2BV = std::pair<double, DgBvh::AaBb*>; // 거리, 노드

/********************/
/* AaBb 클래스 구현 */
/********************/
DgBvh::AaBb::AaBb()
{
	mMesh = NULL;
	mMin = DgPos(DBL_MAX, DBL_MAX, DBL_MAX);
	mMax = DgPos(-DBL_MAX, -DBL_MAX, -DBL_MAX);
	mDepth = -1;
	mIsLeaf = false;
}

DgBvh::AaBb::AaBb(DgMesh* pMesh, std::vector<DgFace*>& Faces, const int Depth)
{
	// 대상 메쉬와 삼각형 리스트를 복사한다.
	mMesh = pMesh;
	mFaces = Faces;

	// 삼각형 리스트를 포함하는 AaBb 타입 경계 상자를 구한다.
	mMin = DgPos(DBL_MAX, DBL_MAX, DBL_MAX);
	mMax = DgPos(-DBL_MAX, -DBL_MAX, -DBL_MAX);
	for (DgFace* f : mFaces)
	{
		for (int i = 0; i < 3; ++i)
		{
			DgPos p = f->getVertexPos(i);
			mMin[0] = MIN(mMin[0], p[0]);
			mMin[1] = MIN(mMin[1], p[1]);
			mMin[2] = MIN(mMin[2], p[2]);
			mMax[0] = MAX(mMax[0], p[0]);
			mMax[1] = MAX(mMax[1], p[1]);
			mMax[2] = MAX(mMax[2], p[2]);
		}
	}

	// 레벨과 리프 노드 여부를 결정한다.
	mDepth = Depth;
	mIsLeaf = (mDepth == MAX_BVH_DEPTH || Faces.size() < 6) ? true : false;

	// 리프 노드가 아니라면 현재 경계 상자를 4개로 분할한다.
	if (!mIsLeaf)
		divide();
}

DgBvh::AaBb::~AaBb()
{
	for (AaBb* pNode : mChildNodes)
		delete pNode;
}

void DgBvh::AaBb::divide()
{
	// 경계 상자의 각 축의 길이를 구하여
	double dx = mMax[0] - mMin[0];
	double dy = mMax[1] - mMin[1];
	double dz = mMax[2] - mMin[2];

	// 분할할 두 축(axis0, axis1)과 중심(c0, c1)을 구한다.
	int axis0 = 1, axis1 = 2;
	if (dy < dx && dy < dz)
	{
		axis0 = 2;
		axis1 = 0;
	}
	else if (dz < dx && dz < dy)
	{
		axis0 = 0;
		axis1 = 1;
	}
	double c0 = (mMin[axis0] + mMax[axis0]) / 2.0;
	double c1 = (mMin[axis1] + mMax[axis1]) / 2.0;

	// 삼각형 리스트를 4개의 영역으로 분할한다.
	std::vector<DgFace*> Faces0;
	std::vector<DgFace*> Faces1;
	std::vector<DgFace*> Faces2;
	std::vector<DgFace*> Faces3;
	for (DgFace* f : mFaces)
	{
		DgPos p = f->getVertexPos(0);
		if (p[axis0] <= c0 && p[axis1] <= c1)
			Faces0.push_back(f);
		else if (p[axis0] > c0 && p[axis1] <= c1)
			Faces1.push_back(f);
		else if (p[axis0] > c0 && p[axis1] > c1)
			Faces2.push_back(f);
		else if (p[axis0] <= c0 && p[axis1] > c1)
			Faces3.push_back(f);
		else
			printf("BVH construction error...\n");
	}

	// 4개의 영역에 대한 하위 경계 상자를 생성한다.
	mChildNodes.push_back(new AaBb(mMesh, Faces0, mDepth + 1));
	mChildNodes.push_back(new AaBb(mMesh, Faces1, mDepth + 1));
	mChildNodes.push_back(new AaBb(mMesh, Faces2, mDepth + 1));
	mChildNodes.push_back(new AaBb(mMesh, Faces3, mDepth + 1));
}

double DgBvh::AaBb::getSqrDist(const DgPos& p)
{
	DgPos Mid = ::lerp(mMin, mMax, 0.5);
	double dx = ABS(Mid[0] - p[0]) - (mMax[0] - mMin[0]) / 2.0;
	double dy = ABS(Mid[1] - p[1]) - (mMax[1] - mMin[1]) / 2.0;
	double dz = ABS(Mid[2] - p[2]) - (mMax[2] - mMin[2]) / 2.0;
	bool bx = (dx < 0.0) ? true : false;
	bool by = (dy < 0.0) ? true : false;
	bool bz = (dz < 0.0) ? true : false;

	// 점이 상자 내부의 점이라면 0을 반환한다.
	if (bx && by && bz)
		return 0.0;

	// 거리 제곱을 반환한다.
	dx = bx ? 0.0 : dx;
	dy = by ? 0.0 : dy;
	dz = bz ? 0.0 : dz;
	return (dx * dx + dy * dy + dz * dz);
}

bool DgBvh::AaBb::intersectWithTri(const DgPos& p0, const DgPos& p1, const DgPos& p2)
{
	return ::intersect_tri_box(p0, p1, p2, mMin, mMax);
}

/*********************/
/* DgBvh 클래스 구현 */
/*********************/
DgBvh::DgBvh()
{
	mMesh = NULL;
	mRoot = NULL;
	mCopy = false;
}

DgBvh::DgBvh(DgMesh* pMesh)
{
	clock_t st = clock();
	mMesh = pMesh;
	mRoot = new AaBb(pMesh, pMesh->mFaces, 0);
	mCopy = false;
	clock_t ed = clock();
	//printf("BVH construction time = %u ms \n", ed - st);
}

DgBvh::DgBvh(std::vector<DgFace*>& subFaces)
{
	mMesh = subFaces.front()->getMesh();
	mRoot = new AaBb(mMesh, subFaces, 0);
	mCopy = false;
}

DgBvh::DgBvh(std::vector<DgPos>& Verts, std::vector<int>& Faces)
{
	// 메쉬를 직접 생성하여
	mCopy = true;
	mMesh = new DgMesh("Target Mesh");

	// 정점을 추가한다.
	for (DgPos& p : Verts)
		mMesh->addVertex(::create_vertex(p[0], p[1], p[2]));

	// 삼각형을 추가한다.
	for (int i = 0; i < (int)Faces.size(); i += 3)
	{
		DgVertex* v0 = &mMesh->mVerts[Faces[i]];
		DgVertex* v1 = &mMesh->mVerts[Faces[i + 1]];
		DgVertex* v2 = &mMesh->mVerts[Faces[i + 2]];
		mMesh->addFace(::create_face(v0, v1, v2));
	}

	// 메쉬의 연결 정보를 초기화 한다.
	mMesh->updateEdgeMate();
	mMesh->updateBndBox();
	mMesh->updateNormal(DgMesh::NORMAL_FACE);

	// BVH를 생성한다.
	clock_t st = clock();
	mRoot = new AaBb(mMesh, mMesh->mFaces, 0);
	clock_t ed = clock();
	printf("BVH construction time = %u ms \n", ed - st);
}

DgBvh::~DgBvh()
{
	if (mRoot != NULL)
		delete mRoot;
	mRoot = NULL;

	if (mCopy)
		delete mMesh;
	mMesh = NULL;
}

bool DgBvh::isCloserThan(double dist, const DgPos& p, bool bVertOnly)
{
	// 최소 힙: 작은 거리 순으로 정렬됨
	std::priority_queue<Dist2BV, std::vector<Dist2BV>, std::greater<Dist2BV>> minQ;
	minQ.push(std::make_pair(0.0, mRoot));

	double sqrDist = SQR(dist);
	while (!minQ.empty())
	{
		// 가장 가까운 노드 꺼냄
		auto [dist2Box, box] = minQ.top();
		minQ.pop();

		// 거리 체크
		if (dist2Box > sqrDist)
			continue;

		// 리프 노드라면 삼각형 거리 측정
		if (box->mIsLeaf)
		{
			for (DgFace* f : box->mFaces)
			{
				double d = (bVertOnly) ? SQRT(dist_sq_vert(p, f)) : SQRT(dist_sq(p, f));
				if (d < dist)
					return true;
			}
		}
		else
		{
			for (AaBb* child : box->mChildNodes)
				minQ.emplace(child->getSqrDist(p), child);
		}
	}
	return false;
}

double DgBvh::computeDistance(DgPos p, DgPos& q, DgFace** pFace, bool bSigned)
{
	// 최소 힙: 작은 거리 순으로 정렬됨
	std::priority_queue<Dist2BV, std::vector<Dist2BV>, std::greater<Dist2BV>> minQ;
	minQ.push(std::make_pair(0.0, mRoot));

	// 점과 메쉬의 최단 거리를 구한다.
	double sqr_min_d = DBL_MAX;
	while (!minQ.empty())
	{
		// 가장 가까운 경계 상자를 꺼내어
		auto [dist2Box, box] = minQ.top();
		minQ.pop();

		// 경계 상자까지의 거리가 현재까지 최단 거리보다 길다면 스킵한다.
		if (dist2Box > sqr_min_d)
			continue;

		// 리프 노드 경계 상자라면 삼각형과의 거리를 계산한다.
		if (box->mIsLeaf)
		{
			for (DgFace* f : box->mFaces)
			{
				double area = f->getArea();
				if (EQ_ZERO(area, 1.0e-7))
					continue;

				DgPos tmp;
				double d = dist_sq(p, f, tmp);
				if (d < sqr_min_d)
				{
					sqr_min_d = d;
					q = tmp;
					*pFace = f;
				}
			}
		}
		else
		{
			for (AaBb* child : box->mChildNodes)
				minQ.emplace(child->getSqrDist(p), child);
		}
	}

	// 계산된 최소 거리를 반환한다.
	if (bSigned)
	{
		DgVec3 n;
		MgPos q1(*pFace, q);
		if (q1.isOnVertex())
		{
			DgVertex* v = q1.getClosestVert();
			n = v->getAvgNormal();
		}
		else if (q1.isOnEdge())
		{
			DgEdge* e = q1.getEdgeOverPt();
			DgVec3 n1 = SV(e)->getAvgNormal();
			DgVec3 n2 = EV(e)->getAvgNormal();
			n = (n1 + n2).normalize();
		}
		else
		{
			n = (*pFace)->getFaceNormal(false);
		}
		DgVec3 qp = (p - q).normalize();
		if (n * qp < 0.0 && !(*pFace)->isBndryFace())
			return -SQRT(sqr_min_d);
		else
			return SQRT(sqr_min_d);
	}
	else
		return SQRT(sqr_min_d);
}

double DgBvh::computeDistance(DgPos p, DgPos& q, int& fidx, bool sign)
{
	DgFace* f;
	double d = computeDistance(p, q, &f, sign);
	fidx = f->mIdx;
	return d;
}

int DgBvh::intersectWithTri(const DgPos& u0, const DgPos& u1, const DgPos& u2, std::vector<DgFace*>& hitFaces)
{
	// 삼각형이 BV 루트 경계 상자와 교차하지 않으면 0를 반환한다.
	if (!mRoot->intersectWithTri(u0, u1, u2))
		return 0;

	// BFS 탐색을 위한 경계 상자를 저장하는 queue를 생성
	std::queue<DgBvh::AaBb*> boxQueue;
	boxQueue.push(mRoot);

	// BVH를 BFS 방식으로 탐색하여 교차 삼각형을 구한다.
	while (!boxQueue.empty())
	{
		DgBvh::AaBb* pBox = boxQueue.front();
		boxQueue.pop();

		if (pBox->mIsLeaf)
		{
			for (DgFace* f : pBox->mFaces)
			{
				DgPos v0 = f->getVertexPos(0);
				DgPos v1 = f->getVertexPos(1);
				DgPos v2 = f->getVertexPos(2);
				DgPos p, q;
				if (intersect_tri_tri(u0, u1, u2, v0, v1, v2, p, q))
					hitFaces.push_back(f);
			}
		}
		else
		{
			for (int i = 0; i < 4; ++i)
				if (pBox->mChildNodes[i]->intersectWithTri(u0, u1, u2))
					boxQueue.push(pBox->mChildNodes[i]);
		}
	}

	return static_cast<int>(hitFaces.size());
}

double dist_sq_vert(const DgPos& p, DgFace* f)
{
	double min_d = dist_sq(p, f->getVertexPos(0));
	min_d = MIN(min_d, dist_sq(p, f->getVertexPos(1)));
	min_d = MIN(min_d, dist_sq(p, f->getVertexPos(2)));
	return min_d;
}

double dist_sq(const DgPos& q, DgFace* f)
{
	// 삼각형의 세 점과 차이 벡터를 구한다.
	DgPos p0 = f->getVertexPos(0);
	DgPos p1 = f->getVertexPos(1);
	DgPos p2 = f->getVertexPos(2);
	DgVec3 v01 = p1 - p0;
	DgVec3 v12 = p2 - p1;
	DgVec3 v20 = p0 - p2;

	// Case 1: 정점에서 최단 거리가 생기는 경우
	bool p01q, p02q, p12q, p10q, p20q, p21q;
	// true if order of points is q -> p0 -> p1 (about the axis v01)
	p01q = (v01 * (q - p0) < 0.0);
	// true if order of points is q -> p0 -> p2 (about the axis p13)
	p02q = (v20 * (q - p0) > 0.0);
	// true if order of points is q -> p1 -> p2 (about the axis v12)
	p12q = (v12 * (q - p1) < 0.0);
	// true if order of points is q -> p1 -> p0 (about the axis p21)
	p10q = (v01 * (q - p1) > 0.0);
	// true if order of points is q -> p2 -> p0 (about the axis v20)
	p20q = (v20 * (q - p2) < 0.0);
	// true if order of points is q -> p2 -> p1 (about the axis p32)
	p21q = (v12 * (q - p2) > 0.0);

	// p0 에서 최단 거리 발생
	if (p01q && p02q)
	{
		return dist_sq(q, p0);
	}
	else if (p10q && p12q)		// p1 에서 최단 거리 발생
	{
		return dist_sq(q, p1);
	}
	else if (p20q && p21q)		// p2 에서 최단 거리 발생
	{
		return dist_sq(q, p2);
	}

	// Case 2: 에지에서 최단 거리가 발생하는 경우
	DgVec3 n = (v20 ^ v01).normalize();
	DgVec3 p0q = v01 ^ n; // outward direction of edge(p0, p1)
	DgVec3 p1q = v12 ^ n; // outward direction of edge(p1, p2)
	DgVec3 p2q = v20 ^ n; // outward direction of edge(p2, p0)

	// 에지 (p0, p1)에서 최단 거리가 발생하는 경우
	if (!p01q && !p10q && (p0q * (q - p0) > 0.0))
	{
		p0q.normalize();
		v01.normalize();
		return ((p0q * (q - p0)) * (p0q * (q - p0)) + (n * (q - p0)) * (n * (q - p0)));
	}
	// 에지 (p1, p2)에서 최단 거리가 발생하는 경우
	else if (!p12q && !p21q && (p1q * (q - p1) > 0.0))
	{
		p1q.normalize();
		v12.normalize();
		return ((p1q * (q - p1)) * (p1q * (q - p1)) + (n * (q - p1)) * (n * (q - p1)));
	}
	// 에지 (p2, p0)에서 최단 거리가 발생하는 경우
	else if (!p20q && !p02q && (p2q * (q - p2) > 0.0))
	{
		p2q.normalize();
		v20.normalize();
		return ((p2q * (q - p2)) * (p2q * (q - p2)) + (n * (q - p2)) * (n * (q - p2)));
	}

	// Case 3: 삼각형 내부에서 최단 거리가 발생하는 경우
	return (n * (q - p0)) * (n * (q - p0));
}

double dist_sq(const DgPos& q, DgFace* f, DgPos& r)
{
	// 삼각형의 세 점과 차이 벡터를 구한다.
	DgPos p0 = f->getVertexPos(0);
	DgPos p1 = f->getVertexPos(1);
	DgPos p2 = f->getVertexPos(2);
	DgVec3 v01 = (p1 - p0).normalize();
	DgVec3 v12 = (p2 - p1).normalize();
	DgVec3 v20 = (p0 - p2).normalize();

	// Case 1: 정점에서 최단 거리가 생기는 경우
	bool p01q, p02q, p12q, p10q, p20q, p21q;
	// true if order of points is q -> p0 -> p1 (about the axis v01)
	p01q = (v01 * (q - p0) < 0.0);
	// true if order of points is q -> p0 -> p2 (about the axis p13)
	p02q = (v20 * (q - p0) > 0.0);
	// true if order of points is q -> p1 -> p2 (about the axis v12)
	p12q = (v12 * (q - p1) < 0.0);
	// true if order of points is q -> p1 -> p0 (about the axis p21)
	p10q = (v01 * (q - p1) > 0.0);
	// true if order of points is q -> p2 -> p0 (about the axis v20)
	p20q = (v20 * (q - p2) < 0.0);
	// true if order of points is q -> p2 -> p1 (about the axis p32)
	p21q = (v12 * (q - p2) > 0.0);

	// p0 에서 최단 거리 발생
	if (p01q && p02q)
	{
		r = p0;
		return dist_sq(q, p0);
	}
	else if (p10q && p12q)		// p1 에서 최단 거리 발생
	{
		r = p1;
		return dist_sq(q, p1);
	}
	else if (p20q && p21q)		// p2 에서 최단 거리 발생
	{
		r = p2;
		return dist_sq(q, p2);
	}

	// Case 2: 에지에서 최단 거리가 발생하는 경우
	DgVec3 tmp = v20 ^ v01;
	DgVec3 n = tmp.normalize();
	//DgVec3 n = (v20 ^ v01).normalize();
	DgVec3 p0q = v01 ^ n; // outward direction of edge(p0, p1)
	DgVec3 p1q = v12 ^ n; // outward direction of edge(p1, p2)
	DgVec3 p2q = v20 ^ n; // outward direction of edge(p2, p0)

	// 에지 (p0, p1)에서 최단 거리가 발생하는 경우
	if (!p01q && !p10q && (p0q * (q - p0) > 0.0))
	{
		p0q.normalize();
		v01.normalize();
		r = p0 + ((q - p0) * v01) * v01;
		return ((p0q * (q - p0)) * (p0q * (q - p0)) + (n * (q - p0)) * (n * (q - p0)));
	}
	// 에지 (p1, p2)에서 최단 거리가 발생하는 경우
	else if (!p12q && !p21q && (p1q * (q - p1) > 0.0))
	{
		p1q.normalize();
		v12.normalize();
		r = p1 + ((q - p1) * v12) * v12;
		return ((p1q * (q - p1)) * (p1q * (q - p1)) + (n * (q - p1)) * (n * (q - p1)));
	}
	// 에지 (p2, p0)에서 최단 거리가 발생하는 경우
	else if (!p20q && !p02q && (p2q * (q - p2) > 0.0))
	{
		p2q.normalize();
		v20.normalize();
		r = p2 + ((q - p2) * v20) * v20;
		return ((p2q * (q - p2)) * (p2q * (q - p2)) + (n * (q - p2)) * (n * (q - p2)));
	}

	// Case 3: 삼각형 내부에서 최단 거리가 발생하는 경우
	double d = (n * (q - p0)) * (n * (q - p0));
	if (n * (q - p0) < 0.0)
		r = q + SQRT(d) * n;
	else
		r = q - SQRT(d) * n;
	return d;
}
