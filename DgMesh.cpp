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

double& DgPos::operator[](const int& idx)
{
	assert(idx >= 0 && idx < 3);
	return mPos[idx];
}

const double& DgPos::operator[](const int& idx) const
{
	assert(idx >= 0 && idx < 3);
	return mPos[idx];
}

DgPos lerp(const DgPos& p, const DgPos& q, double t)
{
	double x = (1.0 - t) * p[0] + t * q[0];
	double y = (1.0 - t) * p[1] + t * q[1];
	double z = (1.0 - t) * p[2] + t * q[2];
	return DgPos(x, y, z);
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


/**********************/
/* DgVec3 클래스 구현 */
/**********************/

DgVec3::DgVec3(double x, double y, double z)
{
	mCoords[0] = x;
	mCoords[1] = y;
	mCoords[2] = z;
}

DgVec3::DgVec3(std::initializer_list<double> coords)
{
	auto it = coords.begin();
	mCoords[0] = *it;
	mCoords[1] = *(it + 1);
	mCoords[2] = *(it + 2);
}

DgVec3::DgVec3(const DgVec3& cpy)
{
	mCoords[0] = cpy.mCoords[0];
	mCoords[1] = cpy.mCoords[1];
	mCoords[2] = cpy.mCoords[2];
}

DgVec3::~DgVec3()
{
}

DgVec3& DgVec3::setCoords(double x, double y, double z)
{
	mCoords[0] = x;
	mCoords[1] = y;
	mCoords[2] = z;
	return *this;
}

bool DgVec3::isZero(double eps) const
{
	return EQ_ZERO(mCoords[0], eps) && EQ_ZERO(mCoords[1], eps) && EQ_ZERO(mCoords[2], eps);
}

DgVec3& DgVec3::normalize(double eps)
{
	if (isZero(eps))
	{
		throw std::runtime_error("DgVec3::normalize()...\n");
	}
	double len = norm(*this);
	mCoords[0] /= len;
	mCoords[1] /= len;
	mCoords[2] /= len;
	return *this;
}

DgVec3& DgVec3::operator =(const DgVec3& rhs)
{
	mCoords[0] = rhs.mCoords[0];
	mCoords[1] = rhs.mCoords[1];
	mCoords[2] = rhs.mCoords[2];
	return *this;
}

DgVec3& DgVec3::operator +=(const DgVec3& rhs)
{
	mCoords[0] += rhs.mCoords[0];
	mCoords[1] += rhs.mCoords[1];
	mCoords[2] += rhs.mCoords[2];
	return *this;
}

DgVec3& DgVec3::operator -=(const DgVec3& rhs)
{
	mCoords[0] -= rhs.mCoords[0];
	mCoords[1] -= rhs.mCoords[1];
	mCoords[2] -= rhs.mCoords[2];
	return *this;
}

DgVec3& DgVec3::operator *=(const double& s)
{
	mCoords[0] *= s;
	mCoords[1] *= s;
	mCoords[2] *= s;
	return *this;
}

DgVec3& DgVec3::operator /=(const double& s)
{
	if (EQ_ZERO(s, MTYPE_EPS))
	{
		throw std::runtime_error("DgVec3::operator /=(const double &s)...\n");
	}
	mCoords[0] /= s;
	mCoords[1] /= s;
	mCoords[2] /= s;
	return *this;
}

DgVec3& DgVec3::operator ^=(const DgVec3& rhs)
{
	double x = mCoords[0], y = mCoords[1], z = mCoords[2];
	mCoords[0] = y * rhs.mCoords[2] - z * rhs.mCoords[1];
	mCoords[1] = z * rhs.mCoords[0] - x * rhs.mCoords[2];
	mCoords[2] = x * rhs.mCoords[1] - y * rhs.mCoords[0];
	return *this;
}

DgVec3 DgVec3::operator +() const
{
	return *this;
}

DgVec3 DgVec3::operator -() const
{
	return DgVec3(-mCoords[0], -mCoords[1], -mCoords[2]);
}

double& DgVec3::operator [](const int& idx)
{
	assert(idx >= 0 && idx < 3);
	return mCoords[idx];
}

const double& DgVec3::operator [](const int& idx) const
{
	assert(idx >= 0 && idx < 3);
	return mCoords[idx];
}

DgVec3 operator +(const DgVec3& v, const DgVec3& w)
{
	return DgVec3(v.mCoords[0] + w.mCoords[0], v.mCoords[1] + w.mCoords[1], v.mCoords[2] + w.mCoords[2]);
}

DgVec3 operator -(const DgVec3& v, const DgVec3& w)
{
	return DgVec3(v.mCoords[0] - w.mCoords[0], v.mCoords[1] - w.mCoords[1], v.mCoords[2] - w.mCoords[2]);
}

DgVec3 operator *(const DgVec3& v, const double& s)
{
	return DgVec3(v.mCoords[0] * s, v.mCoords[1] * s, v.mCoords[2] * s);
}

DgVec3 operator *(const double& s, const DgVec3& v)
{
	return DgVec3(v.mCoords[0] * s, v.mCoords[1] * s, v.mCoords[2] * s);
}

double operator *(const DgVec3& v, const DgVec3& w)
{
	return (v.mCoords[0] * w.mCoords[0] + v.mCoords[1] * w.mCoords[1] + v.mCoords[2] * w.mCoords[2]);
}

DgVec3 operator /(const double& s, const DgVec3& v)
{
	return DgVec3(s / v.mCoords[0], s / v.mCoords[1], s / v.mCoords[2]);
}

DgVec3 operator /(const DgVec3& v, const double& s)
{
	if (EQ_ZERO(s, 1e-10))
	{
		throw std::runtime_error("DgVec3 operator /(const DgVec3 &v, double s)...\n");
	}
	return DgVec3(v.mCoords[0] / s, v.mCoords[1] / s, v.mCoords[2] / s);
}

DgVec3 operator ^(const DgVec3& v, const DgVec3& w)
{
	return DgVec3(
		v.mCoords[1] * w.mCoords[2] - v.mCoords[2] * w.mCoords[1],
		v.mCoords[2] * w.mCoords[0] - v.mCoords[0] * w.mCoords[2],
		v.mCoords[0] * w.mCoords[1] - v.mCoords[1] * w.mCoords[0]);
}

bool operator ==(const DgVec3& v, const DgVec3& w)
{
	return EQ(v.mCoords[0], w.mCoords[0], MTYPE_EPS) && EQ(v.mCoords[1], w.mCoords[1], MTYPE_EPS) && EQ(v.mCoords[2], w.mCoords[2], MTYPE_EPS);
}

bool operator !=(const DgVec3& v, const DgVec3& w)
{
	return !(v == w);
}

std::ostream& operator <<(std::ostream& os, const DgVec3& v)
{
	os << "(" << v.mCoords[0] << ", " << v.mCoords[1] << ", " << v.mCoords[2] << ")";
	return os;
}

std::istream& operator >>(std::istream& is, DgVec3& v)
{
	is >> v.mCoords[0] >> v.mCoords[1] >> v.mCoords[2];
	return is;
}

/**********************/
/* DgVec3 클래스 구현 */
/**********************/

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
	double min_val = std::min({ v.mCoords[0], v.mCoords[1], v.mCoords[2] });

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
		u.mCoords[0] * (v.mCoords[1] * w.mCoords[2] - v.mCoords[2] * w.mCoords[1]) -
		u.mCoords[1] * (v.mCoords[0] * w.mCoords[2] - v.mCoords[2] * w.mCoords[0]) +
		u.mCoords[2] * (v.mCoords[0] * w.mCoords[1] - v.mCoords[1] * w.mCoords[0]));
}

double norm(const DgVec3& v)
{
	return SQRT(norm_sq(v));
}

double norm_sq(const DgVec3& v)
{
	return SQR(v.mCoords[0]) + SQR(v.mCoords[1]) + SQR(v.mCoords[2]);
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