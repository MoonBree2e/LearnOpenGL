#include "fluid.h"

using namespace glcs;

void Fluid::update(double time) {
    m_Sort->run(m_ParticlesBuffer, m_SortedParticlesBuffer);
    _dispatchDensityCS();
    _dispatchUpdateCS();
}

void Fluid::_dispatchDensityCS() {

}

void Fluid::_dispatchUpdateCS() {

}