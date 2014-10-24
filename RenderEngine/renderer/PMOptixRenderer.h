/* 
 * Copyright (c) 2014 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#pragma once

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_aabb_namespace.h>
#include "render_engine_export_api.h"
#include "OptixRenderer.h"
#include "math/AAB.h"
#include "logging/Logger.h"
#include <vector>
#include <string>


class ComputeDevice;
class RenderServerRenderRequestDetails;
class Scene;
class Camera;
template <class Key, class T> class QMap;

class PMOptixRenderer: public OptixRenderer
{
public:

    RENDER_ENGINE_EXPORT_API PMOptixRenderer();
    RENDER_ENGINE_EXPORT_API ~PMOptixRenderer();

    RENDER_ENGINE_EXPORT_API void initScene(Scene & scene);
    RENDER_ENGINE_EXPORT_API void initialize(const ComputeDevice & device, Logger *logger);

    RENDER_ENGINE_EXPORT_API void renderNextIteration(unsigned long long iterationNumber, unsigned long long localIterationNumber, 
        float PPMRadius, const RenderServerRenderRequestDetails & details);
	RENDER_ENGINE_EXPORT_API void render(unsigned int photonLaunchWidth, unsigned int height, unsigned int width, const Camera camera, bool calculateRadiance);
	RENDER_ENGINE_EXPORT_API void genPhotonMap(unsigned int photonLaunchWidth);
    RENDER_ENGINE_EXPORT_API void getOutputBuffer(void* data);
	RENDER_ENGINE_EXPORT_API std::vector<unsigned int> getHitCount();
    RENDER_ENGINE_EXPORT_API unsigned int getWidth() const;
    RENDER_ENGINE_EXPORT_API unsigned int getHeight() const;
    RENDER_ENGINE_EXPORT_API unsigned int getScreenBufferSizeBytes() const;
	RENDER_ENGINE_EXPORT_API std::vector<float> getRadiance();

    const static unsigned int PHOTON_GRID_MAX_SIZE;
private:
	const static unsigned int MAX_BOUNCES;
    const static unsigned int MAX_PHOTON_COUNT;

    void initDevice(const ComputeDevice & device);
	void compile();
    void loadObjGeometry( const std::string& filename, optix::Aabb& bbox );
    void initializeRandomStates();
    void createUniformGridPhotonMap(float ppmRadius);
    void initializeStochasticHashPhotonMap(float ppmRadius);
    void createPhotonKdTreeOnCPU();
	unsigned int getNumPhotons() const;
	void resizeBuffers(unsigned int width, unsigned int height, unsigned int generateOutput);
	void countHitCountPerObject();
	
	optix::Context m_context;
    optix::Buffer m_outputBuffer;
    optix::Buffer m_photons;
    optix::Buffer m_photonKdTree;
    optix::Buffer m_hashmapOffsetTable;
    optix::Buffer m_photonsHashCells;
    optix::Buffer m_raytracePassOutputBuffer;
    optix::Buffer m_directRadianceBuffer;
    optix::Buffer m_indirectRadianceBuffer;
    optix::Group  m_sceneRootGroup;
    optix::Buffer m_volumetricPhotonsBuffer;
    optix::Buffer m_lightBuffer;
    optix::Buffer m_randomStatesBuffer;
	optix::Buffer m_hitCountBuffer;
	optix::Buffer m_rawRadianceBuffer;
	optix::uint3 m_gridSize;
    float m_spatialHashMapCellSize;
    AAB m_sceneAABB;
	float m_scenePPMRadius;
    unsigned int m_spatialHashMapNumCells;
    unsigned int m_width;
    unsigned int m_height;
	unsigned int m_photonWidth;
	unsigned int m_sceneObjects;
    bool m_initialized;
    int m_optixDeviceOrdinal;
	std::vector<std::string> m_objectIdToName;
	QMap<QString, optix::Group *>* m_groups;
	Logger *m_logger;

};