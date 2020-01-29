#pragma once

#include <string>
#include <iosfwd>
#include <serverlib/rip.h>
#include <serverlib/rip_validators.h>
#include <serverlib/lngv.h>
#include <serverlib/enum.h>

#include <serverlib/encstring.h>
#include <nsi/nsi.h>

namespace ct
{
bool isValidRecloc(const std::string&);
bool isValidSpecRes(const EncString&);
bool isValidGroupName(const EncString&);
bool isValidLangCode(const std::string&);
bool isValidServiceType(const std::string&);

DECL_RIP2(Recloc, std::string, ct::isValidRecloc);
DECL_RIP_LENGTH(RemoteRecloc, std::string, 5, 15);
DECL_RIP2(GroupName, EncString, ct::isValidGroupName);
DECL_RIP2(SpecRes, EncString, ct::isValidSpecRes); // special reserve code
DECL_RIP_LENGTH(DocNumber, EncString, 1, 70);
DECL_RIP2(LangCode, std::string, ct::isValidLangCode);
DECL_RIP2(ServiceType, std::string, ct::isValidServiceType);

typedef std::set<ct::SpecRes> SpecReserves;

DECL_RIP(SegId, int);
DECL_RIP(PassId, int);
DECL_RIP(DocId, int);
DECL_RIP(SsrId, int);
DECL_RIP(OsiId, int);
DECL_RIP(SvcId, int);
DECL_RIP(ContactId, int);
DECL_RIP(TaxiId, int);

struct NameParts
{
    std::string firstName;
    std::string secondName;
    std::string title;
};
bool operator==(const NameParts&, const NameParts&);
std::ostream& operator<<(std::ostream& os, const NameParts&);
NameParts parseNameParts(const std::string&);
bool isTitle(const std::string&);

// translate recloc
std::string trRecloc(const Recloc&, Language l);
std::string trRemoteRecloc(const RemoteRecloc&, Language l);

// PASS_INFANT - ����� ���� � ���⮬ � ��� ����
// �� �஢�� ������� enum ࠧ��稥 �� �������� �।����७��.
enum PassType { PASS_ADULT = 0, PASS_CHILD, PASS_INFANT };
ENUM_NAMES_DECL(PassType);

enum Sex { PASS_MALE = 0, PASS_FEMALE };
ENUM_NAMES_DECL(Sex);

enum class EmdType { EMD_A = 0, EMD_S };
ENUM_NAMES_DECL(EmdType);
std::ostream& operator<<(std::ostream&, const EmdType&);

struct DocTypeEx
{
    EncString str;
    boost::optional<nsi::DocTypeId> id;

    DocTypeEx(const EncString& i_str);
    DocTypeEx(const nsi::DocTypeId& i_id);
};
bool operator==(const DocTypeEx& lhs, const DocTypeEx& rhs);
bool operator!=(const DocTypeEx& lhs, const DocTypeEx& rhs);
std::ostream& operator<<(std::ostream& os, const DocTypeEx& docType);

bool isPassport(const ct::DocTypeEx& docType);
bool isInternationalPassport(const ct::DocTypeEx& docType);
std::string makeDocTypeText(const ct::DocTypeEx& docType);

/*******************************************************************************
 * ����� ����⭮�� ᥣ���� � PNR
 ******************************************************************************/

class SegStatus {
public:
    // � ���������� � ���� ����� �ᯮ�짮���� ᫥���騥 ᮪�饭��:
    // ���: ����ਡ�⨢��� ��⥬�
    // ���: ��થ⨭�
    // ���: ������ୠ� ��⥬� ��� ������
    //
    // !!! �������� !!!
    // ��������� �᫮��� ���祭�� ����� �ਢ��� � �஡�����
    // (��-�� �࠭���� �᫮��� ���祭�� � ��)
    enum Code {
        // ���, ���, ���:
        // ���� �� ᥣ���� ���஭�஢���
        CONFIRMED = 0,

        // ���: ᥣ���� ����襭 ��� �㤥� ����襭 � ������୮� ��⥬�
        // ���: ᥣ���� ����襭 ��� �㤥� ����襭 � ������
        // ���: ᥣ���� ������� ��筮�� ���⢥ত����
        REQUESTED = 1,

        // ���: ����襭� ��� �㤥� ����襭� ���⠭���� �� ���� ��������
        //      � ������୮� ��⥬�
        // ���: ����襭� ��� �㤥� ����襭� ���⠭���� �� ���� ��������
        //      � ������
        // ���: ����� ���������
        WAITLIST_REQUESTED = 2,

        // ���, ���, ���:
        // ������� ���⠢��� �� ���� ��������
        WAITLISTED = 3,

        // ���, ���, ���:
        // ������� �� ���ᠤ��.
        // �� ⠪�� ᥣ���� ��ᠤ�� ���ᠦ�஢ �ந�室�� �� ����������,
        // �᫨ ��⠫��� ᢮����� ����.
        // ����� �ᯮ�짮������ ��� ��ॢ���� ���㤭���� ������������.
        STANDBY = 4,

        // �����⥬�� ३�
        NON_CONTROLLED = 5,

        /***********************************************************************
         * ����⨢�� ������.
         * ������� �ਬ�୮ ���� � � �� - ᥣ���� �����⥫쭮 �⬥��,
         * �� � ࠧ���묨 ��᫮�묨 ��⥭����.
         **********************************************************************/

        // ���, ���, ���: ᥣ���� �⬥�� ������ ��� ��⮬���᪨
        // ���, ���: ����祭� 㢥�������� �� �⬥�� ᥣ����
        CANCELLED = 6,

        // ���, ���: ����祭� 㢥�������� � ⮬, �� ३� �⬥�� ��� �� �������
        // ���: ��ࠢ���� ��� �㤥� ��ࠢ���� 㢥�������� � ⮬,
        //      �� ३� �⬥�� ��� �� �������
        UNABLE = 7,

        // �⪠� �� ����� �஭�஢���� ��� �⪠� � ���⠭���� �� ���� ��������
        // ���, ���: �⪠� �� ����祭
        // ���: �⪠� �� ��� �㤥� ��ࠢ���
        REJECTED = 8
    };
    SegStatus(Code code): code_(code) {}
    Code code() const { return code_; }
    bool isActive() const;
private:
    Code code_;
};
ENUM_NAMES_DECL2(SegStatus, Code);

std::ostream& operator<<(std::ostream& out, const SegStatus& status);
inline bool operator==(const SegStatus& a, const SegStatus& b) { return a.code() == b.code(); }
inline bool operator!=(const SegStatus& a, const SegStatus& b) { return a.code() != b.code(); }
inline bool operator<(const SegStatus& a, const SegStatus& b) { return a.code() < b.code(); }
inline bool operator>(const SegStatus& a, const SegStatus& b) { return a.code() > b.code(); }

std::set<ct::SegStatus> getActiveSegStatuses();

/*******************************************************************************
 * ����� ᥣ���� �ਡ��� � PNR
 ******************************************************************************/

class ArrStatus {
public:
    // !!! �������� !!!
    // ��������� �᫮��� ���祭�� ����� �ਢ��� � �஡�����
    // (��-�� �࠭���� �᫮��� ���祭�� � ��)
    enum Code {
        CONFIRMED = 0,
        REQUESTED = 1,
        WAITLISTED = 2
    };
    ArrStatus(Code code): code_(code) {}
    Code code() const { return code_; }
private:
    Code code_;
};
ENUM_NAMES_DECL2(ArrStatus, Code);

std::ostream& operator<<(std::ostream& out, const ArrStatus& status);
inline bool operator==(const ArrStatus& a, const ArrStatus& b) { return a.code() == b.code(); }
inline bool operator!=(const ArrStatus& a, const ArrStatus& b) { return a.code() != b.code(); }
inline bool operator<(const ArrStatus& a, const ArrStatus& b) { return a.code() < b.code(); }
inline bool operator>(const ArrStatus& a, const ArrStatus& b) { return a.code() > b.code(); }

/*******************************************************************************
 * ����� SSR � PNR
 ******************************************************************************/

class SsrStatus {
public:
    // !!! �������� !!!
    // ��������� �᫮��� ���祭�� ����� �ਢ��� � �஡�����
    // (��-�� �࠭���� �᫮��� ���祭�� � ��)
    enum Code {
        CONFIRMED = 0,
        REQUESTED = 1,
        CANCELLED = 2,
        UNABLE = 3,
        REJECTED = 4,
        IGNORED = 5
    };
    SsrStatus(Code code): code_(code) {}
    Code code() const { return code_; }
    bool isActive() const;
private:
    Code code_;
};
ENUM_NAMES_DECL2(SsrStatus, Code);

std::ostream& operator<<(std::ostream& out, const SsrStatus& status);
inline bool operator==(const SsrStatus& a, const SsrStatus& b) { return a.code() == b.code(); }
inline bool operator!=(const SsrStatus& a, const SsrStatus& b) { return a.code() != b.code(); }
inline bool operator<(const SsrStatus& a, const SsrStatus& b) { return a.code() < b.code(); }
inline bool operator>(const SsrStatus& a, const SsrStatus& b) { return a.code() > b.code(); }

/*******************************************************************************
 * ����� SVC � PNR
 ******************************************************************************/

class SvcStatus {
public:
    // !!! �������� !!!
    // ��������� �᫮��� ���祭�� ����� �ਢ��� � �஡�����
    // (��-�� �࠭���� �᫮��� ���祭�� � ��)
    enum Code {
        CONFIRMED_AND_EMD_REQUIRED, // HD_STAT
        CONFIRMED_AND_EMD_ISSUED,   // HI_STAT
        CONFIRMED,                  // HK_STAT
        REQUESTED,                  // HN_STAT
        IGNORED,                    // NO_STAT
        REJECTED,                   // UC_STAT
        UNABLE,                     // UN_STAT
        CANCELLED                   // XX_STAT
    };
    SvcStatus(Code code): code_(code) {}
    Code code() const { return code_; }
    bool isActive() const;
private:
    Code code_;
};

ENUM_NAMES_DECL2(SvcStatus, Code);

std::ostream& operator<<(std::ostream& out, const SvcStatus& status);
inline bool operator==(const SvcStatus& a, const SvcStatus& b) { return a.code() == b.code(); }
inline bool operator!=(const SvcStatus& a, const SvcStatus& b) { return a.code() != b.code(); }
inline bool operator<(const SvcStatus& a, const SvcStatus& b) { return a.code() < b.code(); }
inline bool operator>(const SvcStatus& a, const SvcStatus& b) { return a.code() > b.code(); }

class SsrCode
{
public:
    explicit SsrCode(const std::string&);
    explicit SsrCode(const EncString&);

    static boost::optional<SsrCode> create(const std::string&);
    static boost::optional<SsrCode> create(const EncString&);

    const std::string& get() const;

    static SsrCode fromNsi(nsi::SsrTypeId); // remove after custom airline SSR codes
    nsi::SsrTypeId toNsi() const; // remove after custom airline SSR codes

    static const SsrCode& ADMD();
    static const SsrCode& ADTK();
    static const SsrCode& CBBG();
    static const SsrCode& CHLD();
    static const SsrCode& CHML();
    static const SsrCode& CKIN();
    static const SsrCode& CLID();
    static const SsrCode& CTCE();
    static const SsrCode& CTCM();
    static const SsrCode& CTCR();
    static const SsrCode& DBML();
    static const SsrCode& DOCA();
    static const SsrCode& DOCO();
    static const SsrCode& DOCS();
    static const SsrCode& EXST();
    static const SsrCode& FOID();
    static const SsrCode& FQTR();
    static const SsrCode& FQTS();
    static const SsrCode& FQTU();
    static const SsrCode& FQTV();
    static const SsrCode& GPST();
    static const SsrCode& GRPS();
    static const SsrCode& INFT();
    static const SsrCode& MEDA();
    static const SsrCode& NRSB();
    static const SsrCode& OTHS();
    static const SsrCode& PCTC();
    static const SsrCode& RLOC();
    static const SsrCode& RQST();
    static const SsrCode& SEAT();
    static const SsrCode& STCR();
    static const SsrCode& TKNA();
    static const SsrCode& TKNE();
    static const SsrCode& TKNM();
    static const SsrCode& TKNR();
    static const SsrCode& TKTL();
    static const SsrCode& TLAC();
    static const SsrCode& VGML();

private:
    SsrCode();
    std::string code_;
};

std::ostream& operator<<(std::ostream&, const SsrCode&);
inline bool operator==(const SsrCode& lhs, const SsrCode& rhs) { return lhs.get() == rhs.get(); }
inline bool operator!=(const SsrCode& lhs, const SsrCode& rhs) { return lhs.get() != rhs.get(); }
inline bool operator<(const SsrCode& lhs, const SsrCode& rhs) { return lhs.get() < rhs.get(); }
inline bool operator>(const SsrCode& lhs, const SsrCode& rhs) { return lhs.get() > rhs.get(); }

bool isSeatSsr(const SsrCode&);
bool isFQTVSsr(const SsrCode&);

} // namespace ct
