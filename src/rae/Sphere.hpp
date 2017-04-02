#pragma once

#include <glm/glm.hpp>
using glm::vec3;

#include "Hitable.hpp"

namespace rae
{

class Ray;
class HitRecord;
class Material;
class Aabb;

class Sphere : public Hitable
{
public:
	Sphere(){}
	Sphere(vec3 setCenter, float setRadius, Material* setMaterial)
		: center(setCenter),
		radius(setRadius),
		material(setMaterial)
	{}

	~Sphere();

	virtual bool hit(const Ray& ray, float t_min, float t_max, HitRecord& record) const;
	virtual Aabb getAabb(float t0, float t1) const;

	vec3 center;
	float radius;
	Material* material;
};

}
