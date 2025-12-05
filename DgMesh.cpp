#include "DgViewer.h"

#define STB_IMAGE_IMPLEMENTATION
#include ".\\include\\STB\\stb_image.h"
void DgMesh::setupBuffers()
{
	if (mFaces.empty()) return;

	const bool hasTexCoord = !mTexels.empty();

	// 1) 재질 보정: 없으면 기본재질 하나 추가, 잘못된 인덱스가 있으면 fallback 준비
	bool hadMatsInitially = !mMaterials.empty();
	size_t defaultMatIdx = 0;

	if (!hadMatsInitially) {
		mMaterials.emplace_back();           // 기본 재질 생성
		// (옵션) mMaterials.back().mName = "Default";
		defaultMatIdx = 0;
	}

	bool needFallback = false;
	for (const auto& f : mFaces) {
		if (f.mMtlIdx < 0 || f.mMtlIdx >= (int)mMaterials.size()) {
			needFallback = true; break;
		}
	}
	if (hadMatsInitially && needFallback) {
		defaultMatIdx = mMaterials.size();
		mMaterials.emplace_back();           // 초기 재질이 있었지만 일부 face가 -1/범위밖 → fallback 추가
		// (옵션) mMaterials.back().mName = "Default";
	}

	// 2) 재질별 버킷 준비
	mVertexIndicesPerMtl.clear();
	mVertexIndicesPerMtl.resize(mMaterials.size());

	std::vector<float> vertexData;
	unsigned int nextIndex = 0;

	for (size_t i = 0; i < mFaces.size(); ++i)
	{
		const DgFace& face = mFaces[i];

		// 유효 재질 인덱스 선택(없거나 범위 밖이면 defaultMatIdx)
		size_t bucket = (face.mMtlIdx >= 0 && face.mMtlIdx < (int)mMaterials.size())
			? (size_t)face.mMtlIdx : defaultMatIdx;

		for (int j = 0; j < 3; ++j)
		{
			const int vIdx = face.mVertIdxs[j];
			const int nIdx = face.mNormalIdxs[j];

			const DgVertex& v = mVerts[vIdx];
			const DgNormal& n = mNormals[nIdx];

			// position
			vertexData.push_back((float)v.mPos[0]);
			vertexData.push_back((float)v.mPos[1]);
			vertexData.push_back((float)v.mPos[2]);

			// normal
			vertexData.push_back((float)n.mDir[0]);
			vertexData.push_back((float)n.mDir[1]);
			vertexData.push_back((float)n.mDir[2]);

			// texcoord (옵션)
			if (hasTexCoord) {
				const int tIdx = face.mTexelIdxs[j];
				const DgTexel& t = mTexels[tIdx];
				vertexData.push_back((float)t.mST[0]);
				vertexData.push_back((float)t.mST[1]);
			}

			// 재질 버킷에 인덱스 push
			mVertexIndicesPerMtl[bucket].push_back(nextIndex++);
		}
	}

	// 3) VAO/VBO 업로드
	if (!mBuffersInitialized) {
		glGenVertexArrays(1, &mVAO);
		glGenBuffers(1, &mVBO);
		mBuffersInitialized = true;
	}

	glBindVertexArray(mVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

	const int stride = hasTexCoord ? 8 : 6;

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	if (hasTexCoord) {
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);
	}

	glBindVertexArray(0);
}


//void DgMesh::setupBuffers()
//{
//	if (mFaces.empty()) return;
//
//	bool hasTexCoord = !mTexels.empty();  // 텍스처 좌표 유무
//
//	std::vector<float> vertexData;
//	mVertexIndicesPerMtl.clear();
//	mVertexIndicesPerMtl.resize(mMaterials.size());
//
//	unsigned int nextIndex = 0;
//
//	for (size_t i = 0; i < mFaces.size(); ++i)
//	{
//		const DgFace& face = mFaces[i];
//
//		for (int j = 0; j < 3; ++j)
//		{
//			int vIdx = face.mVertIdxs[j];
//			int nIdx = face.mNormalIdxs[j];
//
//			const DgVertex& v = mVerts[vIdx];
//			const DgNormal& n = mNormals[nIdx];
//
//			// [position: 3 floats]
//			vertexData.push_back((float)v.mPos[0]);
//			vertexData.push_back((float)v.mPos[1]);
//			vertexData.push_back((float)v.mPos[2]);
//
//			// [normal: 3 floats]
//			vertexData.push_back((float)n.mDir[0]);
//			vertexData.push_back((float)n.mDir[1]);
//			vertexData.push_back((float)n.mDir[2]);
//
//			// [texcoord: 2 floats] - only if available
//			if (hasTexCoord) {
//				int tIdx = face.mTexelIdxs[j];
//				const DgTexel& t = mTexels[tIdx];
//				vertexData.push_back((float)t.mST[0]);
//				vertexData.push_back((float)t.mST[1]);
//			}
//
//			// 재질별 인덱스 추가
//			mVertexIndicesPerMtl[face.mMtlIdx].push_back(nextIndex++);
//		}
//	}
//
//	if (!mBuffersInitialized) {
//		glGenVertexArrays(1, &mVAO);
//		glGenBuffers(1, &mVBO);
//		mBuffersInitialized = true;
//	}
//
//	glBindVertexArray(mVAO);
//
//	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
//	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
//
//	// 공통 레이아웃: position (0), normal (1)
//	int stride = hasTexCoord ? 8 : 6;
//
//	// layout(location = 0): position
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
//	glEnableVertexAttribArray(0);
//
//	// layout(location = 1): normal
//	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float)));
//	glEnableVertexAttribArray(1);
//
//	if (hasTexCoord)
//	{
//		// layout(location = 2): texcoord
//		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(6 * sizeof(float)));
//		glEnableVertexAttribArray(2);
//	}
//
//	glBindVertexArray(0);
//}

void DgMesh::render()
{
	if (!mBuffersInitialized)
		setupBuffers();

	// 텍스처가 있으면 텍스처 전용 셰이더 사용
	bool hasTexture = false;
	for (const auto& mat : mMaterials) {
		if (mat.mTexId > 0) {
			hasTexture = true;
			break;
		}
	}

	glBindVertexArray(mVAO);
	for (size_t i = 0; i < mMaterials.size(); ++i)
	{
		const DgMaterial& mtl = mMaterials[i];

		// 재질 계수 uniform 설정
		glUniform3fv(glGetUniformLocation(mShaderId, "uKa"), 1, mtl.mKa);
		glUniform3fv(glGetUniformLocation(mShaderId, "uKd"), 1, mtl.mKd);
		glUniform3fv(glGetUniformLocation(mShaderId, "uKs"), 1, mtl.mKs);
		glUniform1f(glGetUniformLocation(mShaderId, "uNs"), mtl.mNs);

		// 텍스처 유무에 따라 처리
		if (hasTexture)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mtl.mTexId);
			glUniform1i(glGetUniformLocation(mShaderId, "uTex"), 0);
		}

		// 재질별 인덱스 존재 여부 확인
		const std::vector<unsigned int>& indices = mVertexIndicesPerMtl[i];
		if (indices.empty()) continue;

		// EBO 없이 임시 인덱스 전송 (draw call마다)
		GLuint tempEBO;
		glGenBuffers(1, &tempEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tempEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);

		glDeleteBuffers(1, &tempEBO);
	}

	glBindVertexArray(0);
}

void DgMesh::addVertex(DgVertex* pVert)
{
	// 정점의 인덱스를 설정하고 정점 배열에 추가한다.
	pVert->mMesh = this;
	pVert->mIdx = (int)mVerts.size();
	mVerts.push_back(*pVert);
}

void DgMesh::updateEdgeMate(DgVertex* pVert)
{
	if (pVert == nullptr)
	{
		// 메쉬의 모든 에지의 메이트 정보를 초기화 한다.
		for (DgFace& f : mFaces)
			for (DgEdge* e : f.getEdges())
				e->mMate = nullptr;

		// 각각의 정점에 대하여
		for (DgVertex& v : mVerts)
		{
			// 정점에서 시작하는 각각의 에지에 대하여
			for (DgEdge* e1 : v.mEdges)
			{
				if (e1->mMate != NULL)
					continue;

				// 에지의 다음 정점에서 시작하는 각각의 에지에 대하여
				for (DgEdge* e2 : EV(e1)->mEdges)
				{
					// e2가 e1의 mate 에지라면
					if (IS_MATE_EDGE(e1, e2))
					{
						// mate 정보를 설정한다.
						e1->mMate = e2;
						e2->mMate = e1;
						break;
					}
				}
			}
		}
	}
	else // 정점 주변의 정보를 갱신한다.
	{
		for (DgVertex* v : pVert->getOneRingVerts(false))
		{
			// 정점에서 시작하는 각각의 에지 e1에 대하여
			for (DgEdge* e1 : v->mEdges)
			{
				// 2023-10-01 추가...
				if (e1->mFace->mIdx == -1)
					continue;

				// 에지의 끝점에서 시작하는 각각의 에지 e2에 대하여
				for (DgEdge* e2 : EV(e1)->mEdges)
				{
					if (e2->mFace->mIdx == -1)
						continue;

					// e1과 e2가 mate 관계라면
					if (IS_MATE_EDGE(e1, e2))
					{
						// mate 정보를 설정한다.
						e1->mMate = e2;
						e2->mMate = e1;
						break;
					}
				}
			}
		}
	}
}

void DgMesh::updateNormal(TypeNormal normalType)
{
	// 기존의 모든 법선을 제거한다.
	for (DgNormal& n : mNormals)
		delete &n;
	mNormals.clear();
	mNormalBuffer.clear();

	// 법선 형태를 설정한다.
	mNormalType = (normalType == NORMAL_ASIS) ? mNormalType : normalType;

	// 삼각형 법선을 사용한다면
	if (mNormalType == NORMAL_FACE)
	{
		// 각각의 삼각형에 대하여
		mNormals.assign(NUM(mFaces), DgNormal(0.0, 0.0, 0.0));
#pragma omp parallel for
		for (int i = 0; i < NUM(mFaces); ++i)
		{
			// 삼각형 법선을 생성하여 리스트에 추가한다.
			DgFace* f = &mFaces[i];
			DgVec3 N;
			try {
				N = f->getFaceNormal(true);
			}
			catch (...) {
				N.setCoords(0.0, 0.0, 0.0);
			}
			DgNormal* pNormal = new DgNormal(N);
			pNormal->mIdx = i;
			mNormals[i] = pNormal;

			// 각 에지에 삼각형 법선을 할당한다.
			f->mEdge->mNormal = pNormal;
			f->mEdge->mNext->mNormal = pNormal;
			f->mEdge->mNext->mNext->mNormal = pNormal;
		}
	}
	else  if (mNormalType == NORMAL_VERTEX) // 정점 법선을 사용한다면
	{
		// 각각의 정점에 대하여
		mNormals.assign(NUM(mVerts), DgNormal(0.0, 0.0, 0.0));
#pragma omp parallel for
		for (int i = 0; i < NUM(mVerts); ++i)
		{
			// 정점 법선을 생성하고 리스트에 추가한다.
			DgNormal* pNormal = new DgNormal();
			pNormal->mIdx = i;
			mNormals[i] = pNormal;

			// 정점에서 시작하는 각각의 에지에 대하여 정점 법선을 할당한다.
			for (DgEdge* e : mVerts[i].mEdges)
				e->mNormal = pNormal;
		}

		// 각 삼각형의 법선을 계산하여 세 에지에 누적한다.
#pragma omp parallel for
		for (int i = 0; i < NUM(mFaces); ++i)
		{
			DgFace* f = &mFaces[i];
			DgVec3 N;
			try {
				N = f->getFaceNormal(true);
			}
			catch (...) {
				N.setCoords(0.0, 0.0, 0.0);
			}
			f->mEdge->mNormal->mDir += N;
			f->mEdge->mNext->mNormal->mDir += N;
			f->mEdge->mNext->mNext->mNormal->mDir += N;
		}

		// 누적된 법선을 정규화 한다.
#pragma omp parallel for
		for (int i = 0; i < NUM(mNormals); ++i)
		{
			if (mNormals[i]->mDir.isZero())
				continue;
			mNormals[i]->mDir.normalize();
		}
	}
	else
		printf("Error in DgMesh::updateNormal()...\n");
}

void DgMesh::updateBndBox()
{
	// 메쉬 정점이 없다면 리턴한다.
	if (getNumVerts() == 0)
	{
		mBndBox[0] = mBndBox[1] = DgPos(0.0, 0.0, 0.0);
		return;
	}

	// 메쉬 정점의 각 축에 대한 최대/최소 좌표를 구한다.
	mBndBox[0] = mBndBox[1] = mVerts[0].mPos;
	for (DgVertex& v : mVerts)
	{
		mBndBox[0][0] = MIN(mBndBox[0][0], v.mPos[0]);
		mBndBox[0][1] = MIN(mBndBox[0][1], v.mPos[1]);
		mBndBox[0][2] = MIN(mBndBox[0][2], v.mPos[2]);
											
		mBndBox[1][0] = MAX(mBndBox[1][0], v.mPos[0]);
		mBndBox[1][1] = MAX(mBndBox[1][1], v.mPos[1]);
		mBndBox[1][2] = MAX(mBndBox[1][2], v.mPos[2]);
	}
}

int DgMesh::getNumVerts()
{
	return (int)mVerts.size();
}

void DgMesh::computeNormal(int normalType)
{
	if (normalType == 0) // 정점 법선
	{
		int numVerts = (int)mVerts.size();
		mNormals.clear();
		mNormals.assign(numVerts, DgNormal(0.0, 0.0, 0.0));

		int numFaces = (int)mFaces.size();
		for (int i = 0; i < numFaces; ++i)
		{
			int* vidx = mFaces[i].mVertIdxs;
			glm::vec3 p0 = glm::make_vec3(mVerts[vidx[0]].mPos);
			glm::vec3 p1 = glm::make_vec3(mVerts[vidx[1]].mPos);
			glm::vec3 p2 = glm::make_vec3(mVerts[vidx[2]].mPos);
			glm::vec3 e0 = p1 - p0;
			glm::vec3 e1 = p2 - p0;
			glm::vec3 n = glm::normalize(glm::cross(e0, e1));

			mNormals[vidx[0]].mDir[0] += n[0];
			mNormals[vidx[0]].mDir[1] += n[1];
			mNormals[vidx[0]].mDir[2] += n[2];

			mNormals[vidx[1]].mDir[0] += n[0];
			mNormals[vidx[1]].mDir[1] += n[1];
			mNormals[vidx[1]].mDir[2] += n[2];

			mNormals[vidx[2]].mDir[0] += n[0];
			mNormals[vidx[2]].mDir[1] += n[1];
			mNormals[vidx[2]].mDir[2] += n[2];

			mFaces[i].mNormalIdxs[0] = vidx[0];
			mFaces[i].mNormalIdxs[1] = vidx[1];
			mFaces[i].mNormalIdxs[2] = vidx[2];
		}

		for (int i = 0; i < numVerts; ++i)
		{
			glm::vec3 n = glm::make_vec3(mNormals[i].mDir);
			n = glm::normalize(n);
			mNormals[i].mDir[0] = n[0];
			mNormals[i].mDir[1] = n[1];
			mNormals[i].mDir[2] = n[2];
		}			
	}
	else if (normalType == 1) // 삼각형 법선
	{
		int numFaces = (int)mFaces.size();
		mNormals.clear();
		mNormals.assign(numFaces, DgNormal(0.0, 0.0, 0.0));

		for (int i = 0; i < numFaces; ++i)
		{
			int* vidx = mFaces[i].mVertIdxs;

			// 각 삼각형마다 로컬 법선 생성
			glm::vec3 p0 = glm::make_vec3(mVerts[vidx[0]].mPos);
			glm::vec3 p1 = glm::make_vec3(mVerts[vidx[1]].mPos);
			glm::vec3 p2 = glm::make_vec3(mVerts[vidx[2]].mPos);
			glm::vec3 e0 = p1 - p0;
			glm::vec3 e1 = p2 - p0;
			glm::vec3 n = glm::normalize(glm::cross(e0, e1));

			mNormals[i].mDir[0] = n[0];
			mNormals[i].mDir[1] = n[1];
			mNormals[i].mDir[2] = n[2];
			mFaces[i].mNormalIdxs[0] = i;
			mFaces[i].mNormalIdxs[1] = i;
			mFaces[i].mNormalIdxs[2] = i;
		}
	}
}

bool import_obj_mtl(DgMesh* pMesh, const char* fname)
{
	std::ifstream file(fname);
	if (!file.is_open())
	{
		std::cerr << "\t재질 파일 열기 실패: " << fname << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#') continue;

		std::istringstream iss(line);
		std::string tag;
		iss >> tag;

		if (tag == "newmtl")
		{
			pMesh->mMaterials.push_back(DgMaterial());
			iss >> pMesh->mMaterials.back().mName;
		}
		else if (tag == "Ka")
		{
			float* ka = pMesh->mMaterials.back().mKa;
			iss >> ka[0] >> ka[1] >> ka[2];
		}
		else if (tag == "Kd")
		{
			float* kd = pMesh->mMaterials.back().mKd;
			iss >> kd[0] >> kd[1] >> kd[2];
		}
		else if (tag == "Ks")
		{
			float* ks = pMesh->mMaterials.back().mKs;
			iss >> ks[0] >> ks[1] >> ks[2];
		}
		else if (tag == "Ns")
		{
			iss >> pMesh->mMaterials.back().mNs;
		}
		else if (tag == "map_Kd")
		{
			std::string texFile;
			iss >> texFile;

			int width, height, channels;
			stbi_set_flip_vertically_on_load(true);
			unsigned char* data = stbi_load(texFile.c_str(), &width, &height, &channels, 0);
			if (!data) {
				std::cerr << "\t텍스처 로딩 실패: " << texFile << std::endl;
				continue;
			}

			glEnable(GL_TEXTURE_2D);
			GLuint texId;
			glGenTextures(1, &texId);
			glBindTexture(GL_TEXTURE_2D, texId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			GLenum format = (channels == 3) ? GL_RGB : (channels == 4) ? GL_RGBA : GL_RED;
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
			stbi_image_free(data);
			pMesh->mMaterials.back().mTexId = texId;
		}
	}
	return true;
}

DgMesh* import_mesh_obj(const char* fname)
{
	// 파일 오픈
	std::ifstream file(fname);
	if (!file.is_open()) {
		std::cerr << "OBJ 파일 열기 실패: " << fname << std::endl;
		return nullptr;
	}

	std::filesystem::path prevPath = std::filesystem::current_path();

	// 현재 디렉터리 변경(예, fname = "c:/models/bunny.obj")
	std::filesystem::path objPath(fname);					// objPath = c:/models/bunny.obj
	std::filesystem::path objDir = objPath.parent_path();	// objDir = c:/models/
	std::filesystem::current_path(objDir);					// 현재 디렉터리를 c:/models/ 로 변경

	// 삼각 메쉬 생성
	DgMesh* pMesh = new DgMesh();
	int currMtlIdx = -1;

	// Obj 파일 파싱
	std::string line, tag;
	while (std::getline(file, line))
	{
		// 빈 줄이나 주석 스킵
		if (line.empty() || line[0] == '#')
			continue;

		// 현재 줄에서 테그 읽음
		std::istringstream iss(line);
		iss >> tag;

		if (tag == "mtllib")	// mtllib dice.mtl
		{
			std::string mtlFile;
			iss >> mtlFile;
			if (!import_obj_mtl(pMesh, mtlFile.c_str()))
				std::cerr << "MTL 파일 로딩 실패: " << mtlFile << std::endl;
		}
		else if (tag == "v")	// v	0.0		0.0		5.0
		{
			double x, y, z;
			iss >> x >> y >> z;
			pMesh->mVerts.emplace_back(x, y, z); // pMesh->mVerts.push_back(DgVertex(x, y, z)); 와 동일
		}
		else if (tag == "vt")	// vt	0.25	0.5
		{
			double s, t;
			iss >> s >> t;
			pMesh->mTexels.emplace_back(s, t);
		}
		else if (tag == "vn")	// vn	0.0		0.0		1.0
		{
			double nx, ny, nz;
			iss >> nx >> ny >> nz;
			pMesh->mNormals.emplace_back(nx, ny, nz);
		}
		else if (tag == "usemtl")	// usemtl Dice
		{
			std::string mtlName;
			iss >> mtlName;
			currMtlIdx = -1;
			for (size_t i = 0; i < pMesh->mMaterials.size(); ++i) {
				if (pMesh->mMaterials[i].mName == mtlName) {
					currMtlIdx = static_cast<int>(i);
					break;
				}
			}
		}
		else if (tag == "f") // f	1/3/4	2/2/4	3/5/4
		{
			std::vector<int> vIdxs, tIdxs, nIdxs;
			std::string token;
			bool hasTexel = false, hasNormal = false;

			while (iss >> token)
			{
				int vi, ti, ni;
				size_t firstSlash = token.find('/');
				size_t secondSlash = token.find('/', firstSlash + 1);

				if (firstSlash == std::string::npos)		// f	1	2	3
				{
					vi = std::stoi(token) - 1;
					ti = -1;
					ni = -1;
				}
				else if (secondSlash == std::string::npos)	// f	1/3		2/2		3/5
				{
					vi = std::stoi(token.substr(0, firstSlash)) - 1;
					ti = std::stoi(token.substr(firstSlash + 1)) - 1;
					ni = -1;
					hasTexel = true;
				}
				else if (secondSlash == firstSlash + 1)		// f	1//4	2//4	3//4
				{
					vi = std::stoi(token.substr(0, firstSlash)) - 1;
					ti = -1;
					ni = std::stoi(token.substr(secondSlash + 1)) - 1;
					hasNormal = true;
				}
				else										// f	1/3/4	2/2/4	3/5/4
				{
					vi = std::stoi(token.substr(0, firstSlash)) - 1;
					ti = std::stoi(token.substr(firstSlash + 1, secondSlash - firstSlash - 1)) - 1;
					ni = std::stoi(token.substr(secondSlash + 1)) - 1;
					hasTexel = true;
					hasNormal = true;
				}
				vIdxs.push_back(vi);
				tIdxs.push_back(ti);
				nIdxs.push_back(ni);
			}

			// 다각형은 삼각형으로 분할하여 처리
			for (size_t i = 1; i + 1 < vIdxs.size(); ++i)
			{
				int v0 = vIdxs[0], v1 = vIdxs[i], v2 = vIdxs[i + 1];
				int t0 = tIdxs[0], t1 = tIdxs[i], t2 = tIdxs[i + 1];
				int n0 = nIdxs[0], n1 = nIdxs[i], n2 = nIdxs[i + 1];

				// 법선이 없으면 삼각형 법선으로 자동 계산
				//if (!hasNormal || n0 < 0 || n1 < 0 || n2 < 0)
				if (!hasNormal)
				{
					auto& p0 = pMesh->mVerts[v0].mPos;
					auto& p1 = pMesh->mVerts[v1].mPos;
					auto& p2 = pMesh->mVerts[v2].mPos;
					double e0[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
					double e1[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
					double n[3] = { e0[1] * e1[2] - e0[2] * e1[1], e0[2] * e1[0] - e0[0] * e1[2], e0[0] * e1[1] - e0[1] * e1[0] };
					double len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
					if (len > 1e-8) {
						n[0] /= len; n[1] /= len; n[2] /= len;
					}
					pMesh->mNormals.emplace_back(n[0], n[1], n[2]);
					n0 = n1 = n2 = (int)pMesh->mNormals.size() - 1;
				}

				//if (hasTexel && t0 >= 0 && t1 >= 0 && t2 >= 0)
				if (hasTexel)
					pMesh->mFaces.emplace_back(v0, v1, v2, t0, t1, t2, n0, n1, n2, currMtlIdx);
				else
					pMesh->mFaces.emplace_back(v0, v1, v2, n0, n1, n2, currMtlIdx);
			}
		}
	}

	std::filesystem::current_path(prevPath);
	file.close();
	return pMesh;
}

GLuint load_shaders(const char* vertexPath, const char* fragmentPath)
{
	// 쉐이더 파일 열기
	std::ifstream vShaderFile(vertexPath);
	std::ifstream fShaderFile(fragmentPath);

	// 파일 열기에 실패한 경우 에러 출력
	if (!vShaderFile.is_open() || !fShaderFile.is_open()) {
		std::cerr << "ERROR: Failed to open shader file(s)\n";
		return 0;
	}

	// 파일 내용을 스트림으로 읽어오기
	std::stringstream vShaderStream, fShaderStream;
	vShaderStream << vShaderFile.rdbuf();  // 정점 쉐이더 코드 읽기
	fShaderStream << fShaderFile.rdbuf();  // 프래그먼트 쉐이더 코드 읽기

	// 스트림에서 문자열로 변환
	std::string vertexCode = vShaderStream.str();
	std::string fragmentCode = fShaderStream.str();

	// C 문자열 포인터로 변환 (OpenGL 함수 호출용)
	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();

	// 쉐이더 객체 생성
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// 쉐이더 소스 설정 및 컴파일
	glShaderSource(vertexShader, 1, &vShaderCode, nullptr);
	glCompileShader(vertexShader);
	glShaderSource(fragmentShader, 1, &fShaderCode, nullptr);
	glCompileShader(fragmentShader);

	// 정점 쉐이더 컴파일 성공 여부 확인
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
		std::cerr << "ERROR: Vertex Shader Compilation Failed\n" << infoLog << std::endl;
		return 0;
	}

	// 프래그먼트 쉐이더 컴파일 성공 여부 확인
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
		std::cerr << "ERROR: Fragment Shader Compilation Failed\n" << infoLog << std::endl;
		return 0;
	}

	// 프로그램 객체 생성
	GLuint programID = glCreateProgram();

	// 쉐이더를 프로그램에 연결
	glAttachShader(programID, vertexShader);
	glAttachShader(programID, fragmentShader);

	// 프로그램 링크
	glLinkProgram(programID);

	// 링크 성공 여부 확인
	glGetProgramiv(programID, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(programID, 512, nullptr, infoLog);
		std::cerr << "ERROR: Shader Program Linking Failed\n" << infoLog << std::endl;
		return 0;
	}

	// 쉐이더 객체 삭제 (프로그램에 이미 연결되었으므로 필요 없음)
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// 최종 프로그램 ID 반환
	return programID;
}

void DgMesh::addFace(DgFace* pFace)
{
	// 삼각형의 인덱스를 설정하고 삼각형 배열에 추가한다.
	pFace->mIdx = (int)mFaces.size();
	mFaces.push_back(*pFace);
}

/**********************/
/* DgFace 클래스 구현 */
/**********************/
DgMesh* DgFace::getMesh()
{
	return mEdge->mVert->mMesh;
}

DgPos DgFace::getVertexPos(int vidx)
{
	switch (vidx)
	{
	case 0:
		return DgPos(mEdge->mVert->mPos[0], mEdge->mVert->mPos[1], mEdge->mVert->mPos[2]);
	case 1:
		return DgPos(mEdge->mNext->mVert->mPos[0], mEdge->mNext->mVert->mPos[1], mEdge->mNext->mVert->mPos[2]);
	case 2:
		return DgPos(mEdge->mNext->mNext->mVert->mPos[0], mEdge->mNext->mNext->mVert->mPos[1], mEdge->mNext->mNext->mVert->mPos[2]);
	}
	throw std::runtime_error("Invalide index...");
}

DgVertex* DgFace::getVertex(int vIdx)
{
	return getEdge(vIdx)->mVert;
}

DgEdge* DgFace::getEdge(int eIdx)
{
	switch (eIdx)
	{
	case 0:
		return mEdge;
	case 1:
		return mEdge->mNext;
	case 2:
		return mEdge->mNext->mNext;
	}
	return NULL;
}

double DgFace::getArea()
{
	DgVec3 a = getVertex(1)->mPos - getVertex(0)->mPos;
	DgVec3 b = getVertex(2)->mPos - getVertex(0)->mPos;
	return norm(a ^ b) * 0.5;
}

DgVec3 DgFace::getFaceNormal(bool bLocal)
{
	// 삼각형의 세 정점의 위치를 구하여
	DgPos& p0 = mEdge->mVert->mPos;
	DgPos& p1 = mEdge->mNext->mVert->mPos;
	DgPos& p2 = mEdge->mNext->mNext->mVert->mPos;

	// 단위 길이의 법선을 구하여 반환한다.
	DgVec3 e0 = (p1 - p0) * 1000.0;
	DgVec3 e1 = (p2 - p0) * 1000.0;
	DgVec3 N = (e0 ^ e1).normalize();
	if (bLocal)
		return N;
	else
		return getMesh()->mMC * N;
}

bool DgFace::isBndryFace()
{
	for (int i = 0; i < 3; ++i)
		if (getVertex(i)->isBndry())
			return true;
	return false;
}


/**********************/
/* DgEdge 클래스 구현 */
/**********************/
DgEdge::DgEdge(DgVertex* pVert, DgTexel* pTexel, DgNormal* pNormal)
{
	// 정점, 텍셀, 법선 정보를 에지의 시작점에 할당한다.
	mVert = pVert;
	mTexel = pTexel;
	mNormal = pNormal;

	// 다음 에지, 반대편 에지, 에지가 속한 삼각형에 대한 포인터를 초기화한다.
	mNext = NULL;
	mMate = NULL;
	mFace = NULL;

	// 시작점의 정점에 현재 에지를 추가한다.
	mVert->mEdges.push_back(this);

	// 에지 비용을 초기화 한다.
	mCostOrLen = 0.0;
}

DgEdge::~DgEdge()
{
}

std::vector<DgFace*> DgEdge::getFaces()
{
	return (mMate == nullptr)
		? std::vector<DgFace*>{ mFace }
	: std::vector<DgFace*>{ mFace, mMate->mFace };
}

bool DgEdge::isBndry()
{
	return (mMate == NULL);
}

double DgEdge::getAngle(bool bRadian)
{
	// 에지가 포함된 삼각형에서 세 점과 법선 벡터를 구한다.
	DgPos p = mVert->mPos;
	DgPos q = mNext->mVert->mPos;
	DgPos r = mNext->mNext->mVert->mPos;
	DgVec3 N = mFace->getFaceNormal(true);

	// 에지가 마주보고 있는 각도를 계산하여 반환한다.
	return (bRadian) ? angle(p - r, q - r, N, true) : angle(p - r, q - r, N, false);
}


/**********************/
/* DgVec3 클래스 구현 */
/**********************/

DgVec3::DgVec3(double x, double y, double z)
{
	mPos[0] = x;
	mPos[1] = y;
	mPos[2] = z;
}

DgVec3::DgVec3(std::initializer_list<double> coords)
{
	auto it = coords.begin();
	mPos[0] = *it;
	mPos[1] = *(it + 1);
	mPos[2] = *(it + 2);
}

DgVec3::DgVec3(const DgVec3& cpy)
{
	mPos[0] = cpy.mPos[0];
	mPos[1] = cpy.mPos[1];
	mPos[2] = cpy.mPos[2];
}

DgVec3::~DgVec3()
{
}

DgVec3& DgVec3::setCoords(double x, double y, double z)
{
	mPos[0] = x;
	mPos[1] = y;
	mPos[2] = z;
	return *this;
}

bool DgVec3::isZero(double eps) const
{
	return EQ_ZERO(mPos[0], eps) && EQ_ZERO(mPos[1], eps) && EQ_ZERO(mPos[2], eps);
}

DgVec3& DgVec3::normalize(double eps)
{
	if (isZero(eps))
	{
		throw std::runtime_error("DgVec3::normalize()...\n");
	}
	double len = norm(*this);
	mPos[0] /= len;
	mPos[1] /= len;
	mPos[2] /= len;
	return *this;
}

DgVec3& DgVec3::operator =(const DgVec3& rhs)
{
	mPos[0] = rhs.mPos[0];
	mPos[1] = rhs.mPos[1];
	mPos[2] = rhs.mPos[2];
	return *this;
}

DgVec3& DgVec3::operator +=(const DgVec3& rhs)
{
	mPos[0] += rhs.mPos[0];
	mPos[1] += rhs.mPos[1];
	mPos[2] += rhs.mPos[2];
	return *this;
}

DgVec3& DgVec3::operator -=(const DgVec3& rhs)
{
	mPos[0] -= rhs.mPos[0];
	mPos[1] -= rhs.mPos[1];
	mPos[2] -= rhs.mPos[2];
	return *this;
}

DgVec3& DgVec3::operator *=(const double& s)
{
	mPos[0] *= s;
	mPos[1] *= s;
	mPos[2] *= s;
	return *this;
}

DgVec3& DgVec3::operator /=(const double& s)
{
	if (EQ_ZERO(s, MTYPE_EPS))
	{
		throw std::runtime_error("DgVec3::operator /=(const double &s)...\n");
	}
	mPos[0] /= s;
	mPos[1] /= s;
	mPos[2] /= s;
	return *this;
}

DgVec3& DgVec3::operator ^=(const DgVec3& rhs)
{
	double x = mPos[0], y = mPos[1], z = mPos[2];
	mPos[0] = y * rhs.mPos[2] - z * rhs.mPos[1];
	mPos[1] = z * rhs.mPos[0] - x * rhs.mPos[2];
	mPos[2] = x * rhs.mPos[1] - y * rhs.mPos[0];
	return *this;
}

DgVec3 DgVec3::operator +() const
{
	return *this;
}

DgVec3 DgVec3::operator -() const
{
	return DgVec3(-mPos[0], -mPos[1], -mPos[2]);
}

double& DgVec3::operator [](const int& idx)
{
	assert(idx >= 0 && idx < 3);
	return mPos[idx];
}

const double& DgVec3::operator [](const int& idx) const
{
	assert(idx >= 0 && idx < 3);
	return mPos[idx];
}

DgVec3 operator +(const DgVec3& v, const DgVec3& w)
{
	return DgVec3(v.mPos[0] + w.mPos[0], v.mPos[1] + w.mPos[1], v.mPos[2] + w.mPos[2]);
}

DgVec3 operator -(const DgVec3& v, const DgVec3& w)
{
	return DgVec3(v.mPos[0] - w.mPos[0], v.mPos[1] - w.mPos[1], v.mPos[2] - w.mPos[2]);
}

DgVec3 operator *(const DgVec3& v, const double& s)
{
	return DgVec3(v.mPos[0] * s, v.mPos[1] * s, v.mPos[2] * s);
}

DgVec3 operator *(const double& s, const DgVec3& v)
{
	return DgVec3(v.mPos[0] * s, v.mPos[1] * s, v.mPos[2] * s);
}

double operator *(const DgVec3& v, const DgVec3& w)
{
	return (v.mPos[0] * w.mPos[0] + v.mPos[1] * w.mPos[1] + v.mPos[2] * w.mPos[2]);
}

DgVec3 operator /(const double& s, const DgVec3& v)
{
	return DgVec3(s / v.mPos[0], s / v.mPos[1], s / v.mPos[2]);
}

DgVec3 operator /(const DgVec3& v, const double& s)
{
	if (EQ_ZERO(s, 1e-10))
	{
		throw std::runtime_error("DgVec3 operator /(const DgVec3 &v, double s)...\n");
	}
	return DgVec3(v.mPos[0] / s, v.mPos[1] / s, v.mPos[2] / s);
}

DgVec3 operator ^(const DgVec3& v, const DgVec3& w)
{
	return DgVec3(
		v.mPos[1] * w.mPos[2] - v.mPos[2] * w.mPos[1],
		v.mPos[2] * w.mPos[0] - v.mPos[0] * w.mPos[2],
		v.mPos[0] * w.mPos[1] - v.mPos[1] * w.mPos[0]);
}

bool operator ==(const DgVec3& v, const DgVec3& w)
{
	return EQ(v.mPos[0], w.mPos[0], MTYPE_EPS) && EQ(v.mPos[1], w.mPos[1], MTYPE_EPS) && EQ(v.mPos[2], w.mPos[2], MTYPE_EPS);
}

bool operator !=(const DgVec3& v, const DgVec3& w)
{
	return !(v == w);
}

std::ostream& operator <<(std::ostream& os, const DgVec3& v)
{
	os << "(" << v.mPos[0] << ", " << v.mPos[1] << ", " << v.mPos[2] << ")";
	return os;
}

std::istream& operator >>(std::istream& is, DgVec3& v)
{
	is >> v.mPos[0] >> v.mPos[1] >> v.mPos[2];
	return is;
}

/*************************/
/* DgVec3 유틸 함수 구현 */
/*************************/

DgVec3 proj(const DgVec3& u, const DgVec3& v)
{
	double v_norm_sq = norm_sq(v);
	if (EQ_ZERO(v_norm_sq, MTYPE_EPS))
	{
		throw std::runtime_error("DgVec3 proj(const DgVec3 &u, const DgVec3 &v)\n");
	}
	return (u * v / v_norm_sq) * v;
}

DgVec3 ortho(const DgVec3& v)
{
	// 가장 작은 값을 찾기 위해 std::min 사용
	double min_val = std::min({ v.mPos[0], v.mPos[1], v.mPos[2] });

	// 가장 작은 값을 기준으로 ret 설정
	DgVec3 ret;
	if (min_val == v[0])
		ret.setCoords(0.0, -v[2], v[1]);
	else if (min_val == v[1])
		ret.setCoords(v[2], 0.0, -v[0]);
	else
		ret.setCoords(-v[1], v[0], 0.0);

	if (ret.isZero())
	{
		throw std::runtime_error("DgVec3 ortho(const DgVec3 &v)\n");
	}
	return ret.normalize();
}

double det(const DgVec3& u, const DgVec3& v, const DgVec3& w)
{
	// det (u, v, w) =  u * ( v ^ w) 와 같음
	return (
		u.mPos[0] * (v.mPos[1] * w.mPos[2] - v.mPos[2] * w.mPos[1]) -
		u.mPos[1] * (v.mPos[0] * w.mPos[2] - v.mPos[2] * w.mPos[0]) +
		u.mPos[2] * (v.mPos[0] * w.mPos[1] - v.mPos[1] * w.mPos[0]));
}

double norm(const DgVec3& v)
{
	return SQRT(norm_sq(v));
}

double norm_sq(const DgVec3& v)
{
	return SQR(v.mPos[0]) + SQR(v.mPos[1]) + SQR(v.mPos[2]);
}

double angle(const DgVec3& u, const DgVec3& v, bool radian)
{
	if (u.isZero() || v.isZero())
		throw std::runtime_error("Zero vector in angle()...\n");
	DgVec3 p(u);
	DgVec3 q(v);
	p.normalize();
	q.normalize();
	double cs = p * q;
	double sn = norm(p ^ q);
	return (radian) ? atan2(sn, cs) : RAD2DEG(atan2(sn, cs));
}

double angle(const DgVec3& u, const DgVec3& v, const DgVec3& axis, bool radian)
{
	if (u.isZero() || v.isZero() || axis.isZero())
		throw std::runtime_error("Zero vector in angle()...\n");
	DgVec3 p(u);
	DgVec3 q(v);
	p.normalize();
	q.normalize();
	DgVec3 r = p ^ q;

	double cs = p * q;
	double sn = norm(r);
	double theta = atan2(sn, cs);
	if (r * axis < 0.0)
		theta = 2 * M_PI - theta;

	theta = radian ? theta : RAD2DEG(theta);
	return theta;
}

/***********************/
/* intersect 함수 구현 */
/***********************/

/*!
*	\brief	평면과 경계 상자의 교차 여부를 검사한다.
*
*	\param	n[in]			평면의 법선
*	\param	p[in]			평면 위의 점
*	\param	halfsize[in]	원점을 중심으로하는 경계 상자의 각 축 길이의 반
*
*	\return 평면과 경계 상자가 교차하면 true, 아니면 false를 반환한다.
*/
static bool intersect_plane_box(DgVec3 n, DgVec3 p, DgVec3 halfsize)
{
	DgVec3 vmin, vmax;
	for (int i = 0; i < 3; ++i)
	{
		if (n[i] > 0.0)
		{
			vmin[i] = -halfsize[i] - p[i];	// -NJMP-
			vmax[i] = halfsize[i] - p[i];	// -NJMP-
		}
		else
		{
			vmin[i] = halfsize[i] - p[i];	// -NJMP-
			vmax[i] = -halfsize[i] - p[i];	// -NJMP-
		}
	}
	if (n * vmin > 0.0)
		return false;	// -NJMP-
	if (n * vmax >= 0.0)
		return true;	// -NJMP-
	return false;
}

bool intersect_tri_box(DgPos u0, DgPos u1, DgPos u2, DgPos box_min, DgPos box_max)
{
	// x 축 테스트
#define AXISTEST_X01(a, b, fa, fb)	\
		p0 = a * v0[1] - b * v0[2];	p2 = a * v2[1] - b * v2[2];	\
		if (p0 < p2) { min = p0; max = p2; } else { min = p2;  max = p0; }	\
		rad = fa * box_halfsize[1] + fb * box_halfsize[2];	\
		if (min > rad || max < -rad) return 0

#define AXISTEST_X2(a, b, fa, fb)	\
		p0 = a * v0[1] - b * v0[2];	p1 = a * v1[1] - b * v1[2]; \
		if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; } \
		rad = fa * box_halfsize[1] + fb * box_halfsize[2];   \
		if (min > rad || max < -rad) return 0

	// y 축 테스트
#define AXISTEST_Y02(a, b, fa, fb)			   \
		p0 = -a * v0[0] + b * v0[2]; p2 = -a * v2[0] + b * v2[2];	\
		if (p0 < p2) { min = p0; max = p2; } else { min = p2; max = p0; } \
		rad = fa * box_halfsize[0] + fb * box_halfsize[2];   \
		if (min > rad || max < -rad) return 0

#define AXISTEST_Y1(a, b, fa, fb)			   \
		p0 = -a * v0[0] + b * v0[2]; p1 = -a * v1[0] + b * v1[2];  	   \
		if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; } \
		rad = fa * box_halfsize[0] + fb * box_halfsize[2];   \
		if (min > rad || max < -rad) return 0

	// z 축 테스트
#define AXISTEST_Z12(a, b, fa, fb)			   \
		p1 = a * v1[0] - b * v1[1];	p2 = a * v2[0] - b * v2[1];			       	   \
		if (p2 < p1) { min = p2; max = p1; } else { min = p1; max = p2; } \
		rad = fa * box_halfsize[0] + fb * box_halfsize[1];   \
		if (min > rad || max < -rad) return 0;

#define AXISTEST_Z0(a, b, fa, fb)			   \
		p0 = a * v0[0] - b * v0[1]; p1 = a * v1[0] - b * v1[1];			           \
		if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; } \
		rad = fa * box_halfsize[0] + fb * box_halfsize[1];   \
		if (min > rad || max < -rad) return 0;

#define FINDMINMAX(x0, x1, x2, min, max) \
	min = max = x0;   \
	if (x1 < min) min = x1; \
		if (x1 > max) max = x1; \
			if (x2 < min) min = x2; \
				if (x2 > max) max = x2;

	// 경계 상자의 중심이 원점에 오도록 삼각형 정점의 좌표를 변환
	double min, max, p0, p1, p2, rad;
	DgVec3 box_halfsize = (box_max - box_min) * 0.5;
	DgPos box_cnt = box_min + box_halfsize;
	DgVec3 v0 = u0 - box_cnt;
	DgVec3 v1 = u1 - box_cnt;
	DgVec3 v2 = u2 - box_cnt;

	// 삼각형 에지를 구한다.
	DgVec3 e0 = v1 - v0;
	DgVec3 e1 = v2 - v1;
	DgVec3 e2 = v0 - v2;

	// 테스트 1: 9개의 축에 대한 SAT를 수행한다.
	double fex = abs(e0[0]);
	double fey = abs(e0[1]);
	double fez = abs(e0[2]);
	AXISTEST_X01(e0[2], e0[1], fez, fey);
	AXISTEST_Y02(e0[2], e0[0], fez, fex);
	AXISTEST_Z12(e0[1], e0[0], fey, fex);

	fex = abs(e1[0]);
	fey = abs(e1[1]);
	fez = abs(e1[2]);
	AXISTEST_X01(e1[2], e1[1], fez, fey);
	AXISTEST_Y02(e1[2], e1[0], fez, fex);
	AXISTEST_Z0(e1[1], e1[0], fey, fex);

	fex = abs(e2[0]);
	fey = abs(e2[1]);
	fez = abs(e2[2]);
	AXISTEST_X2(e2[2], e2[1], fez, fey);
	AXISTEST_Y1(e2[2], e2[0], fez, fex);
	AXISTEST_Z12(e2[1], e2[0], fey, fex);

	// 테스트 2: {x, y, z} 축에 대한 SAT를 수행한다.
	FINDMINMAX(v0[0], v1[0], v2[0], min, max);
	if (min > box_halfsize[0] || max < -box_halfsize[0]) return false;
	FINDMINMAX(v0[1], v1[1], v2[1], min, max);
	if (min > box_halfsize[1] || max < -box_halfsize[1]) return false;
	FINDMINMAX(v0[2], v1[2], v2[2], min, max);
	if (min > box_halfsize[2] || max < -box_halfsize[2]) return false;

	// 테스트 3: 삼각형이 놓인 평면과 경계 상자와의 교차 검사
	DgVec3 n = (e0 ^ e1).normalize();
	if (!intersect_plane_box(n, v0, box_halfsize))
		return false;

	return true;;
}

int intersect_tri_tri(DgPos a0, DgPos a1, DgPos a2, DgPos b0, DgPos b1, DgPos b2, DgPos& p, DgPos& q, double eps)
{
	// 삼각형, 평면, 평면에서 삼각형 각 점까지 거리
	DgPos a[3] = { a0, a1, a2 }, b[3] = { b0, b1, b2 };
	DgPlane planeA(a0, a1, a2), planeB(b0, b1, b2);
	double da[3], db[3];

	// Case 1: 삼각형 A가 평면 planeB 위/아래쪽에 있는 경우: 비교차
	for (int i = 0; i < 3; ++i)
	{
		da[i] = planeB.eval(a[i]);
		if (std::fabs(da[i]) < eps) // planeB에 거의 붙어 있다면
		{
			da[i] = 0.0;
			a[i] = ::proj(a[i], planeB);
		}
	}
	if ((da[0] > 0.0 && da[1] > 0.0 && da[2] > 0.0) || (da[0] < 0.0 && da[1] < 0.0 && da[2] < 0.0))
		return 0;	// 비교차

	// Case 1: 삼각형 B가 평면 planeA 위/아래쪽에 있는 경우: 비교차
	for (int i = 0; i < 3; ++i)
	{
		db[i] = planeA.eval(b[i]);
		if (std::fabs(db[i]) < eps)	// planeA에 거의 붙어 있다면
		{
			db[i] = 0.0;
			b[i] = ::proj(b[i], planeA);
		}
	}
	if ((db[0] > 0.0 && db[1] > 0.0 && db[2] > 0.0) || (db[0] < 0.0 && db[1] < 0.0 && db[2] < 0.0))
		return 0;	// 비교차

	// Case 2: 삼각형 A과 B가 동일 평면에 놓인 경우(교차 여부만 반환하고, 다수의 교차점은 계산하지 않음)
	if (da[0] == 0.0 && da[1] == 0.0 && da[2] == 0.0)
	{
		// Case 2(a): 경계원이 교차하지 않는 경우: 비교차
		DgPos c1 = a[2] + (a[0] - a[2]) / 3.0 + (a[1] - a[2]) / 3.0;
		DgPos c2 = b[2] + (b[0] - b[2]) / 3.0 + (b[1] - b[2]) / 3.0;
		double r1 = std::max({ dist(c1, a[0]), dist(c1, a[1]), dist(c1, a[2])});
		double r2 = std::max({ dist(c2, b[0]), dist(c2, b[1]), dist(c2, b[2]) });
		if (r1 + r2 < dist(c1, c2))	return 0;	// 비교차

		// Case 2(b): 두 삼각형의 에지쌍이 하나라도 교차하는 경우: 교차(교차점 미반환).
		DgPos r, s;
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				if (intersect_edge_edge(a[i], a[(i + 1) % 3], b[j], b[(j + 1) % 3], r, s))
					return -1;	// 동일 평면 교차

		// Case 2(c): 하나의 삼각형이 다른 삼각형의 내부에 포함된 경우
		for (int i = 0; i < 3; ++i)
		{
			DgVec3 uvw = get_barycentric_coords(a[i], b[0], b[1], b[2]);
			if (uvw[0] >= 0.0 && uvw[1] >= 0.0 && uvw[2] >= 0.0)
				return -1;	// Case 2(b)를 통과 했으니, 한 점 검사로 충분

			uvw = get_barycentric_coords(b[i], a[0], a[1], a[2]);
			if (uvw[0] >= 0.0 && uvw[1] >= 0.0 && uvw[2] >= 0.0)
				return -1;	// Case 2(b)를 통과 했으니, 한 점 검사로 충분
		}

		// Case 2(a): 한 평면에 있지만 교차하지 않는 경우: 비교차
		return 0;	// 비교차
	}

	// Case 3: (대부분의 경우)삼각형 B가 평면 planeA와 교차하는 경우
	if (db[0] != 0.0 && db[1] != 0.0 && db[2] != 0.0)
	{
		// 삼각형 B과 평면 planeA의 교차 선분 rs를 구한다.
		DgPos r, s;
		for (int i0 = 0; i0 < 3; ++i0)
		{
			int i1 = (i0 + 1) % 3, i2 = (i0 + 2) % 3;
			if (db[i0] * db[i1] > 0.0)
			{
				r = b[i1] + (db[i1] / (db[i1] - db[i2])) * (b[i2] - b[i1]);
				s = b[i2] + (db[i2] / (db[i2] - db[i0])) * (b[i0] - b[i2]);
				break;
			}
		}

		// 교차선분 rs와 삼각형 A와 교차 선분 pq를 구한다.
		return intersect_edge_tri(r, s, a[0], a[1], a[2], p, q);
	}

	// Case 4: 삼각형 B의 한 점 또는 두 점이 평면 planeA에 놓인 경우
	int i0 = (db[0] == 0.0) ? 0 : (db[1] == 0.0) ? 1 : 2;
	int i1 = (i0 + 1) % 3;
	int i2 = (i0 + 2) % 3;
	DgPos r(b[i0]), s(b[i0]);

	// 삼각형 B와 평면 planeA와 나머지 교차점 계산
	if (db[i1] * db[i2] <= 0.0)
		s = b[i1] + (db[i1] / (db[i1] - db[i2])) * (b[i2] - b[i1]);
	else
		return 0; // 한 점 교차인 경우, 비교차

	// 교차선분 rs와 삼각형 A와 교차 선분 pq를 구한다.
	return intersect_edge_tri(r, s, a[0], a[1], a[2], p, q);
}

/*********************/
/* DgPos 클래스 구현 */
/*********************/
DgPos::DgPos(double x, double y, double z)
{
	mPos[0] = x;
	mPos[1] = y;
	mPos[2] = z;
}

DgPos::DgPos(double* Coords)
{
	mPos[0] = Coords[0];
	mPos[1] = Coords[1];
	mPos[2] = Coords[2];
}

DgPos::DgPos(float* Coords)
{
	mPos[0] = (double)Coords[0];
	mPos[1] = (double)Coords[1];
	mPos[2] = (double)Coords[2];
}

DgPos::DgPos(const DgPos& cpy)
{
	mPos[0] = cpy.mPos[0];
	mPos[1] = cpy.mPos[1];
	mPos[2] = cpy.mPos[2];
}

DgPos::~DgPos()
{
}

DgPos& DgPos::setCoords(double x, double y, double z)
{
	mPos[0] = x;
	mPos[1] = y;
	mPos[2] = z;
	return *this;
}

DgPos lerp(const DgPos& p, const DgPos& q, double t)
{
	double x = (1.0 - t) * p[0] + t * q[0];
	double y = (1.0 - t) * p[1] + t * q[1];
	double z = (1.0 - t) * p[2] + t * q[2];
	return DgPos(x, y, z);
}

double DgPos::distance_sq(const DgPos& p, const DgPos& q)
{
	return (SQR(p.mPos[0] - q.mPos[0]) + SQR(p.mPos[1] - q.mPos[1]) + SQR(p.mPos[2] - q.mPos[2]));
}

double DgPos::dist(const DgPos& p, const DgPos& q)
{
	return SQRT(distance_sq(p, q));
}

DgPos& DgPos::operator =(const DgPos& rhs)
{
	mPos[0] = rhs.mPos[0];
	mPos[1] = rhs.mPos[1];
	mPos[2] = rhs.mPos[2];
	return *this;
}

DgPos& DgPos::operator +=(const DgVec3& v)
{
	mPos[0] += v.mPos[0];
	mPos[1] += v.mPos[1];
	mPos[2] += v.mPos[2];
	return *this;
}

DgPos& DgPos::operator-=(const DgVec3& v)
{
	mPos[0] -= v.mPos[0];
	mPos[1] -= v.mPos[1];
	mPos[2] -= v.mPos[2];
	return *this;
}

double& DgPos::operator [](const int& idx)
{
	assert(idx >= 0 && idx < 3);
	return mPos[idx];
}

const double& DgPos::operator [](const int& idx) const
{
	assert(idx >= 0 && idx < 3);
	return mPos[idx];
}

DgVec3 operator -(const DgPos& p, const DgPos& q)
{
	return DgVec3(
		p.mPos[0] - q.mPos[0],
		p.mPos[1] - q.mPos[1],
		p.mPos[2] - q.mPos[2]);
}

DgPos operator -(const DgPos& p, const DgVec3& v)
{
	return DgPos(
		p.mPos[0] - v.mPos[0],
		p.mPos[1] - v.mPos[1],
		p.mPos[2] - v.mPos[2]);
}

DgPos operator +(const DgPos& p, const DgVec3& v)
{
	return DgPos(
		p.mPos[0] + v.mPos[0],
		p.mPos[1] + v.mPos[1],
		p.mPos[2] + v.mPos[2]);
}

DgPos operator +(const DgVec3& v, const DgPos& p)
{
	return DgPos(
		p.mPos[0] + v.mPos[0],
		p.mPos[1] + v.mPos[1],
		p.mPos[2] + v.mPos[2]);
}

bool operator ==(const DgPos& p, const DgPos& q)
{
	return	(std::fabs(p.mPos[0] - q.mPos[0]) < MTYPE_EPS) &&
		(std::fabs(p.mPos[1] - q.mPos[1]) < MTYPE_EPS) &&
		(std::fabs(p.mPos[2] - q.mPos[2]) < MTYPE_EPS);
}

bool operator !=(const DgPos& p, const DgPos& q)
{
	return !(p == q);
}

bool operator <(const DgPos& p, const DgPos& q)
{
	if (1)
	{
		if (std::fabs(p.mPos[0] - q.mPos[0]) > MTYPE_EPS)
			return p.mPos[0] < q.mPos[0];
		if (std::fabs(p.mPos[1] - q.mPos[1]) > MTYPE_EPS)
			return p.mPos[1] < q.mPos[1];
		if (std::fabs(p.mPos[2] - q.mPos[2]) > MTYPE_EPS)
			return p.mPos[2] < q.mPos[2];
		return false;
	}
	else
	{
		if (std::fabs(p.mPos[0] - q.mPos[0]) > MTYPE_EPS)
			return p.mPos[0] < q.mPos[0];
		if (std::fabs(p.mPos[1] - q.mPos[1]) > MTYPE_EPS)
			return p.mPos[1] < q.mPos[1];
		return p.mPos[2] < q.mPos[2];
	}
}

std::ostream& operator <<(std::ostream& os, const DgPos& p)
{
	os << "(" << std::setw(5) << p.mPos[0] << ", " << std::setw(5) << p.mPos[1] << ", " << std::setw(5) << p.mPos[2] << ")";
	return os;
}

std::istream& operator >>(std::istream& is, DgPos& p)
{
	is >> p.mPos[0] >> p.mPos[1] >> p.mPos[2];
	return is;
}

/************************/
/* DgVertex 클래스 구현 */
/************************/

std::vector<DgEdge*> DgVertex::getEdges(bool bCCW)
{
	// 반시계 방향으로 정렬하지 않는다면 에지 배열을 반환한다.
	if (!bCCW)
		return mEdges;

	// 고립 정점이라면 빈 배열을 반환한다.
	if (mEdges.empty())
		return std::vector<DgEdge*>();

	// 시계 방향 순회
	std::vector<DgEdge*> edgeList;
	DgEdge* e = mEdges[0];
	do {
		if (e->mMate == nullptr) break; // 조건(1): 경계 에지를 만난 경우
		edgeList.emplace_back(e);
		e = e->mMate->mNext;
	} while (e != mEdges[0]); // 조건(2): 시작에지로 되돌아온 경우

	// 경계 정점인 경우: 조건(1)로 나온 경우
	if (mEdges.size() != edgeList.size())
	{
		// 반시계 방향 순회
		edgeList.clear();
		do {
			edgeList.emplace_back(e);
			e = e->mNext->mNext->mMate;
		} while (e != nullptr);

		// Non-manifold 정점의 경우
		if (mEdges.size() != edgeList.size())
		{
			edgeList.clear();
			throw std::runtime_error("Non-manifold vertex...\n");
		}
	}
	else // 경계 정점이 아닌 경우: 조건(2)로 나온 경우
		std::reverse(edgeList.begin(), edgeList.end());

	return edgeList;
}

std::vector<DgVertex*> DgVertex::getOneRingVerts(bool bCCW)
{
	std::vector<DgVertex*> verts;
	for (DgEdge* e : getEdges(bCCW))
	{
		verts.push_back(EV(e));
		if (PREV(e)->mMate == NULL)
			verts.push_back(PREV(e)->mVert);
	}
	return verts;
}

DgVec3 DgVertex::getAvgNormal(bool bWgt)
{
	// 예외 처리
	DgVec3 N;
	if (mEdges.empty())
		return N;

	// 각도 가중치를 고려하는 경우
	if (bWgt)
	{
		std::vector<double> weights;
		for (DgEdge* e : mEdges)
		{
			try {
				weights.push_back(e->mNext->getAngle(true));
			}
			catch (...)
			{
				weights.push_back(0.0);
			}
		}

		double totWgt = std::accumulate(weights.begin(), weights.end(), 0.0);
		for (int i = 0; i < (int)mEdges.size(); ++i)
		{
			try {
				double wgt = weights[i] / totWgt;
				N += wgt * mEdges[i]->mFace->getFaceNormal(true);
			}
			catch (...) {
				continue;
			}
		}
	}
	else // 단순 평균을 구하는 경우
	{
		for (DgEdge* e : mEdges)
		{
			try {
				N += e->mFace->getFaceNormal(true);
			}
			catch (...) {
				continue;
			}
		}
	}
	if (!N.isZero())
		N.normalize();
	return N;
}

bool DgVertex::isBndry()
{
	for (DgEdge* e : mEdges)
		if (e->mMate == NULL)
			return true;
	return false;
}

/************************/
/* DgNormal 클래스 구현 */
/************************/
