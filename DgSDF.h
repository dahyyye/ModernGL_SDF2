#pragma once

/*!
 *	\class	DgSDF
 *	\brief	메쉬의 부호거리장 공식을 표현하는 클래스
 */
class DgSDF
{
public:
    int id;                   // 고유 ID
    int type;                 // 0=sphere,1=box,...

    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1, 0, 0, 0);

    float offset = 0.0f;
    glm::vec3 color = glm::vec3(0.7f);

public:
    DgSDF(int _id, int _type)
        : id(_id), type(_type)
    {
    }
    virtual ~DgSDF() = default;
};

class Sphere : public DgSDF
{
public:
    float radius;

    Sphere(int _id, float r)
        : DgSDF(_id, 0), radius(r)
    { }
};

class Box : public DgSDF
{
public:
    glm::vec3 halfSize;

    Box(int _id, glm::vec3 hs) 
        : DgSDF(_id, 1), halfSize(hs)
    { }
};

class Torus : public DgSDF
{
public:
    glm::vec2 radii; // major, minor

    Torus(int _id, glm::vec2 r)
        : DgSDF(_id, 2), radii(r)
    { }
};

class RoundBox : public DgSDF
{
public:
    glm::vec3 halfSize;
    float radius;

    RoundBox(int _id, glm::vec3 hs, float r)
        : DgSDF(_id, 3), halfSize(hs), radius(r)
    { }
};

class BoxFrame : public DgSDF
{
public:
    glm::vec3 halfSize;
    float thickness;

    BoxFrame(int _id, glm::vec3 hs, float t)
        : DgSDF(_id, 4), halfSize(hs), thickness(t)
    {
    }
};

class CapTorus : public DgSDF
{
public:
    glm::vec2 sc;   // sin,cos
    float radiusA;
    float radiusB;

    CapTorus(int _id, glm::vec2 _sc, float a, float b)
        : DgSDF(_id, 5), sc(_sc), radiusA(a), radiusB(b)
    {
    }
};

class Link : public DgSDF
{
public:
    float len;
    float radiusA;
    float radiusB;

    Link(int _id, float l, float A, float B)
        : DgSDF(_id, 6), len(l), radiusA(A), radiusB(B)
    {
    }
};

class Cylinder : public DgSDF
{
public:
    glm::vec2 centerXZ;
    float radius;
    float halfHeight;

    Cylinder(int _id, glm::vec2 c, float r, float h)
        : DgSDF(_id, 7), centerXZ(c), radius(r), halfHeight(h)
    {
    }
};

class Cone : public DgSDF
{
public:
    glm::vec2 centerXZ;
    float height;
    float radius;

    Cone(int _id, glm::vec2 c, float h, float r)
        : DgSDF(_id, 8), centerXZ(c), height(h), radius(r)
    {
    }
};