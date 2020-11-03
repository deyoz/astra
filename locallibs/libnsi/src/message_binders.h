#pragma once

#include <serverlib/message_fwd.h>
#include <libnsi/nsi.h>

namespace message_details {

MSG_BINDER_DECL(nsi::CompanyId)
MSG_BINDER_DECL(nsi::CityId)
MSG_BINDER_DECL(nsi::PointId)
MSG_BINDER_DECL(nsi::AircraftTypeId)
MSG_BINDER_DECL(nsi::SsrTypeId)
MSG_BINDER_DECL(nsi::TermId)
MSG_BINDER_DECL(nsi::CurrencyId)

MSG_BINDER_DECL(nsi::DepArrPoints)
MSG_BINDER_DECL(nsi::DepArrCities)

} //message_details
