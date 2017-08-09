#include "dev_consts.h"

namespace ASTRA {

const TDevOperTypes& DevOperTypes() { return ASTRA::singletone<TDevOperTypes>(); }

    const char *TDevFmtTypeS[] = {"ATB", "BTP", "EPL2", "ZPL2", "DPL", "EPSON", "Graphics2D", "FRX", "TEXT", "SCAN1", "SCAN2", "SCAN3", "BCR", ""};
};
