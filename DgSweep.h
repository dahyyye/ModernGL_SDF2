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
		bool useGPU = false,
		bool useSegment = false
    );

    static DgVolume* generateTestCPU(DgVolume* brush, const DgTrajectory& trajectory,
        int resolution, int samplingSteps);

private:
    static GLuint sComputeShader;
	static GLuint sTransformSSBO;
	static bool sInitialized;

    static bool initializeGPU();

    static DgVolume* generateCPU(DgVolume* brush, const DgTrajectory& trajectory,
        int resolution, int timeSteps);
    static DgVolume* generateGPU(DgVolume* brush, const DgTrajectory& trajectory,
        int resolution, int timeSteps);
    static DgVolume* generateSegmentCPU(DgVolume* brush, const DgTrajectory& trajectory,
        int resolution, int samplingSteps, int lamda = 8);
    
};