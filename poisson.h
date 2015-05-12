#ifndef POISSON_H
#define POISSON_H

#include <vector>
#include "cvec.h"
#include "mesh.h"

using namespace std;

typedef	pair<int, Cvec2> handleType;

void initPoisson(Mesh&);
void afterMove(Mesh&, vector<handleType>&);
void addPin(handleType&);
void removePin(handleType&);
bool doIteration(Mesh&, vector<handleType>&);

#endif