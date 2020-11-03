#pragma once

#include <serverlib/rip.h>
#include <serverlib/rip_validators.h>

namespace ct
{

bool typebAddressValidator(const std::string& str);
DECL_RIP2(TypebAddress, std::string, typebAddressValidator);

bool ediAddressValidator(const std::string& str);
DECL_RIP2(EdiAddress, std::string, ediAddressValidator);

} // ct
