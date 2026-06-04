#pragma once

class DgVolume;
class DgMesh;

/*!
 *  \brief  Marching Cubes로 DgVolume의 등치면을 메시로 추출
 *          (Lorensen & Cline 1987; Paul Bourke 표준 테이블)
 *
 *  \param[in]  vol         입력 SDF 볼륨 (mData, mDim, mMin, mSpacing 사용)
 *  \param[in]  isoLevel    iso-surface 값 (기본 0.0, 일반적으로 vol->mOffset)
 *
 *  \return 추출된 DgMesh* (소유권 호출자). 추출된 삼각형이 없으면 nullptr.
 *          좌표계: 볼륨 로컬 공간 (vol->mMin 원점, vol->mSpacing 스케일).
 *          mPosition/mRotation/mScale은 적용하지 않음.
 */
DgMesh* extractMeshMC(const DgVolume* vol, float isoLevel = 0.0f);

/*!
 *  \brief  DgMesh를 Wavefront OBJ 파일로 저장 (v, vn, f만)
 *
 *  \param[in]  mesh        저장할 메시
 *  \param[in]  filename    파일 경로
 *
 *  \return 성공 여부
 */
bool save_mesh_obj(const DgMesh* mesh, const char* filename);