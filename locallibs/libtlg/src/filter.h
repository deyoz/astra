#ifndef LIBTLG_FILTER_H
#define LIBTLG_FILTER_H

#include <boost/function.hpp>

struct tlgnum_t;
namespace telegrams
{
typedef boost::function<std::string (const tlgnum_t, const std::string&)> tlg_text_filter;
} // namespace telegrams

#endif /* LIBTLG_FILTER_H */

