#pragma once

/*!
 *	\class	DgPos
 *	\brief	3차원 위치를 나타내는 클래스
 */
class DgPos
{
public:
	/*! \brief 3차원 위치 좌표 */
	double mPos[3];

public:
	DgPos() {
		mPos[0] = 0.0;
		mPos[1] = 0.0;
		mPos[2] = 0.0;
	}
	DgPos(double x, double y, double z) {
		mPos[0] = x;
		mPos[1] = y;
		mPos[2] = z;
	}
	~DgPos() {}

	/*!
	*	\brief	인덱스 연산자([])
	*
	*	\param[in]	idx 인덱스
	*
	*	\return 포인트의 idx 번째 원소의 레퍼런스를 반환한다.
	*/
	double& operator [](const int& idx);

	/*!
	*	\brief	상수 객체의 인덱스 연산자([])
	*
	*	\param[in]	idx 인덱스
	*
	*	\return 포인트의 idx 번째 원소의 레퍼런스를 반환한다.
	*/
	const double& operator [](const int& idx) const;

};

/*!
 *	\class	DgVertex
 *	\brief	정점을 표현하는 클래스
 */
class DgVertex
{
public:
	/*! \brief 정점의 좌표 */
	double mPos[3];	

public:
	DgVertex(double x, double y, double z) {
		mPos[0] = x;
		mPos[1] = y;
		mPos[2] = z;
	}
	~DgVertex() {}
};

/*!
 *	\class	DgTexel
 *	\brief	텍스처 좌표를 표현하는 클래스
 */
class DgTexel
{
public:
	/*! \brief 텍스처 좌표(s, t) */
	double mST[2];	

public:
	DgTexel(double s, double t) {
		mST[0] = s;
		mST[1] = t;
	}
	~DgTexel() {}
};

/*!
 *	\class	DgNormal
 *	\brief	법선을 표현하는 클래스
 */
class DgNormal
{
public:
	/*! \brief 법선의 방향 */
	double mDir[3];

public:
	DgNormal(double x, double y, double z) {
		mDir[0] = x;
		mDir[1] = y;
		mDir[2] = z;
	}
	~DgNormal() {}
};

/*!
 *	\class	DgFace
 *	\brief	삼각형을 표현하는 클래스
 */
class DgFace
{
public:
	int mMtlIdx;		/*! \brief 삼각형이 사용하는 재질의 인덱스 */
	int mVertIdxs[3];	/*! \brief 삼각형을 구성하는 세 정점의 인덱스 */
	int mTexelIdxs[3];	/*! \brief 삼각형 세 정점에 할당된 텍셀의 인덱스 */
	int mNormalIdxs[3];	/*! \brief 삼각형 세 정점에 할당된 법선의 인덱스 */

public:
	DgFace(int vidx0, int vidx1, int vidx2, int nidx0, int nidx1, int nidx2, int mtlIdx) {
		mVertIdxs[0] = vidx0;	mVertIdxs[1] = vidx1;	mVertIdxs[2] = vidx2;
		mTexelIdxs[0] = -1;		mTexelIdxs[1] = -1;		mTexelIdxs[2] = -1;
		mNormalIdxs[0] = nidx0;	mNormalIdxs[1] = nidx1;	mNormalIdxs[2] = nidx2;	
		mMtlIdx = mtlIdx;
	}
	DgFace(int vidx0, int vidx1, int vidx2, int tidx0, int tidx1, int tidx2, int nidx0, int nidx1, int nidx2, int mtlIdx) {
		mVertIdxs[0] = vidx0;	mVertIdxs[1] = vidx1;	mVertIdxs[2] = vidx2;
		mTexelIdxs[0] = tidx0;	mTexelIdxs[1] = tidx1;	mTexelIdxs[2] = tidx2;
		mNormalIdxs[0] = nidx0;	mNormalIdxs[1] = nidx1;	mNormalIdxs[2] = nidx2;	
		mMtlIdx = mtlIdx;
	}
	~DgFace() {}

	DgPos getVertexPos(int vidx);
};

/*!
 *	\class	DgMaterial
 *	\brief	메쉬 재질을 표현하는 클래스
 */
class DgMaterial
{
public:
	std::string mName;	/*! \brief 재질의 이름 */
	float mKa[3];		/*! \brief 주변광 반사 계수 */
	float mKd[3];		/*! \brief 난반사광 반사 계수 */
	float mKs[3];		/*! \brief 전반사광 반사 계수 */
	float mNs;			/*! \brief 전반사 지수 */
	GLuint mTexId;		/*! \brief 재질의 텍스처 아이디(1부터 시작) */

public:
	DgMaterial() {
		mKa[0] = 0.1f;	mKa[1] = 0.1f;	mKa[2] = 0.1f;
		mKd[0] = 0.7f;	mKd[1] = 0.7f;	mKd[2] = 0.7f;
		mKs[0] = 0.9f;	mKs[1] = 0.9f;	mKs[2] = 0.9f;
		mNs = 32.0f;
		mTexId = 0;
	}
	~DgMaterial() {}
};

/*!
 *	\class	DgMesh
 *	\brief	삼각형으로 구성된 메쉬 모델을 표현하는 클래스
 */
class DgMesh
{
public:
	std::string mName;					/*! \brief 메쉬 이름 */
	std::vector<DgVertex> mVerts;		/*! \brief 메쉬를 구성하는 정점 배열 */
	std::vector<DgTexel> mTexels;		/*! \brief 메쉬를 구성하는 텍셀 배열 */
	std::vector<DgNormal> mNormals;		/*! \brief 메쉬를 구성하는 법선 배열 */
	std::vector<DgFace> mFaces;			/*! \brief 메쉬를 구성하는 삼각형 배열 */
	std::vector<DgMaterial> mMaterials;	/*! \brief 메쉬가 사용하는 재질 배열 */

	std::vector<std::vector<unsigned int>> mVertexIndicesPerMtl;	/*! 재질별 삼각형 정점 인덱스 그룹 */
	GLuint mShaderId;												/*! \brief 메쉬가 사용하는 쉐이더 아이디 */
	GLuint mVAO;
	GLuint mVBO;
	GLuint mEBO;	
	bool mBuffersInitialized;

public:
	DgMesh() {
		mVAO = 0;
		mVBO = 0;
		mEBO = 0;
		mBuffersInitialized = false;
	}

	~DgMesh() {
		if (mVAO) glDeleteVertexArrays(1, &mVAO);
		if (mVBO) glDeleteBuffers(1, &mVBO);
		if (mEBO) glDeleteBuffers(1, &mEBO);
	};

	void setupBuffers();
	void computeNormal(int normalType);
	void render();
};

/*!
 *	\brief	OBJ 파일을 임포트하여 메쉬 모델을 구성한다.
 * 
 *	\param[in]	fname	파일 이름
 */
DgMesh* import_mesh_obj(const char* fname);

/*!
 *	\brief	쉐이더 프로그램을 생성한다.
 *
 *	\param[in]	vertexPath		정점 쉐이더 파일의 경로
 *	\param[in]	fragmentPath	프래그먼트 쉐이더 파일의 경로
 * 
 *	\return	생성된 프로그램의 핸들을 반환한다.
 */
GLuint load_shaders(const char* vertexPath, const char* fragmentPath);

// 바운딩 박스 메쉬 생성 함수 선언
DgMesh* createBoundingBoxMesh(const DgPos& minPos, const DgPos& maxPos);
