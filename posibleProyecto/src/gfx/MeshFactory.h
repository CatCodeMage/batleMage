#pragma once
#include "gfx/Mesh.h"

namespace gfx {

// Primitivas basicas. Todas estan centradas en el origen y son de tamano unidad,
// asi que se escalan y colocan con la matriz de modelo.
Mesh makeCube();                                      // lado 1
Mesh makeSphere(int stacks = 18, int slices = 28);    // radio 1
Mesh makePlane(float half, float uvRepeat);           // suelo en y=0, de -half a +half

}  // namespace gfx
