#pragma once

#include "DgViewer.h"

class DgVolume;
class DgTrajectory;

class DgSweep {
public:
    static DgVolume* generateSweptVolume(
        DgVolume* brush,
        const DgTrajectory& trajectory,
        int resolution = 128,
        int samplingSteps = 100,
		bool useGPU = false
    );

    static DgVolume* generateBrentCPU(DgVolume* brush, const DgTrajectory& trajectory,
        int resolution, int samplingSteps);

    static DgVolume* generateBrentGPU(DgVolume* brush, const DgTrajectory& trajectory,
        int resolution, int samplingSteps, bool skipreadback);
    static void fastSweeping(DgVolume* vol);

private:
    static GLuint sComputeShader;
	static GLuint sTransformSSBO;
	static bool sInitialized;

    static GLuint sBrentComputeShader;
    static GLuint sBrentTransformSSBO;
    static bool sBrentInitialized;

    static bool initializeGPU();
    static bool initializeBrentGPU();

    static DgVolume* generateCPU(DgVolume* brush, const DgTrajectory& trajectory,
        int resolution, int timeSteps);
    static DgVolume* generateGPU(DgVolume* brush, const DgTrajectory& trajectory,
        int resolution, int timeSteps); 
};