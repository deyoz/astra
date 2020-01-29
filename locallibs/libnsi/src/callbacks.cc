#include "callbacks.h"

#include <serverlib/helpcpp.h>

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

namespace nsi
{

std::vector<RestrictionData> restrictions()
{
    static const RestrictionData data[] = {
        RestrictionData(1,   EncString::from866("A"), EncString::from866("No local traffic"),
                                                      EncString::from866("���⭠� ��ॢ���� ����饭�")),
        RestrictionData(2,   EncString::from866("B"), EncString::from866("Local traffic only"),
                                                      EncString::from866("���쪮 ���⭠� ��ॢ����")),
        RestrictionData(3,   EncString::from866("C"), EncString::from866("Local and domestic connecting traffic only"),
                                                      EncString::from866("���쪮 ���⭠� ��ॢ���� ��� "
                                                                         "�� ����㭠த��� ��몮��筠� ��ॢ����")),
        RestrictionData(4,   EncString::from866("D"), EncString::from866("Qualified international online connecting "
                                                                         "or stopover traffic only"),
                                                      EncString::from866("����㭠த��� ������ ��몮��� "
                                                                         "��� ��ॢ���� � ��⠭����� (�⮯���஬)")),
        RestrictionData(5,   EncString::from866("E"), EncString::from866("Qualified online connecting or stopover traffic only"),
                                                      EncString::from866("���쪮 ��몮��� ����� ३ᠬ� "
                                                                         "������ ��ॢ��稪� (������ ��몮���) "
                                                                         "��� ��ॢ���� � ��⠭����� (�⮯���஬)")),
        RestrictionData(6,   EncString::from866("F"), EncString::from866("Local and online connecting traffic only"),
                                                      EncString::from866("���쪮 ���⭠� ��ॢ���� � ��몮��� "
                                                                         "����� ३ᠬ� ������ ��ॢ��稪� (������ ��몮���)")),
        RestrictionData(7,   EncString::from866("G"), EncString::from866("Qualified online connecting traffic only"),
                                                      EncString::from866("���쪮 ��몮��� ����� ३ᠬ� "
                                                                         "������ ��ॢ��稪� (������ ��몮���)")),
        RestrictionData(8,   EncString::from866("H"), EncString::from866("Segment not to be displayed"),
                                                      EncString::from866("������� �� ������ �⮡ࠦ�����")),
        RestrictionData(9,   EncString::from866("I"), EncString::from866("Technical landing"),
                                                      EncString::from866("��孨�᪠� ��ᠤ��")),
        RestrictionData(10,  EncString::from866("K"), EncString::from866("Connecting traffic only"),
                                                      EncString::from866("���쪮 ��몮��筠� ��ॢ����")),
        RestrictionData(11,  EncString::from866("M"), EncString::from866("International online stopover traffic only"),
                                                      EncString::from866("���쪮 ����㭠த��� ��ॢ���� � "
                                                                         "��⠭����� (�⮯���஬) �� ३�� "
                                                                         "������ ��ॢ��稪� (������ �⮯����)")),
        RestrictionData(12,  EncString::from866("N"), EncString::from866("International connecting traffic only"),
                                                      EncString::from866("���쪮 ����㭠த��� ��몮��筠� ��ॢ����")),
        RestrictionData(13,  EncString::from866("O"), EncString::from866("International online connecting traffic only"),
                                                      EncString::from866("���쪮 ����㭠த��� ��몮��� ����� "
                                                                         "३ᠬ� ������ ��ॢ��稪� (������ ��몮���)")),
        RestrictionData(14,  EncString::from866("Q"), EncString::from866("International online connecting or stopover traffic only"),
                                                      EncString::from866("���쪮 ����㭠த��� ��몮��� ����� "
                                                                         "३ᠬ� ������ ��ॢ��稪� (������ ��몮���) "
                                                                         "��� ��ॢ���� � ��⠭����� (�⮯���஬)")),
        RestrictionData(15,  EncString::from866("T"), EncString::from866("Online stopover traffic only"),
                                                      EncString::from866("��ॢ���� ⮫쪮 � ��⠭����� (�⮯���஬) "
                                                                         "�� ३�� ������ ��ॢ��稪� (������ �⮯����)")),
        RestrictionData(16,  EncString::from866("V"), EncString::from866("Connecting or stopover traffic only"),
                                                      EncString::from866("���쪮 ��몮��筠� ��ॢ���� ��� "
                                                                         "��ॢ���� � ��⠭����� (�⮯���஬)")),
        RestrictionData(17,  EncString::from866("W"), EncString::from866("International connecting or stopover traffic only"),
                                                      EncString::from866("���쪮 ����㭠த��� ��몮��筠� ��ॢ���� "
                                                                         "��� ����㭠த��� ��ॢ���� � ��⠭����� (�⮯���஬)")),
        RestrictionData(18,  EncString::from866("X"), EncString::from866("Online connecting or stopover traffic only"),
                                                      EncString::from866("���쪮 ��몮��� ����� ३ᠬ� ������ ��ॢ��稪� "
                                                                         "(������ ��몮���) ��� ��ॢ���� � ��⠭����� (�⮯���஬)")),
        RestrictionData(19,  EncString::from866("Y"), EncString::from866("Online connecting traffic only"),
                                                      EncString::from866("���쪮 ��몮��� ����� ३ᠬ� ������ "
                                                                         "��ॢ��稪� (������ ��몮���)")),
        RestrictionData(20,  EncString::from866("Z"), EncString::from866("Multiple traffic restrictions"),
                                                      EncString::from866("������⢥��� ��࠭�祭�� � ��ॢ����"))
    };
    return std::vector<RestrictionData>(data, data + sizeof(data)/sizeof(data[0]));
}
int restrictionsCount()
{
    static int count(restrictions().size());
    return count;
}

std::vector<MealServiceData> mealServices()
{
    static const MealServiceData data[] = {
        MealServiceData( 1, EncString::from866("B"), EncString::from866("Breakfast"),
                                                     EncString::from866("����ࠪ")),

        MealServiceData( 2, EncString::from866("C"), EncString::from866("Alcoholic Beverages - Complimentary"),
                                                     EncString::from866("��ᯫ��� ��������� ����⪨")),

        MealServiceData( 3, EncString::from866("D"), EncString::from866("Dinner"),
                                                     EncString::from866("����")),

        MealServiceData( 4, EncString::from866("F"), EncString::from866("Food for Purchase"),
                                                     EncString::from866("��⠭�� �� �⤥���� �����")),

        MealServiceData( 5, EncString::from866("G"), EncString::from866("Food and Beverages for Purchase"),
                                                     EncString::from866("����⪨ � ��⠭�� �� �⤥���� �����")),

        MealServiceData( 6, EncString::from866("H"), EncString::from866("Hot Meal"),
                                                     EncString::from866("����稥 ����᪨")),

        MealServiceData( 7, EncString::from866("K"), EncString::from866("Continental Breakfast"),
                                                     EncString::from866("���⨭��⠫�� ����ࠪ")),

        MealServiceData( 8, EncString::from866("L"), EncString::from866("Lunch"),
                                                     EncString::from866("����")),

        MealServiceData( 9, EncString::from866("M"), EncString::from866("Meal (general)"),
                                                     EncString::from866("�᭮���� �ନ�, �ᯮ������, �᫨ "
                                                                        "������� ��� ��⠭�� �� ������祭 "
                                                                        "(�.�. �।�ᬮ�७� �����-� ���)")),

        MealServiceData(10, EncString::from866("N"), EncString::from866("No Meal Service"),
                                                     EncString::from866("��⠭�� �� �।��⠢�����")),

        MealServiceData(11, EncString::from866("O"), EncString::from866("Cold Meal"),
                                                     EncString::from866("������� ����᪨")),

        MealServiceData(12, EncString::from866("P"), EncString::from866("Alcoholic Beverages for Purchase"),
                                                     EncString::from866("��������� ����⪨ �� �⤥���� �����")),

        MealServiceData(13, EncString::from866("R"), EncString::from866("Refreshments - Complimentary"),
                                                     EncString::from866("��嫠��⥫�� ����⪨ - ��ᯫ�⭮")),

        MealServiceData(14, EncString::from866("S"), EncString::from866("Snack or Brunch"),
                                                     EncString::from866("������ ����᪨ ��� ������� ����ࠪ")),

        MealServiceData(15, EncString::from866("V"), EncString::from866("Refreshments for Purchase"),
                                                     EncString::from866("��嫠��⥫�� ����⪨ - �� �����")),

        MealServiceData(16, EncString::from866("Y"), EncString::from866("Duty Free sales"),
                                                     EncString::from866("�த��� �� Duty Free"))
    };
    return std::vector<MealServiceData>(data, data + sizeof(data)/sizeof(data[0]));
}
int mealServicesCount()
{
    static int count(mealServices().size());
    return count;
}

std::vector<InflServiceData> inflServices()
{
    static const std::vector<InflServiceData> data = {
        {  1,  EncString::from866("1"), EncString::from866("Movie"), EncString::from866("����") },
        {  2,  EncString::from866("2"), EncString::from866("Telephone"), EncString::from866("����䮭") },
        {  3,  EncString::from866("3"), EncString::from866("Entertainment on demand"),
                                        EncString::from866("��������⥫쭮� ���㦨����� �� ������") },
        {  4,  EncString::from866("4"), EncString::from866("Audio programming"), EncString::from866("�㤨� �ணࠬ��") },
        {  5,  EncString::from866("5"), EncString::from866("Live TV"), EncString::from866("Live TV") },
        {  6,  EncString::from866("6"), EncString::from866("Reservation booking service"),
                                        EncString::from866("��ࢨ� �� �஭�஢����") },
        {  7,  EncString::from866("7"), EncString::from866("Duty Free sales"),
                                        EncString::from866("��ࢨ� �࣮��� (Duty Free)") },
        {  8,  EncString::from866("8"), EncString::from866("Smoking"), EncString::from866("����騩 ३�") },
        {  9,  EncString::from866("9"), EncString::from866("Non-smoking"), EncString::from866("������騩 ३�") },
        { 10, EncString::from866("10"), EncString::from866("Short feature video"), EncString::from866("���⪮���ࠦ��� �����") },
        { 11, EncString::from866("11"), EncString::from866("No Duty Free sales"),
                                        EncString::from866("��������� �ࢨ� �࣮��� (Duty Free)") },
        { 12, EncString::from866("12"), EncString::from866("In-seat power source"),
                                        EncString::from866("���஥��� � ��᫮ ���筨� ��⠭��") },
        { 13, EncString::from866("13"), EncString::from866("Internet access"), EncString::from866("����� � ���୥�") },

        { 14, EncString::from866("14"), EncString::from866("Unassigned"), EncString::from866("��ࢨ� �� ��।����") },

        { 15, EncString::from866("15"), EncString::from866("In-seat Video Player/Library"),
                                        EncString::from866("���஥��� � ��᫮ ���������� � �����-������⥪��") },
        { 16, EncString::from866("16"), EncString::from866("Lie-flat seat"),
                                        EncString::from866("��᪫��뢠�饥�� ��᫮") },
        { 17, EncString::from866("17"), EncString::from866("Additional Services"), EncString::from866("�������⥫�� ��㣨") },
        { 18, EncString::from866("18"), EncString::from866("Wi-Fi"), EncString::from866("Wi-Fi") },

        { 19, EncString::from866("19"), EncString::from866("Lie-flat Seat First"),
                                        EncString::from866("��᪫��뢠�饥�� ��᫮ � ��ࢮ� �����") },
        { 20, EncString::from866("20"), EncString::from866("Lie-flat Seat Business"),
                                        EncString::from866("��᪫��뢠�饥�� ��᫮ � ������ �����") },
        { 21, EncString::from866("21"), EncString::from866("Lie-flat Seat Premium Economy"),
                                        EncString::from866("��᪫��뢠�饥�� ��᫮ � ����� ������-�६��") },

        { 22, EncString::from866("22"), EncString::from866("110V AC Power"),
                                        EncString::from866("������஧�⪠ 110V") },
        { 23, EncString::from866("23"), EncString::from866("110V AC Power First"),
                                        EncString::from866("������஧�⪠ 110V � ��ࢮ� �����") },
        { 24, EncString::from866("24"), EncString::from866("110V AC Power Business"),
                                        EncString::from866("������஧�⪠ 110V � ����� ������ �����") },
        { 25, EncString::from866("25"), EncString::from866("110V AC Power Premium Economy"),
                                        EncString::from866("������஧�⪠ 110V � ����� ������-�६��") },
        { 26, EncString::from866("26"), EncString::from866("110V AC Power Economy"),
                                        EncString::from866("������஧�⪠ 110V � ������ �����") },

        { 27, EncString::from866("27"), EncString::from866("USB Power"),
                                        EncString::from866("USB ࠧꥬ") },
        { 28, EncString::from866("28"), EncString::from866("USB Power First"),
                                        EncString::from866("USB ࠧꥬ � ��ࢮ� �����") },
        { 29, EncString::from866("29"), EncString::from866("USB Power Business"),
                                        EncString::from866("USB ࠧꥬ � ������ �����") },
        { 30, EncString::from866("30"), EncString::from866("USB Power Premium Economy"),
                                        EncString::from866("USB ࠧꥬ � ����� ������-�६��") },
        { 31, EncString::from866("31"), EncString::from866("USB Power Economy"),
                                        EncString::from866("USB ࠧꥬ � ������ �����") },

        { 32, EncString::from866("99"), EncString::from866("Amenities subject to change"),
                                        EncString::from866("������� �।���� 㤮��⢠ � ������") }
    };
    return data;
}
int inflServicesCount()
{
    static int count(inflServices().size());
    return count;
}

std::vector<ServiceTypeData> serviceTypes()
{
    static const ServiceTypeData data[] = {
        ServiceTypeData( 1, EncString::from866("A"), ADDITIONAL,
                                                     EncString::from866("Cargo / Mail"),
                                                     EncString::from866("��㧮��� ��� ���⮢�")),
        ServiceTypeData( 2, EncString::from866("B"), ADDITIONAL,
                                                     EncString::from866("Passenger, Shuttle mode"),
                                                     EncString::from866("���ᠦ��᪨� 祫����")),
        ServiceTypeData( 3, EncString::from866("C"), CHARTER,
                                                     EncString::from866("Passenger Only"),
                                                     EncString::from866("������ ���ᠦ��᪨�")),
        ServiceTypeData( 4, EncString::from866("D"), OTHER,
                                                     EncString::from866("General aviation"),
                                                     EncString::from866("���筠� ������")),
        ServiceTypeData( 5, EncString::from866("E"), OTHER,
                                                     EncString::from866("Special (FAA/Government)"),
                                                     EncString::from866("���樠��� �ࠢ�⥫��⢥���")),
        ServiceTypeData( 6, EncString::from866("F"), REGULAR,
                                                     EncString::from866("Loose loaded cargo and/or preloaded devices"),
                                                     EncString::from866("��㧮��� / ���⮢�, ������㦥��� ���� ��࠭�� ����㦥��� ����")),
        ServiceTypeData( 7, EncString::from866("G"), ADDITIONAL,
                                                     EncString::from866("Passenger, normal service"),
                                                     EncString::from866("���ᠦ��᪨�")),
        ServiceTypeData( 8, EncString::from866("H"), CHARTER,
                                                     EncString::from866("Cargo and/or mail"),
                                                     EncString::from866("��㧮��� �/��� ���⮢�")),
        ServiceTypeData( 9, EncString::from866("I"), OTHER,
                                                     EncString::from866("State / Diplomatic / Air ambulance"),
                                                     EncString::from866("���㤠��⢥��� / ���������᪨� / ����樭᪨�")),
        ServiceTypeData(10, EncString::from866("J"), REGULAR,
                                                     EncString::from866("Passenger, Normal Service"),
                                                     EncString::from866("���ᠦ��᪨�, ॣ����")),
        ServiceTypeData(11, EncString::from866("K"), OTHER,
                                                     EncString::from866("Training (school / crew check)"),
                                                     EncString::from866("���祭�� ���ᠭ⮢ / �஢�ઠ �����")),
        ServiceTypeData(12, EncString::from866("L"), CHARTER,
                                                     EncString::from866("Passenger and Cargo and/or mail"),
                                                     EncString::from866("������ ���ᠦ��᪨� � ��㧮� �/��� ���⮩")),
        ServiceTypeData(13, EncString::from866("M"), REGULAR,
                                                     EncString::from866("Mail only"),
                                                     EncString::from866("���⮢�")),
        ServiceTypeData(14, EncString::from866("N"), OTHER,
                                                     EncString::from866("Business aviation / Air taxi"),
                                                     EncString::from866("������� ������ / ���⠪�")),
        ServiceTypeData(15, EncString::from866("O"), CHARTER,
                                                     EncString::from866("Charter requiring special handling"),
                                                     EncString::from866("������, �ॡ��騩 ᯥ樠�쭮�� ���㦨�����")),
        ServiceTypeData(16, EncString::from866("P"), OTHER,
                                                     EncString::from866("Non-revenue (Positioning/Ferry/Delivery/Demo)"),
                                                     EncString::from866("���������᪨� (����樮��஢����/��ॣ��/���⠢��/����������)")),
        ServiceTypeData(17, EncString::from866("Q"), REGULAR,
                                                     EncString::from866("Passenger / Cargo in cabin"),
                                                     EncString::from866("���ᠦ��᪨� � ��㧮� � ᠫ���")),
        ServiceTypeData(18, EncString::from866("R"), ADDITIONAL,
                                                     EncString::from866("Passenger / Cargo in �abin"),
                                                     EncString::from866("���ᠦ��᪨� � ��㧮� � ᠫ���")),
        ServiceTypeData(19, EncString::from866("S"), REGULAR,
                                                     EncString::from866("Passenger, Shuttle mode"),
                                                     EncString::from866("���ᠦ��᪨� 祫����")),
        ServiceTypeData(20, EncString::from866("T"), OTHER,
                                                     EncString::from866("Technical test"),
                                                     EncString::from866("��孨�᪠� �஢�ઠ")),
        ServiceTypeData(21, EncString::from866("U"), REGULAR,
                                                     EncString::from866("Passenger, operated by Surface vehicle"),
                                                     EncString::from866("���ᠦ��᪨�, ������� �࠭ᯮ�⮬")),
        ServiceTypeData(22, EncString::from866("V"), REGULAR,
                                                     EncString::from866("Cargo / mail, operated by Surface vehicle"),
                                                     EncString::from866("��㧮��� / ���⮢�, ������� �࠭ᯮ�⮬")),
        ServiceTypeData(23, EncString::from866("W"), OTHER,
                                                     EncString::from866("Military"),
                                                     EncString::from866("������")),
        ServiceTypeData(24, EncString::from866("X"), OTHER,
                                                     EncString::from866("Technical stop"),
                                                     EncString::from866("��孨�᪠� ��⠭����"))
    };
    return std::vector<ServiceTypeData>(data, data + sizeof(data)/sizeof(data[0]));
}
int serviceTypesCount()
{
    static int count(serviceTypes().size());
    return count;
}

#define NSI_CB_DEF(CallbacksImpl, NsiTypeId, NsiTypeData, valuesFnc) \
boost::optional<NsiTypeId> CallbacksImpl::MakeName2(find, NsiTypeId)(const EncString& code) \
{ \
    static const std::vector<NsiTypeData> data(valuesFnc()); \
    for(const NsiTypeData& d : data) { \
        const std::map<Language, EncString>::const_iterator it_en(d.codes.find(ENGLISH)); \
        if (it_en != d.codes.end() && it_en->second == code) \
            return d.id; \
    } \
    return boost::optional<NsiTypeId>(); \
} \
boost::optional<NsiTypeData> CallbacksImpl::MakeName2(find, NsiTypeData)(const NsiTypeId& id) \
{ \
    static const std::vector<NsiTypeData> data(valuesFnc()); \
    if (id.get() > static_cast<int>(data.size())) \
        return boost::optional<NsiTypeData>(); \
    return data[id.get() - 1]; \
}

NSI_CB_DEF(Callbacks, RestrictionId, RestrictionData, restrictions);
NSI_CB_DEF(Callbacks, MealServiceId, MealServiceData, mealServices);
NSI_CB_DEF(Callbacks, InflServiceId, InflServiceData, inflServices);
NSI_CB_DEF(Callbacks, ServiceTypeId, ServiceTypeData, serviceTypes);

size_t Callbacks::getCompanyMaxVersion() const { return 0; }
size_t Callbacks::getDocTypeMaxVersion() const { return 0; }
size_t Callbacks::getSsrTypeMaxVersion() const { return 0; }
size_t Callbacks::getGeozoneMaxVersion() const { return 0; }
size_t Callbacks::getCountryMaxVersion() const { return 0; }
size_t Callbacks::getRegionMaxVersion() const { return 0; }
size_t Callbacks::getCityMaxVersion() const { return 0; }
size_t Callbacks::getPointMaxVersion() const { return 0; }
size_t Callbacks::getAircraftTypeMaxVersion() const { return 0; }
size_t Callbacks::getRouterMaxVersion() const { return 0; }
size_t Callbacks::getCurrencyMaxVersion() const { return 0; }

size_t Callbacks::getRestrictionMaxVersion() const { return 0; }
size_t Callbacks::getMealServiceMaxVersion() const { return 0; }
size_t Callbacks::getInflServiceMaxVersion() const { return 0; }
size_t Callbacks::getServiceTypeMaxVersion() const { return 0; }

nsi::CompanyId Callbacks::replaceObsoleteId(const nsi::CompanyId& id) { return id; }
nsi::DocTypeId Callbacks::replaceObsoleteId(const nsi::DocTypeId& id) { return id; }
nsi::SsrTypeId Callbacks::replaceObsoleteId(const nsi::SsrTypeId& id) { return id; }
nsi::GeozoneId Callbacks::replaceObsoleteId(const nsi::GeozoneId& id) { return id; }
nsi::CountryId Callbacks::replaceObsoleteId(const nsi::CountryId& id) { return id; }
nsi::RegionId Callbacks::replaceObsoleteId(const nsi::RegionId& id) { return id; }
nsi::CityId Callbacks::replaceObsoleteId(const nsi::CityId& id) { return id; }
nsi::PointId Callbacks::replaceObsoleteId(const nsi::PointId& id) { return id; }
nsi::AircraftTypeId Callbacks::replaceObsoleteId(const nsi::AircraftTypeId& id) { return id; }
nsi::RouterId Callbacks::replaceObsoleteId(const nsi::RouterId& id) { return id; }
nsi::CurrencyId Callbacks::replaceObsoleteId(const nsi::CurrencyId& id) { return id; }

nsi::RestrictionId Callbacks::replaceObsoleteId(const RestrictionId& id) { return id; }
nsi::MealServiceId Callbacks::replaceObsoleteId(const MealServiceId& id) { return id; }
nsi::InflServiceId Callbacks::replaceObsoleteId(const InflServiceId& id) { return id; }
nsi::ServiceTypeId Callbacks::replaceObsoleteId(const ServiceTypeId& id) { return id; }

#ifdef XP_TESTING

class TestCallbacks : public Callbacks
{
public:
    virtual bool needCheckVersion() const;

    virtual nsi::CityId centerCity() const;

    virtual boost::optional<CompanyId> findCompanyId(const EncString&);
    virtual boost::optional<CompanyId> findCompanyIdByAccountCode(const EncString&);
    virtual boost::optional<CompanyData> findCompanyData(const CompanyId&);

    virtual boost::optional<DocTypeId> findDocTypeId(const EncString&);
    virtual boost::optional<DocTypeData> findDocTypeData(const DocTypeId&);

    virtual boost::optional<SsrTypeId> findSsrTypeId(const EncString&);
    virtual boost::optional<SsrTypeData> findSsrTypeData(const SsrTypeId&);

    virtual boost::optional<GeozoneId> findGeozoneId(const EncString&);
    virtual boost::optional<GeozoneData> findGeozoneData(const GeozoneId&);

    virtual std::set<GeozoneId> getGeozones(const CountryId&);
    virtual std::set<GeozoneId> getGeozones(const RegionId&);
    virtual std::set<GeozoneId> getGeozones(const CityId&);

    virtual boost::optional<CountryId> findCountryId(const EncString&);
    virtual boost::optional<CountryId> findCountryIdByIso(const EncString&);
    virtual boost::optional<CountryData> findCountryData(const CountryId&);

    virtual boost::optional<RegionId> findRegionId(const EncString&);
    virtual boost::optional<RegionData> findRegionData(const RegionId&);

    virtual boost::optional<CityId> findCityId(const EncString&);
    virtual boost::optional<CityData> findCityData(const CityId&);

    virtual boost::optional<PointId> findPointId(const EncString&);
    virtual boost::optional<PointData> findPointData(const PointId&);

    virtual boost::optional<AircraftTypeId> findAircraftTypeId(const EncString&);
    virtual boost::optional<AircraftTypeData> findAircraftTypeData(const AircraftTypeId&);

    virtual boost::optional<RouterId> findRouterId(const EncString&);
    virtual boost::optional<RouterData> findRouterData(const nsi::RouterId&);

    virtual boost::optional<CurrencyId> findCurrencyId(const EncString&);
    virtual boost::optional<CurrencyData> findCurrencyData(const nsi::CurrencyId&);
};


void setupTestNsi()
{
    setCallbacks(new TestCallbacks);
}

bool TestCallbacks::needCheckVersion() const { return false; }

nsi::CityId TestCallbacks::centerCity() const
{
    return nsi::CityId(255);
}

template<typename IdType, typename ItemType, size_t sz>
boost::optional<IdType> findIdByCode(const EncString& code, const ItemType(&items)[sz])
{
    const std::string s = code.to866();
    for (size_t i = 0; i < sz; ++i) {
        if (strcmp(s.c_str(), items[i].latCode) == 0
                || strcmp(s.c_str(), items[i].rusCode) == 0) {
            return IdType(items[i].id);
        }
    }
    return boost::none;
}

struct TestCompanyData
{
    int id;
    const char* latCode;
    const char* rusCode;
    const char* latName;
    const char* rusName;
    const char* accountCode;
} airlinesForTest[] {
    { 119, "UT", "��", "UTAIR AVIATION JSC", "��� ������������ �����", "298" },
    { 122, "U6", "�6", "URAL AIRLINES", "��������� ��", "262" },
    { 8041, "LH", "LH", "LUFTHANSA GERMAN AIRLINES", "��������� ���������", "220" },
    { 500, "YY", "��", "", "����� ������������", "��" },
    { 6, "9U", "9�", "AIR MOLDOVA", "��� �������", "572"},
    { 7624, "OK", "OK", "CZECH AIRLINES", "CZECH AIRLINES", "064"},
    { 10112, "1H", "1H", "GLOBAL DISTRIBUTION SYSTEM", "���������� �������������� ����", "1H"},
    { 7701, "AY", "AY", "FINNAIR", "FINNAIR", "999"},
    { 111, "SU", "��", "AEROFLOT RUSSIAN AIRLINES", "��������", "555"},
    { 31, "NN", "��", "VIM AIRLINES", "VIM AIRLINES", "823"},
    { 13609, "XX", "XX", "TEST AIRLINE", "�������� ��������", "01C"},
    { 9493, "VY", "VY", "VUELING AIRLINES", "VUELING AIRLINES", "995"},
};

boost::optional<CompanyId> TestCallbacks::findCompanyId(const EncString& company)
{
    return findIdByCode<CompanyId>(company, airlinesForTest);
}

boost::optional<CompanyId> TestCallbacks::findCompanyIdByAccountCode(const EncString& accountCode)
{
    static std::map<std::string, boost::optional<CompanyId> > accountCodeMap;
    if (accountCodeMap.empty()) {
        accountCodeMap["262"] = CompanyId(122);
        accountCodeMap["298"] = CompanyId(119);
        accountCodeMap["220"] = CompanyId(8041);
    }
    return accountCodeMap[accountCode.to866()];
}

boost::optional<CompanyData> TestCallbacks::findCompanyData(const CompanyId& id)
{
    for (const auto& a : airlinesForTest) {
        if (id.get() != a.id) {
            continue;
        }
        CompanyData cd(id);
        cd.codes[ENGLISH] = EncString::from866(a.latCode);
        cd.codes[RUSSIAN] = EncString::from866(a.rusCode);
        cd.names[ENGLISH] = EncString::from866(a.latName);
        cd.names[RUSSIAN] = EncString::from866(a.rusName);
        cd.accountCode = a.accountCode;
        return cd;
    }
    return boost::none;
}

struct TestDocTypeData
{
    int id;
    const char* latCode;
    const char* rusCode;
    const char* latName;
    const char* rusName;
    bool requireCitizenship;
} docTypesForTest[] {
    { 1, "PS", "��", "PASSPORT", "�������", false },
    { 2, "PSP", "���", "INTERNATIONAL PASSPORT", "����������� �������", false },
    { 4, "UDL", "���", "OFFICER IDENTIFICATION", "������������� �������� �������", false },
    { 7, "SR", "��", "BIRTH REGISTRATION DOCUMENT", "������������� � ��������", false },
    { 320, "VV", "��", "DOCUMENT OF PASSPORT LOSING", "��� �� ����������", true }
};

boost::optional<DocTypeId> TestCallbacks::findDocTypeId(const EncString& s)
{
    return findIdByCode<DocTypeId>(s, docTypesForTest);
}

boost::optional<DocTypeData> TestCallbacks::findDocTypeData(const DocTypeId& id)
{
    for (const auto& d: docTypesForTest) {
        if (id.get() != d.id) {
            continue;
        }
        DocTypeData dtd(id);
        dtd.codes[ENGLISH] = EncString::from866(d.latCode);
        dtd.codes[RUSSIAN] = EncString::from866(d.rusCode);
        dtd.names[ENGLISH] = EncString::from866(d.latName);
        dtd.names[RUSSIAN] = EncString::from866(d.rusName);
        dtd.requireCitizenship = d.requireCitizenship;
        return dtd;
    }
    return boost::none;
}

struct TestSsrTypeData
{
    int id;
    const char* latCode;
    const char* rusCode;
    const char* latText;
    const char* rusText;
    bool needConfirm;
    nsi::SsrRequiredType actionCode;
    nsi::SsrRequiredType freeText;
    nsi::SsrRequiredType freeTextAnswer;
    nsi::SsrRequiredType pass;
    nsi::SsrRequiredType seg;
} const ssrTypesForTest[] {
    {  3, "CBBG", "����", "CABIN BAGGAGE", "����� � ������ (��� ������ ���� ������� �����)", true, RT_REQUIRED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {  5, "EXST", "����", "EXTRA SEAT", "�������������� �����", true, RT_REQUIRED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {  8, "LANG", "����", "FOREING LANGUAGE ONLY", "�������� �� ����� �������� �����", false, RT_REQUIRED, RT_REQUIRED, RT_ALLOWED, RT_ALLOWED, RT_REQUIRED},
    {  9, "VGML", "����", "VEGETARIAN MEAL", "�������������� �������", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 13, "RQST", "����", "SEAT REQUEST", "", true, RT_REQUIRED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED},
    { 14, "SEAT", "����", "PRERESERVED SEAT AND BOARDING PASS NOTIFICATION", "", true, RT_REQUIRED, RT_REQUIRED, RT_REQUIRED, RT_REQUIRED, RT_REQUIRED},
    { 17, "AVML", "����", "VEGETARIAN HINDU MEAL", "�������������� ��������� ����", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 18, "BBML", "����", "INFANT/BABY FOOD", "������� ��� �������� �������", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 20, "BLML", "����", "BLAND MEAL", "��������� ����", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 27, "FPML", "����", "FRUIT PLATTER MEAL", "������", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 28, "FQTV", "����", "FREQUENT TRAVELLER", "", false, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED},
    { 31, "GFML", "����", "GLUTEN INTOLERANT MEAL", "������������ ����", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 33, "HNML", "����", "HINDU MEAL", "��������� ����", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 34, "KSML", "����", "KOSHER MEAL", "�������� ����", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 35, "LCML", "����", "LOW CALORIE MEAL", "��������������� ����", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 37, "LSML", "����", "LOW SALT MEAL", "��������� ����", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 40, "MOML", "����", "MOSLEM MEAL", "������������� ����", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 41, "NSSA", "����", "NO SMOKING AISLE SEAT", "", true, RT_DENIED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED},
    { 42, "NSSB", "����", "NO SMOKING BULKHEAD SEAT", "", true, RT_DENIED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED},
    { 43, "NSST", "����", "NO SMOKING SEAT", "", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED},
    { 44, "NSSW", "����", "NO SMOKING WINDOW SEAT", "", true, RT_DENIED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED},
    { 46, "PCTC", "����", "EMERGENCY CONTACT FOR PASSENGER", "������� ������� � ����������", false, RT_ALLOWED, RT_REQUIRED, RT_DENIED, RT_ALLOWED, RT_DENIED},
    { 49, "PSPT", "����", "PASSPORT NUMBER", "����� ��������", false, RT_DENIED, RT_REQUIRED, RT_ALLOWED, RT_ALLOWED, RT_DENIED},
    { 50, "RVML", "����", "VEGETARIAN RAW MEAL", "����� �������������� ����", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 54, "SMSA", "����", "SMOKING AISLE SEAT", "", true, RT_DENIED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED},
    { 55, "SMSB", "����", "SMOKING BULKHEAD SEAT", "", true, RT_DENIED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED},
    { 56, "SMST", "����", "SMOKING SEAT", "", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED},
    { 57, "SMSW", "����", "SMOKING WINDOW SEAT", "", true, RT_DENIED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED},
    { 64, "TKNE", "����", "ELECTRONIC TICKET NUMBER", "����� ������������ ������", false, RT_REQUIRED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 76, "INFT", "����", "INFANT", "", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    { 77, "DOCS", "����", "PASSPORT NUMBER", "", false, RT_DENIED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_DENIED},
    { 79, "FQTR", "����", "FREQUENT TRAVELLER AWARD REDEMPTION JOURNEY", "", false, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED},
    { 80, "FQTU", "����", "FREQUENT FLYER UPGRADE AWARD REDEMPTION", "", false, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED},
    { 81, "FQTS", "����", "FREQUENT TRAVELLER SERVICE BENEFIT INFORMATION", "", false, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED},
    {350, "DOCO", "����", "OTHER TRAVELER RELATED INFORMATION", "", false, RT_DENIED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_DENIED},
    {351, "DOCA", "����", "ADDRESS INFORMATION", "", false, RT_DENIED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_DENIED},
    {352, "NRSB", "����", "NON REVENUE STANDBY", "", false, RT_ALLOWED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_DENIED},
    {402, "SMSR", "����", "SMOKING REAR-FACING SEAT", "", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED},
    {403, "NSSR", "����", "NO SMOKING REAR-FACING SEAT", "", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED},
    {421, "FOID", "", "FORM OF IDENTIFICATION", "", false, RT_REQUIRED, RT_REQUIRED, RT_ALLOWED, RT_ALLOWED, RT_DENIED},
    {453, "CTCE", "", "PASSENGER CONTACT INFORMATION E-MAIL ADDRESS", "", false, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_ALLOWED, RT_DENIED},
    {454, "CTCM", "", "PASSENGER CONTACT INFORMATION MOBILE PHONE NUMBER", "", false, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_ALLOWED, RT_DENIED},
    {404, "VVIP", "����", "PASSENGER ORDERED VIP SERVICE AT PORT", "", false, RT_ALLOWED, RT_ALLOWED, RT_DENIED, RT_ALLOWED, RT_ALLOWED },
    {23, "CKIN", "����", "PROVIDES INFORMATION FOR AIRPORT PERSONNEL", "���������� ��� ��������� ���������", false, RT_REQUIRED, RT_ALLOWED, RT_ALLOWED, RT_DENIED, RT_DENIED },
    {32, "GPST", "����", "GROUP SEAT REQUEST", "������ ���� ��� ������", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_DENIED, RT_REQUIRED },
    {2, "BLND", "����", "BLIND PASSENGER", "������ ��������", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {4, "DEAF", "����", "DEAF PASSENGER", "������ ��������", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {39, "MEDA", "����", "COMPANY MEDICAL CLEARANCE MAY BE REQUIRED", "����������� ��������", true, RT_REQUIRED, RT_ALLOWED, RT_ALLOWED, RT_ALLOWED, RT_DENIED },
    {60, "STCR", "����", "STRETCHER PASSENGER", "������� �������� �� ��������", true, RT_DENIED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {69, "WCBD", "����", "WHEELCHAIR", "���������� ������ �� ����������", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {70, "WCHC", "����", "WHEELCHAIR FOR CABIN SEAT. PASSENGER COMPLETELY IMMOBILE", "������� ������. �� ����� �������������� �������������", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {71, "WCHR", "����", "WHEELCHAIR- FOR RAMP", "��������� ������ ��� ������� � �������", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {72, "WCHS", "����", "WHEELCHAIR -FOR STEPS", "������� �������� �� ����� ��������� �� ����������", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {73, "WCMP", "����", "WHEELCHAIR-MANUAL POWER TO BE TRANSP. BY A PASSENGER", "�����. ������ ���. ���. ������� ����� ���� �������������", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {68, "UMNR", "����", "UNACCOMPANIED MINOR", "������� ��� �������������", true, RT_REQUIRED, RT_REQUIRED, RT_ALLOWED, RT_ALLOWED, RT_REQUIRED },
    {12, "OTHS", "����", "OTHER", "������", false, RT_REQUIRED, RT_ALLOWED, RT_ALLOWED, RT_ALLOWED, RT_ALLOWED},
    {78, "CHLD", "����", "CHILD", "������� �������", true, RT_DENIED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED },
    {6,  "GRPS", "����", "COMMON IDENTITY ASSIGNED BY THE BOOKING MEMBER", "��� ������", true, RT_ALLOWED, RT_ALLOWED, RT_ALLOWED, RT_DENIED, RT_DENIED },
    {66, "TKTL", "����", "TICKETING TIME LIMIT", "���������� ���� ������������ ������", false, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_DENIED },
    {62, "TKNA", "����", "TICKET NUMBERS FOR AUTOMATICALLY", "����� ������, �������� ������������� ��������", false, RT_REQUIRED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED },
    {401, "ASVC", "ASVC", "ADDITIONAL SERVICE", "", true, RT_REQUIRED, RT_REQUIRED, RT_REQUIRED, RT_REQUIRED, RT_REQUIRED },
    {10, "XBAG", "����", "EXCESS BAGGAGE", "���������������� �����", true, RT_REQUIRED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {16, "AVIH", "����", "ANIMAL IN HOLD", "������� � ����. �������� ������", true, RT_REQUIRED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {19, "BIKE", "����", "BICYCLE", "���������", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {22, "CHML", "����", "CHILD MEAL", "������� �������", true, RT_ALLOWED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED },
    {25, "DBML", "����", "DIABETIC MEAL", "������������� ����", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
    {47, "PETC", "����", "ANIMAL IN CABIN", "�������� � ������", true, RT_REQUIRED, RT_REQUIRED, RT_ALLOWED, RT_REQUIRED, RT_REQUIRED},
    {52, "SFML", "����", "SEAFOOD MEAL", "������������", true, RT_DENIED, RT_REQUIRED, RT_DENIED, RT_REQUIRED, RT_REQUIRED},
};

boost::optional<SsrTypeId> TestCallbacks::findSsrTypeId(const EncString& s)
{
    return findIdByCode<SsrTypeId>(s, ssrTypesForTest);
}

boost::optional<SsrTypeData> TestCallbacks::findSsrTypeData(const SsrTypeId& id)
{
    for (const auto& ssr: ssrTypesForTest) {
        if (id.get() != ssr.id) {
            continue;
        }
        SsrTypeData ssrTypeData(id);
        ssrTypeData.codes[ENGLISH] = EncString::from866(ssr.latCode);
        ssrTypeData.codes[RUSSIAN] = EncString::from866(ssr.rusCode);
        ssrTypeData.names[ENGLISH] = EncString::from866(ssr.latText);
        ssrTypeData.names[RUSSIAN] = EncString::from866(ssr.rusText);
        ssrTypeData.needConfirm = ssr.needConfirm;
        ssrTypeData.actionCode = ssr.actionCode;
        ssrTypeData.freeText = ssr.freeText;
        ssrTypeData.freeTextAnswer = ssr.freeTextAnswer;
        ssrTypeData.pass = ssr.pass;
        ssrTypeData.seg = ssr.seg;
        return ssrTypeData;
    }
    return boost::none;
}

struct TestGeozoneData
{
    int id;
    const char* latCode;
    const char* rusCode;
    const char* latName;
    const char* rusName;
} geozonesForTest[] = {
    {2, "SAF", "SAF", "SOUTHERN AFRICA", "SOUTHERN AFRICA"},
    {10, "SCH", "SCH", "SCHENGEN AGREEMENT", "SCHENGEN AGREEMENT"},
    {11, "EUR", "EUR", "EUROPE", "EUROPE"},
    {12, "SEA", "SEA", "SOUTH EAST ASIA", "SOUTH EAST ASIA"},
    {13, "NOA", "NOA", "NORTH AMERICA", "NORTH AMERICA"},
};

boost::optional<GeozoneId> TestCallbacks::findGeozoneId(const EncString& s)
{
    return findIdByCode<GeozoneId>(s, geozonesForTest);
}

boost::optional<GeozoneData> TestCallbacks::findGeozoneData(const GeozoneId& v)
{
    for (const auto& gz: geozonesForTest) {
        if (gz.id != v.get()) {
            continue;
        }
        GeozoneData gzd(v);
        gzd.codes[ENGLISH] = EncString::from866(gz.latCode);
        gzd.codes[RUSSIAN] = EncString::from866(gz.rusCode);
        gzd.names[ENGLISH] = EncString::from866(gz.latName);
        gzd.names[RUSSIAN] = EncString::from866(gz.rusName);
        return gzd;
    }
    return boost::none;
}

struct TestCountryData
{
    int id;
    int currency_id;
    const char* latCode;
    const char* rusCode;
    const char* latName;
    const char* rusName;
    const char* isoCode;
    std::vector<int> zones;

} countriesForTest[] = {
    { 1,   1, "RU", "��", "RUSSIAN FEDERATION", "���������� ���������", "RUS", { 11, 12 }},
    { 762, 1, "TJ", "��", "TAJIKISTAN", "�����������", "TJK", { 12 } },
    { 804, 1, "UA", "��", "UKRAINE", "�������", "UKR", { 11 } },
    { 2,   1, "DE", "��", "GERMANY REPUBLIC", "��������", "DEU", { 10, 11 } },
    { 3,   1, "FR", "��", "FRANCE", "�������", "FRA", { 10, 11 } },
    { 4,   1, "RO", "��", "ROMANIA", "�������", "ROU", { 11 } },
    { 5,   1, "US", "��", "UNITED STATES OF AMERICA", "����������� ����� �������", "USA", { 13 } },
};


boost::optional<CountryId> TestCallbacks::findCountryId(const EncString& s)
{
    return findIdByCode<CountryId>(s, countriesForTest);
}

boost::optional<CountryId> TestCallbacks::findCountryIdByIso(const EncString& iso)
{
    for (const auto& v : countriesForTest) {
        if (iso.to866() == v.isoCode) {
            return CountryId(v.id);
        }
    }
    return boost::none;
}

boost::optional<CountryData> TestCallbacks::findCountryData(const CountryId& id)
{
    for (const auto& v : countriesForTest) {
        if (id.get() != v.id) {
            continue;
        }
        CountryData cd(id, CurrencyId(v.currency_id));
        cd.codes[ENGLISH] = EncString::from866(v.latCode);
        cd.codes[RUSSIAN] = EncString::from866(v.rusCode);
        cd.names[ENGLISH] = EncString::from866(v.latName);
        cd.names[RUSSIAN] = EncString::from866(v.rusName);
        cd.isoCode = v.isoCode;
        return cd;
    }
    return boost::none;
}

boost::optional<RegionId> TestCallbacks::findRegionId(const EncString&)
{
    boost::optional<RegionId> regionId;
    return regionId;
}

boost::optional<RegionData> TestCallbacks::findRegionData(const RegionId&)
{
    boost::optional<RegionData> regionData;
    return regionData;
}

struct TestCityData
{
    int id;
    int country_id;
    const char* latCode;
    const char* rusCode;
    const char* latName;
    const char* rusName;
    int latit, longit;
    const char* tz;
    std::vector<int> zones;
} citiesForTest[] = {
    {26,  1,   "MOW", "���", "MOSCOW", "������", 3345, 2257, "EUROPE/MOSCOW", { 11 } },
    {27,  1,   "VOZ", "���", "VORONEZH", "�������", 3100, 2352, "EUROPE/MOSCOW", { 11 } },
    {28,  1,   "LED", "���", "ST PETERSBURG", "�����-���������", 3596, 1819, "EUROPE/MOSCOW", { 11 } },
    {29,  804, "IEV", "���", "KIEV", "����", 3027, 1830, "EUROPE/KIEV", { 11 } },
    {35,  1,   "OMS", "���", "OMSK", "����", 3298, 4404, "ASIA/OMSK", { 12 } },
    {42,  1,   "UFA", "���", "UFA", "���", 3284, 3360, "ASIA/YEKATERINBURG", { 11 } },
    {56,  1,   "ARH", "���", "ARKHANGELSK", "�����������", 3872, 2434, "EUROPE/MOSCOW", { 11 } },
    {95,  1,   "SVX", "���", "EKATERINBURG", "������������", 3410, 3636, "ASIA/YEKATERINBURG", { 12 } },
    {119, 1,   "KGD", "���", "KALININGRAD", "�����������", 3283, 1231, "EUROPE/KALININGRAD", { 11 } },
    {131, 1,   "KJA", "���", "KRASNOYARSK", "����������", 3360, 5573, "ASIA/KRASNOYARSK", { 12 }},
    {144, 1,   "MMK", "���", "MURMANSK", "��������", 4137, 1986, "EUROPE/MOSCOW", { 11 } },
    {148, 1,   "GOJ", "���", "NIZHNIY NOVGOROD", "������ ��������", 3380, 2650, "EUROPE/MOSCOW", { 11 }},
    {222, 1,   "CSY", "���", "CHEBOKSARY", "���������", 3368, 2835, "EUROPE/MOSCOW", { 11 }},
    {236, 1,   "ROV", "���", "ROSTOV", "������-��-����", 2834, 2383, "EUROPE/MOSCOW", { 11 }},
    {405, 1,   "IAR", "���", "YAROSLAVL", "���������", 3458, 2392, "EUROPE/MOSCOW", { 11 }},
    {3191, 2,  "FRA", "���", "FRANKFURT", "���������-��-�����", 3070, 521, "EUROPE/BERLIN", { 10, 11 }},
    {4623, 2,  "MUC", "���", "MUNICH", "������", 2888, 695, "EUROPE/BERLIN", { 10, 11 } },
    {6503, 3,  "PAR", "���", "PARIS", "�����", 2931, 141, "EUROPE/PARIS", { 10, 11} },
    {12275, 4, "BUH", "���", "BUCHAREST", "��������", 2667, 1566, "EUROPE/BUCHAREST", { 11 }},
    {12284, 3, "NCE", "���", "NICE", "�����", 2622, 436, "EUROPE/PARIS", { 10, 11 } },
    {22915, 5, "JKV", "JKV", "JACKSONVILLE", "JACKSONVILLE", 1918, -5717, "AMERICA/CHICAGO", { 13 } },
};

boost::optional<CityId> TestCallbacks::findCityId(const EncString& s)
{
    return findIdByCode<CityId>(s, citiesForTest);
}

boost::optional<CityData> TestCallbacks::findCityData(const CityId& id)
{
    for (const auto& c : citiesForTest) {
        if (id.get() != c.id) {
            continue;
        }
        CityData cd(id, CountryId(c.country_id), boost::optional<RegionId>());
        cd.codes[ENGLISH] = EncString::from866(c.latCode);
        cd.codes[RUSSIAN] = EncString::from866(c.rusCode);
        cd.names[ENGLISH] = EncString::from866(c.latName);
        cd.names[RUSSIAN] = EncString::from866(c.rusName);
        cd.latitude = c.latit;
        cd.longitude = c.longit;
        cd.timezone = c.tz;
        return cd;
    }
    return boost::none;
}

struct TestPointData
{
    int id;
    int cityId;
    const char* latCode;
    const char* rusCode;
    const char* latName;
    const char* rusName;
    int latit;
    int longit;
} pointsForTest[] {
    {6,    405,  "IAR", "���", "TUNOSHNA", "�������", 3454, 2490},
    {25,   6503, "CDG", "���","PARIS CHARLES DE GAULLE A", "����� �� ����", 2940, 152},
    {51,   4623, "MUC", "MUC", "MUNICH INTERNATIONAL AIRP", "����� ����� �������", 2901, 707},
    {54,   3191, "FRA", "���", "FRANKFURT INTERNATIONAL A", "��������� �������������", 3001, 514},
    {78,  12275, "BBU", "BBU", "BUCHAREST BANEASA APT", "BANEASA", 2670, 1566},
    {254,  26,   "SVO", "���", "SHEREMETYEVO", "�����������", 3358, 2244},
    {255,  26,   "DME", "���", "DOMODEDOVO", "����������", 3324, 2274},
    {256,  26,   "VKO", "���", "VNUKOVO", "�������", 3335, 2235},
    {258,  29,   "KBP", "���", "BORISPOL", "���������", 3020, 1853},
    {261,  28,   "LED", "���", "PULKOVO", "�������", 3588, 1815},
    {263,  95,   "SVX", "���", "KOLTSOVO", "��������", 3405, 3648},
    {269,  56,   "ARH", "���", "TALAGI", "������", 3876, 2443},
    {279,  35,   "OMS", "���", "OMSK", "����", 3298, 4399},
    {283,  42,   "UFA", "���", "UFA", "���", 3273, 3353},
    {901, 22915, "JKV", "JKV", "JACKSONVILLE", "JACKSONVILLE", 0, 0},
    {1176, 236,  "ROV", "���", "ROSTOV", "������-��-����", 2834, 2383},
    {1187, 222,  "CSY", "���", "CHEBOKSARY", "���������", 3368, 2835},
    {1216, 27,   "VOZ", "���", "VORONEZH", "�������", 3100, 2352},
    {1296, 119,  "KGD", "���", "KALININGRAD", "�����������", 3283, 1231},
    {1304, 148,  "GOJ", "���", "NIZHNIY", "������", 3380, 2650},
    {1310, 144,  "MMK", "���", "MURMANSK", "��������", 4137, 1986},
    {1550, 12284,"NCE", "���", "NICE", "�����", 2622, 436},
    {10960, 131, "KJA", "���", "EMELYANOVO", "����������", 3370, 5550},
};

boost::optional<PointId> TestCallbacks::findPointId(const EncString& s)
{
    return findIdByCode<PointId>(s, pointsForTest);
}

boost::optional<PointData> TestCallbacks::findPointData(const PointId& id)
{
    for (const auto& c : pointsForTest) {
        if (id.get() != c.id) {
            continue;
        }
        PointData pd(id, nsi::CityId(c.cityId));
        pd.codes[ENGLISH] = EncString::from866(c.latCode);
        pd.codes[RUSSIAN] = EncString::from866(c.rusCode);
        pd.names[ENGLISH] = EncString::from866(c.latName);
        pd.names[RUSSIAN] = EncString::from866(c.rusName);
        pd.latitude = c.latit;
        pd.longitude = c.longit;
        return pd;
    }
    return boost::none;
}

// names are not implemented
struct TestAircraftData
{
    int id;
    const char* latCode;
    const char* rusCode;
} aircraftForTest[] = {
    {15, "330", "330"},
    {16, "320", "320"},
    {49, "747", "747"},
    {70, "787", "787"},
    {176, "TU5", "��5"},
};

boost::optional<AircraftTypeId> TestCallbacks::findAircraftTypeId(const EncString& s)
{
    return findIdByCode<AircraftTypeId>(s, aircraftForTest);
}

boost::optional<AircraftTypeData> TestCallbacks::findAircraftTypeData(const AircraftTypeId& id)
{
    for (const auto& a: aircraftForTest) {
        if (a.id != id.get()) {
            continue;
        }
        AircraftTypeData atd(id);
        atd.codes[ENGLISH] = EncString::from866(a.latCode);
        atd.codes[RUSSIAN] = EncString::from866(a.rusCode);
        return atd;
    }
    return boost::none;
}

boost::optional<RouterId> TestCallbacks::findRouterId(const EncString&)
{
    return boost::none;
}

boost::optional<RouterData> TestCallbacks::findRouterData(const nsi::RouterId&)
{
    return boost::none;
}

struct TestCurrencyData
{
    int id;
    const char* latCode;
    const char* rusCode;
} currenciesForTest[] {
    {1, "RUB", "���"},
    {2, "USD", "���"},
};

boost::optional<CurrencyId> TestCallbacks::findCurrencyId(const EncString& s)
{
    return findIdByCode<CurrencyId>(s, currenciesForTest);
}

boost::optional<CurrencyData> TestCallbacks::findCurrencyData(const nsi::CurrencyId& id)
{
    for (const auto& c : currenciesForTest) {
        if (id.get() != c.id) {
            continue;
        }
        CurrencyData cd(id);
        cd.codes[ENGLISH] = EncString::from866(c.latCode);
        cd.codes[RUSSIAN] = EncString::from866(c.rusCode);
        cd.names[ENGLISH] = EncString::from866("CURRENCY NAME");
        cd.names[RUSSIAN] = EncString::from866("�������� ������");
        return cd;
    }
    return boost::none;
}

std::set<GeozoneId> TestCallbacks::getGeozones(const CountryId& cid)
{
    std::set<GeozoneId> out;
    for (const auto& v : countriesForTest) {
        if (cid.get() == v.id) {
            for (auto gid : v.zones) {
                out.emplace(gid);
            }
        }
    }
    return out;
}

std::set<GeozoneId> TestCallbacks::getGeozones(const RegionId& rid)
{
    return getGeozones(CountryId(findRegionData(rid)->countryId));
}

std::set<GeozoneId> TestCallbacks::getGeozones(const CityId& cid)
{
    std::set<GeozoneId> out;
    for (const auto& v : citiesForTest) {
        if (cid.get() == v.id) {
            for (auto gid : v.zones) {
                out.emplace(gid);
            }
        }
    }
    return out;
}

#endif // XP_TESTING

} // nsi
