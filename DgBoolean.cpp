#include "DgViewer.h"

DgMesh* DgBooleanUnion(const DgMesh& A, const DgMesh& B, bool Fairing)
{
	// 1) A와 B 메쉬의 부호거리장 계산
	// 2) 두 부호거리장의 합집합 연산
	// 3) 등거리면 추출하여 합집합 메쉬 생성
	// 4) Fairing 처리
	// 5) 합집합 메쉬 반환 (부호거리장, 또 격자 샘플링해서 계산해야하나)
	return nullptr; // 임시
}

DgMesh* DgBooleanIntersection(const DgMesh& A, const DgMesh& B, bool Fairing)
{
	// 1) A와 B 메쉬의 부호거리장 계산
	// 2) 두 부호거리장의 교집합 연산
	// 3) 등거리면 추출하여 교집합 메쉬 생성
	// 4) Fairing 처리
	// 5) 교집합 메쉬 반환 (부호거리장, 또 격자 샘플링해서 계산해야하나)
	return nullptr; // 임시
}

DgMesh* DgBooleanDifference(const DgMesh& A, const DgMesh& B, bool Fairing)
{
	// 1) A와 B 메쉬의 부호거리장 계산
	// 2) 두 부호거리장의 차집합 연산
	// 3) 등거리면 추출하여 차집합 메쉬 생성
	// 4) Fairing 처리
	// 5) 차집합 메쉬 반환 (부호거리장, 또 격자 샘플링해서 계산해야하나)
	return nullptr; // 임시
}