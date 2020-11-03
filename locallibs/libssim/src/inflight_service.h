#pragma once

#include <vector>
#include <string>

#include <serverlib/expected.h>
#include <nsi/nsi.h>

namespace ssim {

using InflightServices = std::vector<nsi::InflServiceId>;

Expected<InflightServices> getInflightServices(const std::string&);
std::string toString(const InflightServices&);

bool shouldBeTruncated(const std::set<nsi::InflServiceId>&);

} //ssim
