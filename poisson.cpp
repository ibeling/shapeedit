#include "poisson.h"

extern Mesh g_originalMesh;
int g_iterationsPerSecond = 8;

// This function is called at the beginning of the program
// before any handles have been added.
void initPoisson(Mesh& m) {


}

// This function is called almost immediately after each movement.
void afterMove(Mesh& m, vector<handleType>& handles) {
	
}

// This function does a single iteration of the algorithm.
// It must return true if convergence has been achieved,
// and return false if it hasn't been achieved.
bool doIteration(Mesh& m, vector<handleType>& handles) {
	return true;
}