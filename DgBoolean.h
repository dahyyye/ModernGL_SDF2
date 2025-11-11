#pragma once

/*!
 *	\biref	두 메쉬의 합집합 메쉬를 계산한다.
 *
 *	\param	A[in]			첫 번째 메쉬
 *	\param	B[in]			두 번째 메쉬
 *	\param	Fairing[in]		연산 후, 교차영역의 Fairing 여부
 *
 *	\return	두 메쉬의 합집합 메쉬를 반환한다.
 */
DgMesh* DgBooleanUnion(const DgMesh& A, const DgMesh& B, bool Fairing);

/*!
 *	\biref	두 메쉬의 교집합 메쉬를 계산한다.
 *
 *	\param	A[in]			첫 번째 메쉬
 *	\param	B[in]			두 번째 메쉬
 *	\param	Fairing[in]		연산 후, 교차영역의 Fairing 여부
 *
 *	\return	두 메쉬의 교집합 메쉬를 반환한다.
 */
DgMesh* DgBooleanIntersection(const DgMesh& A, const DgMesh& B, bool Fairing);

/*!
 *	\biref	두 메쉬의 차집합(A - B) 메쉬를 계산한다.
 *
 *	\param	A[in]			첫 번째 메쉬
 *	\param	B[in]			두 번째 메쉬
 *	\param	Fairing[in]		연산 후, 교차영역의 Fairing 여부
 *
 *	\return	두 메쉬의 차집합 메쉬를 반환한다.
 */
DgMesh* DgBooleanDifference(const DgMesh& A, const DgMesh& B, bool Fairing);