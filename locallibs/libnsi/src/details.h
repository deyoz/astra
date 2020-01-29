#ifndef LIBNSI_DETAILS_H
#define LIBNSI_DETAILS_H

#include <ctime>
#include <map>

#include <serverlib/encstring.h>
#include <serverlib/lngv.h>

namespace nsi
{

namespace details {

template<typename IdT>
struct BasicNsiData
{
    typedef IdT id_type;

    explicit BasicNsiData(const IdT& id_) : id(id_), timestamp(time(nullptr))
    {}

    IdT id;
    std::map<Language, EncString> codes;
    std::map<Language, EncString> names;
    time_t timestamp;
};

template<typename DataT>
class BasicNsiObject
{
public:
    typedef typename DataT::id_type id_type;
    typedef DataT data_type;

    explicit BasicNsiObject(const id_type& id);
    explicit BasicNsiObject(const EncString& code);

    virtual ~BasicNsiObject()
    {
    }

    const id_type& id() const
    {
        return data_->id;
    }

    const EncString& lcode() const
    {
        return code(ENGLISH, SCM_STRICT);
    }

    const EncString& code(Language lang, StringChoiceMode m = SCM_NOSTRICT) const
    {
        static const EncString emptyStr;
        const std::map<Language, EncString>::const_iterator it = data_->codes.find(lang);
        if (it != data_->codes.end()) {
            return it->second;
        }
        if (m == SCM_STRICT) {
            return emptyStr;
        } else {
            return code(lang == ENGLISH ? RUSSIAN : ENGLISH, SCM_STRICT);
        }
    }

    const EncString& name(Language lang, StringChoiceMode m = SCM_NOSTRICT) const
    {
        static const EncString emptyStr;
        const std::map<Language, EncString>::const_iterator it = data_->names.find(lang);
        if (it != data_->names.end()) {
            return it->second;
        }
        if (m == SCM_STRICT) {
            return emptyStr;
        } else {
            return name(lang == ENGLISH ? RUSSIAN : ENGLISH, SCM_STRICT);
        }
    }

    const DataT& data() const
    {
        //ASSERT(data_ != NULL);
        return *data_;
    }
private:
    std::shared_ptr<DataT> data_;
};

} // details

} // nsi

#endif /* LIBNSI_DETAILS_H */

