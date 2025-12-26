#pragma once

class DgVolume;
class DgTrajectory;

class DgSweep {
public:
    static DgVolume* generateSweptVolume(
        DgVolume* brush,
        const DgTrajectory& trajectory,
        int resolution = 64,
        int timeSteps = 100
    );
};