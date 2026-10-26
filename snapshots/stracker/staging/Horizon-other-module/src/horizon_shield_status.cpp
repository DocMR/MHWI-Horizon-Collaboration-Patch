#include "horizon_shield_status.h"
// Compile/link the real adapter bodies without instantiating a runtime object or
// invoking native code. Other host capacities can instantiate the same header.
template class horizon_other::ShieldStatusAdapter<64>;
