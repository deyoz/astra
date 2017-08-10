#include "dev_consts.h"

namespace ASTRA {

const TDevOperTypes& DevOperTypes() { return ASTRA::singletone<TDevOperTypes>(); }
const TDevFmtTypes& DevFmtTypes() { return ASTRA::singletone<TDevFmtTypes>(); }

};
