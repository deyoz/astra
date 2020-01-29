#pragma once

#include <serverlib/message_fwd.h>
#include <coretypes/flight.h>
#include <coretypes/rbdorder.h>
#include <coretypes/rfisc.h>

namespace ct {
class SegStatus;
class SsrStatus;
class SvcStatus;
class Seat;
class SsrCode;
} //ct

namespace message_details {

MSG_BINDER_DECL(ct::Flight)
MSG_BINDER_DECL(ct::FlightDate)
MSG_BINDER_DECL(ct::Seat)
MSG_BINDER_DECL(ct::Suffix)
MSG_BINDER_DECL(ct::Rbd)
MSG_BINDER_DECL(ct::Cabin)
MSG_BINDER_DECL(ct::SegStatus)
MSG_BINDER_DECL(ct::SsrStatus)
MSG_BINDER_DECL(ct::SvcStatus)
MSG_BINDER_DECL(ct::RfiscSubCode)
MSG_BINDER_DECL(ct::Rfic)
MSG_BINDER_DECL(ct::SsrCode)

} //message_details
