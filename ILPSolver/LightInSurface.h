#pragma once

#include "math/Vector3.h"
#include "scene/Scene.h"
#include <QString>

class LightInSurface
{
public:
	LightInSurface(Scene *scene, const QString& lightId, const QString& surfaceId);

	QString lightId();
	Vector3 generatePointNeighbourhood(const Vector3 center, float radius) const;
	virtual ~LightInSurface();
private:
	bool pointInSurface(optix::float2 point) const;
private:
	QString m_lightId;
	QString m_surfaceId;
	int objectId;
	Scene *scene;

	optix::float3 base; // quad origin in world coordinates

	// orthonormal base of the plane containing the surface.
	// A point in the plane is represented as: c + alfa * u + beta * v
	//										   where alfa and beta are arbitrary numbers 
	optix::float3 u, v; 

	optix::float2 a,b,c; // other points in uv coordinates (base is 0,0 in uv coordinates).
};
