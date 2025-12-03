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

	/*! \brief 정점이 포함된 메쉬 포인터 */
	DgMesh* mMesh;

	/*! \brief 정점의 인덱스 */
	int mIdx;

	/*! \brief 정점의 3차원 좌표 */
	DgPos mPos;

	/*! \brief 정점에서 시작하는 하프에지 배열 */
	std::vector<DgEdge *> mEdges;
public:
	DgVertex(double x, double y, double z) {
		mPos[0] = x;
		mPos[1] = y;
		mPos[2] = z;
	}
	~DgVertex() {}
};

/*!
 *	\class	DgEdge
 *	\brief	삼각형의 하프에지(Halfedge)를 표현하는 클래스
 *
 *	\author 박정호, 윤승현
 *	\date	25 Jan 2018
 */
class DgEdge
{
public:
	/*! \brief 에지 시작점에 연결된 정점 */
	DgVertex* mVert;

	/*! \brief 에지 시작점에 연결된 텍셀 */
	DgTexel* mTexel;

	/*! \brief 에지 시작점에 연결된 법선 */
	DgNormal* mNormal;

	/*! \brief 에지가 포함된 삼각형에 대한 포인터 */
	DgFace* mFace;

	/*! \brief 삼각형을 구성하는 다음 에지에 대한 포인터 */
	DgEdge* mNext;

	/*! \brief 인접한 삼각형의 반대편 에지에 대한 포인터 */
	DgEdge* mMate;

	/*! \brief 에지와 연관된 스칼라(비용, 길이, 가중치, 길이, 특징 에지 여부, 사용 여부) 등의 정보: 초기값(0.0), 특징 에지(-1.0), 미사용 에지(-1.0) */
	double mCostOrLen;

public:
	/*!
	 *	\brief	생성자
	 *
	 *	\param[in]	pVert	에지의 시작점에 연결할 정점에 대한 포인터
	 *	\param[in]	pTexel	에지의 시작점에 연결할 텍셀에 대한 포인터
	 *	\param[in]	pNormal 에지의 시작점에 연결할 법선에 대한 포인터
	 */
	DgEdge(DgVertex* pVert, DgTexel* pTexel, DgNormal* pNormal);

	/*!
	 *	\brief	소멸자
	 */
	virtual ~DgEdge();

	/*!
	 *	\brief	다음 에지를 반환한다.
	 *
	 *	\return 다음 에지를 반환한다.
	 */
	DgEdge* next() { return mNext; }

	/*!
	 *	\brief	이전 에지를 반환한다.
	 *
	 *	\return 이전 에지를 반환한다.
	 */
	DgEdge* prev() { return mNext->mNext; }

	/*!
	 *	\brief	에지의 시작 정점을 반환한다.
	 *
	 *	\return 에지의 시작 정점을 반환한다.
	 */
	DgVertex* sv() { return mVert; }

	/*!
	 *	\brief	에지의 끝 정점을 반환한다.
	 *
	 *	\return 에지의 끝 정점을 반환한다.
	 */
	DgVertex* ev() { return mNext->mVert; }

	/*!
	 *	\brief	에지를 공유한 삼각형을 구한다.
	 *
	 *	\return 에지를 공유한 삼각형의 리스트를 반환한다.
	 */
	std::vector<DgFace*> getFaces();

	/*!
	 *	\brief	경계 에지 여부를 조사한다.
	 *
	 *	\return 경계 에지이면 true, 아니면 false를 반환한다.
	 */
	bool isBndry();
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

	/*! \brief 삼각형의 인덱스 */
	int mIdx;

	/*! \brief 삼각형의 시작 에지에 대한 포인터 */
	DgEdge* mEdge;

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

	/*!
	 *	\brief	삼각형에서 정점의 좌표를 반환한다.
	 *
	 *	\param[in]	vidx	삼각형을 구성하는 정점 인덱스(0, 1, 2)
	 *
	 *	\return 인덱스에 대응하는 정점의 좌표를 반환한다.
	 */
	DgPos getVertexPos(int vidx);

	/*!
	 *	\brief	삼각형 정점의 포인터를 반환한다.
	 *
	 *	\param[in]	vidx	삼각형을 구성하는 정점의 인덱스(0, 1, 2)
	 *
	 *	\return 인덱스에 대응되는 정점의 포인터를 반환한다.
	 */
	DgVertex* getVertex(int vIdx);

	/*!
	 *	\brief	삼각형의 에지 포인터를 반환한다.
	 *
	 *	\param[in]	eidx	삼각형을 구성하는 에지의 인덱스(0, 1, 2)
	 *
	 *	\return 인덱스에 대응되는 에지의 포인터를 반환한다.
	 */
	DgEdge* getEdge(int eidx);

	/*!
	 *	\brief	삼각형의 면적을 계산한다.
	 *
	 *	\return 계산된 면적을 반환한다.
	 */
	double getArea();

	/*!
	*	\brief	삼각형이 포함된 메쉬의 포인터를 반환한다.
	*
	*	\return 삼각형이 포함된 메쉬의 포인터를 반환한다.
	*/
	virtual	DgMesh* getMesh();
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

/*!
 * \brief 점 \a p와 점 \a q를 \a t : (1 - t)로 내분한다.
 *
 * \param[in] p 3차원 공간의 점
 * \param[in] q 3차원 공간의 점
 * \param[in] t 내분 비율
 *
 * \return 점 \a p와 \a q의 내분점을 반환한다.
 */
DgPos lerp(const DgPos& p, const DgPos& q, double t);


/*********************/
/*   DgVec3 클래스   */
/*********************/

/*!
 * \class   DgVec3
 * \brief   3차원 벡터를 표현하는 클래스
 *
 * \author  윤승현(shyun@dongguk.edu)
 * \date    01 Jan 2001
 */
class DgVec3
{
public:
	/*! \brief 3차원 벡터의 좌표 배열 */
	double mCoords[3];

public:
	/*!
	 * \brief  생성자
	 *
	 * \param[in] x  벡터의 x 좌표
	 * \param[in] y  벡터의 y 좌표
	 * \param[in] z  벡터의 z 좌표
	 */
	DgVec3(double x = 0.0, double y = 0.0, double z = 0.0);

	/*!
	 * \brief  이니셜라이저 리스트를 사용한 생성자
	 *
	 * \param[in] coords	중괄호로 표현된 벡터의 좌표 {x, y, z}
	 */
	DgVec3(std::initializer_list<double> coords);

	/*!
	 * \brief  복사 생성자
	 *
	 * \param[in] cpy  복사될 객체
	 */
	DgVec3(const DgVec3& cpy);

	/*!
	 * \brief  소멸자
	 */
	~DgVec3();

	/*!
	 * \brief  벡터의 좌표를 설정한다.
	 *
	 * \param[in] x  설정할 x 좌표
	 * \param[in] y  설정할 y 좌표
	 * \param[in] z  설정할 z 좌표
	 *
	 * \return 설정된 자신을 반환한다.
	 */
	DgVec3& setCoords(double x, double y, double z);

	/*!
	 * \brief  영벡터 여부를 조사한다.
	 *
	 * \param[in] eps  허용 오차
	 *
	 * \return 영벡터이면 true, 아니면 false를 반환한다.
	 */
	bool isZero(double eps = 1e-9) const;

	/*!
	 * \brief  벡터를 단위길이로 정규화한다.
	 *
	 * \param[in] eps  영벡터 판단을 위한 허용 오차
	 *
	 * \return 정규화된 자신을 반환한다.
	 */
	DgVec3& normalize(double eps = 1e-9);

	/*!
	 * \brief  대입 연산자
	 *
	 * \param[in] rhs  대입될 객체
	 *
	 * \return 대입된 자신을 반환한다.
	 */
	DgVec3& operator =(const DgVec3& rhs);

	/*!
	 * \brief  벡터를 더한다.
	 *
	 * \param[in] rhs  더할 벡터
	 *
	 * \return 변경된 자신을 반환한다.
	 */
	DgVec3& operator +=(const DgVec3& rhs);

	/*!
	 * \brief  벡터를 뺀다.
	 *
	 * \param[in] rhs  뺄 벡터
	 *
	 * \return 변경된 자신을 반환한다.
	 */
	DgVec3& operator -=(const DgVec3& rhs);

	/*!
	 * \brief  벡터를 상수배 한다.
	 *
	 * \param[in] s  상수
	 *
	 * \return 변경된 자신을 반환한다.
	 */
	DgVec3& operator *=(const double& s);

	/*!
	 * \brief  벡터를 상수로 나눈다.
	 *
	 * \param[in] s  나눌 상수
	 *
	 * \return 변경된 자신을 반환한다.
	 */
	DgVec3& operator /=(const double& s);

	/*!
	 * \brief  벡터를 외적한다.
	 *
	 * \param[in] rhs  외적할 벡터
	 *
	 * \return 변경된 자신을 반환한다.
	 */
	DgVec3& operator ^=(const DgVec3& rhs);

	/*!
	 * \brief  단항 연산자(+)
	 *
	 * \return 동일부호를 갖는 객체를 반환한다.
	 */
	DgVec3 operator +() const;

	/*!
	 * \brief  단항 연산자(-)
	 *
	 * \return 반대부호를 갖는 객체를 반환한다.
	 */
	DgVec3 operator -() const;

	/*!
	 * \brief  인덱스 연산자([])
	 *
	 * \param[in] idx  참조 인덱스
	 *
	 * \return 벡터의 idx번째 원소의 레퍼런스를 반환한다.
	 */
	double& operator [](const int& idx);

	/*!
	 * \brief  상수객체에 대한 인덱스 연산자([])
	 *
	 * \param[in] idx  인덱스
	 *
	 * \return 벡터의 idx번째 원소의 레퍼런스를 반환한다.
	 */
	const double& operator [](const int& idx) const;

	/*!
	 * \brief  두 벡터를 더한다.
	 *
	 * \param[in] v  첫 번째 벡터
	 * \param[in] w  두 번째 벡터
	 *
	 * \return 두 벡터의 합을 반환한다.
	 */
	friend DgVec3 operator +(const DgVec3& v, const DgVec3& w);

	/*!
	 * \brief  두 벡터를 뺀다.
	 *
	 * \param[in] v  첫 번째 벡터
	 * \param[in] w  두 번째 벡터
	 *
	 * \return 두 벡터의 차를 반환한다.
	 */
	friend DgVec3 operator -(const DgVec3& v, const DgVec3& w);

	/*!
	 * \brief  벡터를 상수배 한다.
	 *
	 * \param[in] v  벡터
	 * \param[in] s  상수
	 *
	 * \return 벡터의 상수배를 반환한다.
	 */
	friend DgVec3 operator *(const DgVec3& v, const double& s);

	/*!
	 * \brief  벡터를 상수배 한다.
	 *
	 * \param[in] s  상수
	 * \param[in] v  벡터
	 *
	 * \return 벡터의 상수배를 반환한다.
	 */
	friend DgVec3 operator *(const double& s, const DgVec3& v);

	/*!
	 * \brief  두 벡터의 내적을 구한다.
	 *
	 * \param[in] v  첫 번째 벡터
	 * \param[in] w  두 번째 벡터
	 *
	 * \return 두 벡터의 내적 결과를 반환한다.
	 */
	friend double operator *(const DgVec3& v, const DgVec3& w);

	/*!
	 * \brief  역수 벡터를 구한다.
	 *
	 * \param[in] s  상수
	 * \param[in] v  벡터
	 *
	 * \return 역수 벡터 (s/x, s/y, s/z)를 반환한다.
	 */
	friend DgVec3 operator /(const double& s, const DgVec3& v);

	/*!
	 * \brief  벡터를 상수로 나눈다.
	 *
	 * \param[in] v  벡터
	 * \param[in] s  상수
	 *
	 * \return 상수로 나누어진 벡터를 반환한다.
	 */
	friend DgVec3 operator /(const DgVec3& v, const double& s);

	/*!
	 * \brief  두 벡터의 외적 벡터를 구한다.
	 *
	 * \param[in] v  첫 번째 벡터
	 * \param[in] w  두 번째 벡터
	 *
	 * \return 외적 벡터 (v X w)를 반환한다.
	 */
	friend DgVec3 operator ^(const DgVec3& v, const DgVec3& w);

	/*!
	 * \brief  두 벡터가 같은지 조사한다.
	 *
	 * \param[in] v  첫 번째 벡터
	 * \param[in] w  두 번째 벡터
	 *
	 * \return 두 벡터가 같으면 true, 다르면 false를 반환한다.
	 */
	friend bool operator ==(const DgVec3& v, const DgVec3& w);

	/*!
	 * \brief  두 벡터가 다른지 조사한다.
	 *
	 * \param[in] v  첫 번째 벡터
	 * \param[in] w  두 번째 벡터
	 *
	 * \return 두 벡터가 다르면 true, 같으면 false를 반환한다.
	 */
	friend bool operator !=(const DgVec3& v, const DgVec3& w);

	/*!
	 * \brief  출력 연산자(<<)
	 *
	 * \param[out] os  출력 스트림
	 * \param[in]  v   출력할 벡터
	 *
	 * \return 벡터가 출력된 스트림을 반환한다.
	 */
	friend std::ostream& operator <<(std::ostream& os, const DgVec3& v);

	/*!
	 * \brief  입력 연산자(>>)
	 *
	 * \param[in]  is  입력스트림
	 * \param[out] v   입력값이 저장될 벡터
	 *
	 * \return 입력값이 제거된 입력스트림을 반환한다.
	 */
	friend std::istream& operator >>(std::istream& is, DgVec3& v);
};

/***********************/
/*   DgVec3 유틸 함수  */
/***********************/

/*!
 * \brief   벡터 u를 벡터 v에 직교 투영한 벡터를 구한다.
 *
 * \param[in] u  대상 벡터
 * \param[in] v  참조 벡터
 *
 * \return  벡터 u를 벡터 v에 사영시킨 벡터를 반환한다.
 */
DgVec3 proj(const DgVec3& u, const DgVec3& v);

/*!
 * \brief   벡터 v에 수직한 단위 벡터를 구한다.
 *
 * \param[in] v  대상 벡터
 *
 * \return  벡터 v에 수직한 단위 벡터를 구하여 반환한다.
 */
DgVec3 ortho(const DgVec3& v);

/*!
 * \brief   벡터 u, v, w의 행렬식을 계산한다.
 *
 * \param[in] u  벡터
 * \param[in] v  벡터
 * \param[in] w  벡터
 *
 * \return  벡터 u, v, w의 행렬식의 값을 반환한다.
 */
double det(const DgVec3& u, const DgVec3& v, const DgVec3& w);

/*!
 * \brief   벡터 v의 크기를 구한다.
 *
 * \param[in] v  대상 벡터
 *
 * \return  벡터 \a v의 크기를 반환한다.
 */
double norm(const DgVec3& v);

/*!
 * \brief   벡터 v의 크기의 제곱을 계산한다.
 *
 * \param[in] v  대상 벡터
 *
 * \return  벡터 \a v의 크기의 제곱을 반환한다.
 */
double norm_sq(const DgVec3& v);

/*!
 * \brief   두 벡터 사이의 사이각(0 ~ 180)을 구한다.
 *
 * \param[in] u       시작 벡터
 * \param[in] v       끝 벡터
 * \param[in] radian  각도의 형태(true: radian, false: degree)
 *
 * \return  벡터 \a u에서 벡터 \a v까지의 사이각(예각, 방향 상관없음)
 */
double angle(const DgVec3& u, const DgVec3& v, bool radian = false);

/*!
 * \brief   두 벡터 사이의 사이각(0 ~ 360)을 구한다.
 *
 * \param[in] u       시작 벡터
 * \param[in] v       끝 벡터
 * \param[in] axis    회전축 벡터
 * \param[in] radian  각도의 형태(true: radian, false: degree)
 *
 * \return  벡터 \a u에서 벡터 \a v까지의 사이각(axis 벡터 기준 반시계 방향)
 */
double angle(const DgVec3& u, const DgVec3& v, const DgVec3& axis, bool radian = false);