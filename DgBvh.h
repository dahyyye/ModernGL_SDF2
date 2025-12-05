#pragma once
#include "DgViewer.h"

/*!
 *	\class	DgBvh
 *	\brief	AABB (Axis-Aligned Bounding Box) 타입의 BVH (Bounding Volume Hierarchy)를 표현하는 클래스
 *	\note	마지막 수정일: 2021-02-23
 *
 *	\author 한상준, 윤승현
 */
class DgBvh
{
public:
	/*!
	 *	\class	AaBb (Axis-Alined Bounding Box)
	 *	\brief	AABB 타입 경계 상자를 표현하는 클래스
	 *	\note	마지막 수정일: 2021-04-07
	 *
	 *	\author 한상준, 윤승현
	 */
	class AaBb
	{
	public:
		/* \brief 대상 메쉬에 대한 포인터 */
		DgMesh* mMesh;

		/* \brief 경계 상자에 포함된 삼각형 리스트 */
		std::vector<DgFace*> mFaces;

		/* \brief 경계 상자의 최소 위치 */
		DgPos mMin;

		/* \brief 경계 상자의 최대 위치 */
		DgPos mMax;

		/* \brief 경계 상자의 BVH 레벨 */
		int mDepth;

		/* \brief 리프 경계 상자 여부 */
		bool mIsLeaf;

		/* \brief 하위 경계 상자에 대한 포인터 */
		std::vector<AaBb*> mChildNodes;

	public:
		/*!
		 *	\brief	디폴트 생성자
		 */
		AaBb();

		/*!
		 *	\brief	삼각형 리스트를 바운딩하는 경계 상자와 하위 경계 상자(필요한 경우)를 생성한다.
		 *	\param[in]	pMesh		메쉬 포인터
		 *	\param[in]	faces		경계 상자에 포함될 삼각형 리스트
		 *	\param[in]	depth		경계 상자의 BVH 레벨
		 */
		AaBb(DgMesh* pMesh, std::vector<DgFace*>& faces, const int depth);

		/*!
		 *	\brief	소멸자
		 */
		~AaBb();

		/*!
		 *	\brief	경계 상자를 두 축(최단축 제외)으로 분할하여 4개의 경계 상자를 생성한다.
		 */
		void divide();

		/*!
		 *	\brief	점과 경계 상자 사이의 최단 거리 제곱을 계산한다.
		 *	\note	점이 경계 상자 내부에 포함되었다면 0을 반환한다.
		 *
		 *	\param[in]	p	점의 위치
		 *
		 *	\return 점과 경계 상자 사이의 최단 거리 제곱을 반환한다.
		 */
		double getSqrDist(const DgPos& p);

		/*!
		 *	\brief	삼각형과 경계 상자의 교차 여부를 검사한다.
		 *
		 *	\param[in]	p0		조사할 삼각형의 첫 번째 정점
		 *	\param[in]	p1		조사할 삼각형의 두 번째 정점
		 *	\param[in]	p2		조사할 삼각형의 세 번째 정점
		 *
		 *	\return 삼각형과 경계 상자가 교차하면 true, 아니면 false를 반환한다.
		 */
		bool intersectWithTri(const DgPos& p0, const DgPos& p1, const DgPos& p2);
	};

public:
	/* \brief 대상 메쉬에 대한 포인터 */
	DgMesh* mMesh;

	/* \brief 최상위 경계 상자에 대한 포인터 */
	AaBb*	mRoot;

	/* \brief 대상 메쉬를 복제 여부 */
	bool	mCopy;

public:
	/*!
	 *	\brief	생성자
	 */
	DgBvh();

	/*!
	 *	\brief	대상 메쉬에 대한 BVH를 생성한다.
	 *
	 *	\param[in]	pMesh	대상 메쉬
	 */
	DgBvh(DgMesh* pMesh);

	/*!
	 *	\brief	부분 메쉬에 대한 BVH를 생성한다.
	 *
	 *	\param[in]	subFaces	부분 메쉬의 삼각형 배열
	 */
	DgBvh(std::vector<DgFace*>& subFaces);

	/*!
	 *	\brief	입력 정보로 BVH를 생성한다.
	 *
	 *	\param[in]	verts	정점의 리스트
	 *	\param[in]	faces	삼각형을 구성하는 정점의 인덱스 리스트
	 */
	DgBvh(std::vector<DgPos>& verts, std::vector<int>& faces);

	/*!
	 *	\brief	소멸자
	 */
	~DgBvh();

	/*!
	 *	\brief	점과 메쉬 사이의 거리가 주어진 거리보다 가까운지 조사한다.
	 *
	 *	\param[in]	dist		입력 거리
	 *	\param[in]	p			조사할 점의 위치
	 *	\param[in]	bVertOnly	점과 메쉬 정점 사이의 거리만을 조사하면 true, 점과 메쉬 삼각형 사이의 거리를 조사하면 false
	 *
	 *	\return 정점과 메쉬 사이의 거리가 주어진 거리보다 가까우면 true, 아니면 false를 반환한다.
	 */
	bool isCloserThan(double dist, const DgPos& p, bool bVertOnly);

	/*!
	 *	\brief	점과 메쉬의 최단 거리를 계산한다.
	 *
	 *	\param[in]	p		점의 위치
	 *	\param[out]	q		최단 거리가 발생하는 메쉬 위의 점이 저장된다.
	 *	\param[out]	pFace	최단 거리가 발생하는 삼각형의 포인터가 저장된다.
	 *	\param[in]	bSinged	음수 거리 변환 여부
	 *
	 *	\return 계산된 최단 거리를 반환한다.
	 */
	double computeDistance(DgPos p, DgPos& q, DgFace** pFace, bool bSigned);

	/*!
	 *	\brief	점과 메쉬의 최단 거리를 계산한다.
	 *
	 *	\param[in]	p		점의 위치
	 *	\param[out]	q		최단 거리가 발생하는 메쉬 위의 점이 저장된다.
	 *	\param[out]	fidx	최단 거리가 발생하는 삼각형의 인덱스가 저장된다.
	 *	\param[in]	sign	음수 거리 변환 여부
	 *
	 *	\return 계산된 최단 거리를 반환한다.
	 */
	double computeDistance(DgPos p, DgPos& q, int& fidx, bool sign);

	/*!
	 *	\brief	입력 삼각형과 교차하는 BVH의 모든 삼각형을 찾는다.
	 *
	 *	\param[in]	fB				입력 삼각형
	 *	\param[out]	crossFacesA		입력 삼각형과 교차하는 BVH의 모든 삼각형이 저장된다.
	 *	\param[out]	cutSegmentsB	fB에 놓인 교차 선분들이 저장된다.
	 *
	 *	\return 수치 안정성을 위해 fB의 교란이 필요한 경우 -1, 아니면 fB와 교차하는 삼각형의 수를 반환한다.
	 */
	//int intersectWithTri(DgFace* fB, std::vector<DgFace*>& crossFacesA, std::vector<Segment>& cutSegmentsB);
	//int intersectWithTri2(DgFace* fB, std::unordered_map<DgFace*, std::vector<Segment>>& faceToSegmentsA, std::unordered_map <DgFace*, std::vector<Segment>>& faceToSegmentsB);

	/*!
	 *	\brief	입력 삼각형과 교차하는 모든 삼각형을 찾는다.
	 *
	 *	\param[in]	u0			입력 삼각형의 첫 번째 정점의 좌표
	 *	\param[in]	u1			입력 삼각형의 두 번째 정점의 좌표
	 *	\param[in]	u2			입력 삼각형의 세 번째 정점의 좌표
	 *	\param[out]	hitFaces	입력 삼각형과 교차하는 삼각형이 저장된다.
	 *
	 *	\return 입력 삼각형과 교차하는 삼각형의 수를 반환한다.
	 */
	int intersectWithTri(const DgPos& u0, const DgPos& u1, const DgPos& u2, std::vector<DgFace*>& hitFaces);
};

/*!
 *	\brief	점과 삼각형 정점 사이의 최단 거리 제곱을 계산한다.
 *
 *	\param[in]	q	점의 위치
 *	\param[in]	f	대상 삼각형의 포인터
 *
 *	\return 계산된 최단 거리 제곱을 반환한다.
 */
double dist_sq_vert(const DgPos& p, DgFace* f);

/*!
 *	\brief	점과 삼각형 사이의 최단 거리 제곱을 계산한다.
 *
 *	\param[in]	q	점의 위치
 *	\param[in]	f	대상 삼각형의 포인터
 *
 *	\return 계산된 최단 거리 제곱을 반환한다.
 */
double dist_sq(const DgPos& q, DgFace* f);
