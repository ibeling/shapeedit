#include "poisson.h"

// access to the original (undeformed) mesh
//
extern Mesh g_originalMesh;

// you may have to tweak this to get smooth motion
//
int g_iterationsPerSecond = 8;

// This function is called at the beginning of the program
// before any handles have been added.
//
void initPoisson(Mesh& m) {
// TODO
//
}

// This function is called immediately after each movement
// with an updated set of handles and the most recent mesh.
//
void afterMove(Mesh& m, vector<handleType>& handles) {
// TODO
//
}

// This function does a single iteration of the algorithm.
// It must return true if convergence has been achieved,
// and return false if it hasn't been achieved.
// It is called after each movement, and depending on whether
// convergence was reached, is scheduled to call again
// after 1/iterationsPerSecond seconds. If it is already scheduled
// to call when a movement is done, then it is not called,
// although its next call will come with updated handles.
// However in this case, afterMove does get called.
//
bool doIteration(Mesh& m, vector<handleType>& handles) {
// TODO
// Currently, this just moves every vertex directly.
//
	for (vector<handleType>::iterator i = handles.begin(); i != handles.end(); ++i)
		m.getVertex(i->first).setPosition(i->second);
	return true;
}