/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
 */

#include "Diffuse.h"
#include "renderer/RayType.h"
#include "util/RelPath.h"

bool Diffuse::m_optixMaterialIsCreated = false;
optix::Material Diffuse::m_optixMaterial;

Diffuse::Diffuse(const Vector3 & Kd)
{
    this->Kd = Kd;
}

optix::Material Diffuse::getOptixMaterial(optix::Context & context, bool useHoleCheckProgram)
{
    if(!m_optixMaterialIsCreated)
    {
        m_optixMaterial = context->createMaterial();
        optix::Program radianceProgram = context->createProgramFromPTXFile( relativePathToExe("Diffuse.cu.ptx"), "closestHitRadiance");
        optix::Program photonProgram = context->createProgramFromPTXFile( relativePathToExe("Diffuse.cu.ptx"), "closestHitPhoton");
        m_optixMaterial->setClosestHitProgram(RayType::RADIANCE, radianceProgram);
        m_optixMaterial->setClosestHitProgram(RayType::RADIANCE_IN_PARTICIPATING_MEDIUM, radianceProgram);
        m_optixMaterial->setClosestHitProgram(RayType::PHOTON, photonProgram);
        m_optixMaterial->setClosestHitProgram(RayType::PHOTON_IN_PARTICIPATING_MEDIUM, photonProgram);
        m_optixMaterial->validate();

		this->registerMaterialWithShadowProgram(context, m_optixMaterial, useHoleCheckProgram);
        m_optixMaterialIsCreated = true;
    }
    return m_optixMaterial;
}

/*
// Register any material-dependent values to be available in the optix program.
*/

void Diffuse::registerGeometryInstanceValues(optix::GeometryInstance & instance )
{
    instance["Kd"]->setFloat(this->Kd);
}

Material* Diffuse::clone()
{
	return new Diffuse(*this);
}
