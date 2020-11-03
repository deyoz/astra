#include "timatic_base.h"
#include "timatic_exception.h"
#include "timatic_xml.h"

#include <serverlib/dates.h>
#define NICKNAME "TIMATIC"
#include <serverlib/slogger.h>
#include <serverlib/str_utils.h>

namespace Timatic {

//-----------------------------------------------

static const std::map<DataSection, ParamValue> DATA_SECTION_MAP = {
    {DataSection::PassportVisaHealth, {"Passport,Visa,Health", "Passport,Visa,Health"}},
    {DataSection::PassportVisa, {"Passport,Visa", "Passport,Visa"}},
    {DataSection::Passport, {"Passport", "Passport"}},
    {DataSection::Health, {"Health", "Health"}},
    {DataSection::All, {"All", "All"}},
};

static const std::map<Country, ParamValue> COUNTRY_MAP = {
    {Country::AA, {"ZZ Testing 1", "ZZ Testing 1", "AA"}},
    {Country::AD, {"Andorra", "Andorra", "AD"}},
    {Country::AE, {"United Arab Emirates", "United Arab Emirates", "AE"}},
    {Country::AF, {"Afghanistan", "Afghanistan", "AF"}},
    {Country::AG, {"Antigua and Barbuda", "Antigua and Barbuda", "AG"}},
    {Country::AI, {"Anguilla", "Anguilla", "AI"}},
    {Country::AL, {"Albania", "Albania", "AL"}},
    {Country::AM, {"Armenia", "Armenia", "AM"}},
    {Country::AO, {"Angola", "Angola", "AO"}},
    {Country::AR, {"Argentina", "Argentina", "AR"}},
    {Country::AS, {"Samoa (American)", "Samoa (American)", "AS"}},
    {Country::AT, {"Austria", "Austria", "AT"}},
    {Country::AU, {"Australia", "Australia", "AU"}},
    {Country::AW, {"Aruba", "Aruba", "AW"}},
    {Country::AZ, {"Azerbaijan", "Azerbaijan", "AZ"}},
    {Country::BA, {"Bosnia and Herzegovina", "Bosnia and Herzegovina", "BA"}},
    {Country::BB, {"Barbados", "Barbados", "BB"}},
    {Country::BD, {"Bangladesh", "Bangladesh", "BD"}},
    {Country::BE, {"Belgium", "Belgium", "BE"}},
    {Country::BF, {"Burkina Faso", "Burkina Faso", "BF"}},
    {Country::BG, {"Bulgaria", "Bulgaria", "BG"}},
    {Country::BH, {"Bahrain", "Bahrain", "BH"}},
    {Country::BI, {"Burundi", "Burundi", "BI"}},
    {Country::BJ, {"Benin", "Benin", "BJ"}},
    {Country::BL, {"St. Barthelemy", "St. Barthelemy", "BL"}},
    {Country::BM, {"Bermuda", "Bermuda", "BM"}},
    {Country::BN, {"Brunei Darussalam", "Brunei Darussalam", "BN"}},
    {Country::BO, {"Bolivia", "Bolivia", "BO"}},
    {Country::BQ, {"Bonaire, St. Eustatius and Saba", "Bonaire, St. Eustatius and Saba", "BQ"}},
    {Country::BR, {"Brazil", "Brazil", "BR"}},
    {Country::BS, {"Bahamas", "Bahamas", "BS"}},
    {Country::BT, {"Bhutan", "Bhutan", "BT"}},
    {Country::BW, {"Botswana", "Botswana", "BW"}},
    {Country::BY, {"Belarus", "Belarus", "BY"}},
    {Country::BZ, {"Belize", "Belize", "BZ"}},
    {Country::CA, {"Canada", "Canada", "CA"}},
    {Country::CC, {"Cocos (Keeling) Isl.", "Cocos (Keeling) Isl.", "CC"}},
    {Country::CD, {"Congo (Dem. Rep.)", "Congo (Dem. Rep.)", "CD"}},
    {Country::CF, {"Central African Rep.", "Central African Rep.", "CF"}},
    {Country::CG, {"Congo", "Congo", "CG"}},
    {Country::CH, {"Switzerland", "Switzerland", "CH"}},
    {Country::CI, {"Cote d'Ivoire", "Cote d'Ivoire", "CI"}},
    {Country::CK, {"Cook Isl.", "Cook Isl.", "CK"}},
    {Country::CL, {"Chile", "Chile", "CL"}},
    {Country::CM, {"Cameroon", "Cameroon", "CM"}},
    {Country::CN, {"China (People's Rep.)", "China (People's Rep.)", "CN"}},
    {Country::CO, {"Colombia", "Colombia", "CO"}},
    {Country::CR, {"Costa Rica", "Costa Rica", "CR"}},
    {Country::CU, {"Cuba", "Cuba", "CU"}},
    {Country::CV, {"Cape Verde", "Cape Verde", "CV"}},
    {Country::CW, {"Curacao", "Curacao", "CW"}},
    {Country::CX, {"Christmas Isl.", "Christmas Isl.", "CX"}},
    {Country::CY, {"Cyprus", "Cyprus", "CY"}},
    {Country::CZ, {"Czechia", "Czechia", "CZ"}},
    {Country::DE, {"Germany", "Germany", "DE"}},
    {Country::DJ, {"Djibouti", "Djibouti", "DJ"}},
    {Country::DK, {"Denmark", "Denmark", "DK"}},
    {Country::DM, {"Dominica", "Dominica", "DM"}},
    {Country::DO, {"Dominican Rep.", "Dominican Rep.", "DO"}},
    {Country::DZ, {"Algeria", "Algeria", "DZ"}},
    {Country::EC, {"Ecuador", "Ecuador", "EC"}},
    {Country::EE, {"Estonia", "Estonia", "EE"}},
    {Country::EG, {"Egypt", "Egypt", "EG"}},
    {Country::ER, {"Eritrea", "Eritrea", "ER"}},
    {Country::ES, {"Spain", "Spain", "ES"}},
    {Country::ET, {"Ethiopia", "Ethiopia", "ET"}},
    {Country::FI, {"Finland", "Finland", "FI"}},
    {Country::FJ, {"Fiji", "Fiji", "FJ"}},
    {Country::FK, {"Falkland Isl. (Malvinas)", "Falkland Isl. (Malvinas)", "FK"}},
    {Country::FM, {"Micronesia (Federated States)", "Micronesia (Federated States)", "FM"}},
    {Country::FR, {"France", "France", "FR"}},
    {Country::GA, {"Gabon", "Gabon", "GA"}},
    {Country::GB, {"United Kingdom", "United Kingdom", "GB"}},
    {Country::GD, {"Grenada", "Grenada", "GD"}},
    {Country::GE, {"Georgia", "Georgia", "GE"}},
    {Country::GF, {"French Guiana", "French Guiana", "GF"}},
    {Country::GH, {"Ghana", "Ghana", "GH"}},
    {Country::GI, {"Gibraltar", "Gibraltar", "GI"}},
    {Country::GM, {"Gambia", "Gambia", "GM"}},
    {Country::GN, {"Guinea", "Guinea", "GN"}},
    {Country::GP, {"French West Indies", "French West Indies", "GP"}},
    {Country::GQ, {"Equatorial Guinea", "Equatorial Guinea", "GQ"}},
    {Country::GR, {"Greece", "Greece", "GR"}},
    {Country::GT, {"Guatemala", "Guatemala", "GT"}},
    {Country::GU, {"Guam", "Guam", "GU"}},
    {Country::GW, {"Guinea-Bissau", "Guinea-Bissau", "GW"}},
    {Country::GY, {"Guyana", "Guyana", "GY"}},
    {Country::HK, {"Hong Kong (SAR China)", "Hong Kong (SAR China)", "HK"}},
    {Country::HN, {"Honduras", "Honduras", "HN"}},
    {Country::HR, {"Croatia", "Croatia", "HR"}},
    {Country::HT, {"Haiti", "Haiti", "HT"}},
    {Country::HU, {"Hungary", "Hungary", "HU"}},
    {Country::ID, {"Indonesia", "Indonesia", "ID"}},
    {Country::IE, {"Ireland (Rep.)", "Ireland (Rep.)", "IE"}},
    {Country::IL, {"Israel", "Israel", "IL"}},
    {Country::IN, {"India", "India", "IN"}},
    {Country::IQ, {"Iraq", "Iraq", "IQ"}},
    {Country::IR, {"Iran", "Iran", "IR"}},
    {Country::IS, {"Iceland", "Iceland", "IS"}},
    {Country::IT, {"Italy", "Italy", "IT"}},
    {Country::JM, {"Jamaica", "Jamaica", "JM"}},
    {Country::JO, {"Jordan", "Jordan", "JO"}},
    {Country::JP, {"Japan", "Japan", "JP"}},
    {Country::KE, {"Kenya", "Kenya", "KE"}},
    {Country::KG, {"Kyrgyzstan", "Kyrgyzstan", "KG"}},
    {Country::KH, {"Cambodia", "Cambodia", "KH"}},
    {Country::KI, {"Kiribati", "Kiribati", "KI"}},
    {Country::KM, {"Comoros", "Comoros", "KM"}},
    {Country::KN, {"St. Kitts and Nevis", "St. Kitts and Nevis", "KN"}},
    {Country::KP, {"Korea (Dem. People's Rep.)", "Korea (Dem. People's Rep.)", "KP"}},
    {Country::KR, {"Korea (Rep.)", "Korea (Rep.)", "KR"}},
    {Country::KW, {"Kuwait", "Kuwait", "KW"}},
    {Country::KY, {"Cayman Isl.", "Cayman Isl.", "KY"}},
    {Country::KZ, {"Kazakhstan", "Kazakhstan", "KZ"}},
    {Country::LA, {"Lao People's Dem. Rep.", "Lao People's Dem. Rep.", "LA"}},
    {Country::LB, {"Lebanon", "Lebanon", "LB"}},
    {Country::LC, {"St. Lucia", "St. Lucia", "LC"}},
    {Country::LI, {"Liechtenstein", "Liechtenstein", "LI"}},
    {Country::LK, {"Sri Lanka", "Sri Lanka", "LK"}},
    {Country::LR, {"Liberia", "Liberia", "LR"}},
    {Country::LS, {"Lesotho", "Lesotho", "LS"}},
    {Country::LT, {"Lithuania", "Lithuania", "LT"}},
    {Country::LU, {"Luxembourg", "Luxembourg", "LU"}},
    {Country::LV, {"Latvia", "Latvia", "LV"}},
    {Country::LY, {"Libya", "Libya", "LY"}},
    {Country::MA, {"Morocco", "Morocco", "MA"}},
    {Country::MC, {"Monaco", "Monaco", "MC"}},
    {Country::MD, {"Moldova (Rep.)", "Moldova (Rep.)", "MD"}},
    {Country::ME, {"Montenegro", "Montenegro", "ME"}},
    {Country::MF, {"St. Martin", "St. Martin", "MF"}},
    {Country::MG, {"Madagascar", "Madagascar", "MG"}},
    {Country::MH, {"Marshall Isl.", "Marshall Isl.", "MH"}},
    {Country::MK, {"North Macedonia (Rep.)", "North Macedonia (Rep.)", "MK"}},
    {Country::ML, {"Mali", "Mali", "ML"}},
    {Country::MM, {"Myanmar", "Myanmar", "MM"}},
    {Country::MN, {"Mongolia", "Mongolia", "MN"}},
    {Country::MO, {"Macao (SAR China)", "Macao (SAR China)", "MO"}},
    {Country::MP, {"Northern Mariana Isl.", "Northern Mariana Isl.", "MP"}},
    {Country::MQ, {"Martinique", "Martinique", "MQ"}},
    {Country::MR, {"Mauritania", "Mauritania", "MR"}},
    {Country::MS, {"Montserrat", "Montserrat", "MS"}},
    {Country::MT, {"Malta", "Malta", "MT"}},
    {Country::MU, {"Mauritius", "Mauritius", "MU"}},
    {Country::MV, {"Maldives", "Maldives", "MV"}},
    {Country::MW, {"Malawi", "Malawi", "MW"}},
    {Country::MX, {"Mexico", "Mexico", "MX"}},
    {Country::MY, {"Malaysia", "Malaysia", "MY"}},
    {Country::MZ, {"Mozambique", "Mozambique", "MZ"}},
    {Country::NA, {"Namibia", "Namibia", "NA"}},
    {Country::NC, {"New Caledonia", "New Caledonia", "NC"}},
    {Country::NE, {"Niger", "Niger", "NE"}},
    {Country::NF, {"Norfolk Isl.", "Norfolk Isl.", "NF"}},
    {Country::NG, {"Nigeria", "Nigeria", "NG"}},
    {Country::NI, {"Nicaragua", "Nicaragua", "NI"}},
    {Country::NL, {"Netherlands", "Netherlands", "NL"}},
    {Country::NO, {"Norway", "Norway", "NO"}},
    {Country::NP, {"Nepal", "Nepal", "NP"}},
    {Country::NR, {"Nauru", "Nauru", "NR"}},
    {Country::NU, {"Niue", "Niue", "NU"}},
    {Country::NZ, {"New Zealand", "New Zealand", "NZ"}},
    {Country::OM, {"Oman", "Oman", "OM"}},
    {Country::PA, {"Panama", "Panama", "PA"}},
    {Country::PE, {"Peru", "Peru", "PE"}},
    {Country::PF, {"French Polynesia", "French Polynesia", "PF"}},
    {Country::PG, {"Papua New Guinea", "Papua New Guinea", "PG"}},
    {Country::PH, {"Philippines", "Philippines", "PH"}},
    {Country::PK, {"Pakistan", "Pakistan", "PK"}},
    {Country::PL, {"Poland", "Poland", "PL"}},
    {Country::PM, {"St. Pierre and Miquelon", "St. Pierre and Miquelon", "PM"}},
    {Country::PN, {"Pitcairn Isl.", "Pitcairn Isl.", "PN"}},
    {Country::PR, {"Puerto Rico", "Puerto Rico", "PR"}},
    {Country::PS, {"Palestinian Territory", "Palestinian Territory", "PS"}},
    {Country::PT, {"Portugal", "Portugal", "PT"}},
    {Country::PW, {"Palau", "Palau", "PW"}},
    {Country::PY, {"Paraguay", "Paraguay", "PY"}},
    {Country::QA, {"Qatar", "Qatar", "QA"}},
    {Country::RE, {"Reunion", "Reunion", "RE"}},
    {Country::RK, {"Kosovo (Rep.)", "Kosovo (Rep.)", "RK"}},
    {Country::RO, {"Romania", "Romania", "RO"}},
    {Country::RS, {"Serbia", "Serbia", "RS"}},
    {Country::RU, {"Russian Fed.", "Russian Fed.", "RU"}},
    {Country::RW, {"Rwanda", "Rwanda", "RW"}},
    {Country::SA, {"Saudi Arabia", "Saudi Arabia", "SA"}},
    {Country::SB, {"Solomon Isl.", "Solomon Isl.", "SB"}},
    {Country::SC, {"Seychelles", "Seychelles", "SC"}},
    {Country::SD, {"Sudan", "Sudan", "SD"}},
    {Country::SE, {"Sweden", "Sweden", "SE"}},
    {Country::SG, {"Singapore", "Singapore", "SG"}},
    {Country::SH, {"St. Helena", "St. Helena", "SH"}},
    {Country::SI, {"Slovenia", "Slovenia", "SI"}},
    {Country::SK, {"Slovakia", "Slovakia", "SK"}},
    {Country::SL, {"Sierra Leone", "Sierra Leone", "SL"}},
    {Country::SM, {"San Marino", "San Marino", "SM"}},
    {Country::SN, {"Senegal", "Senegal", "SN"}},
    {Country::SO, {"Somalia", "Somalia", "SO"}},
    {Country::SR, {"Suriname", "Suriname", "SR"}},
    {Country::SS, {"South Sudan", "South Sudan", "SS"}},
    {Country::ST, {"Sao Tome and Principe", "Sao Tome and Principe", "ST"}},
    {Country::SV, {"El Salvador", "El Salvador", "SV"}},
    {Country::SX, {"St. Maarten", "St. Maarten", "SX"}},
    {Country::SY, {"Syria", "Syria", "SY"}},
    {Country::SZ, {"Eswatini (Swaziland)", "Eswatini (Swaziland)", "SZ"}},
    {Country::TC, {"Turks and Caicos Isl.", "Turks and Caicos Isl.", "TC"}},
    {Country::TD, {"Chad", "Chad", "TD"}},
    {Country::TG, {"Togo", "Togo", "TG"}},
    {Country::TH, {"Thailand", "Thailand", "TH"}},
    {Country::TJ, {"Tajikistan", "Tajikistan", "TJ"}},
    {Country::TL, {"Timor-Leste", "Timor-Leste", "TL"}},
    {Country::TM, {"Turkmenistan", "Turkmenistan", "TM"}},
    {Country::TN, {"Tunisia", "Tunisia", "TN"}},
    {Country::TO, {"Tonga", "Tonga", "TO"}},
    {Country::TR, {"Turkey", "Turkey", "TR"}},
    {Country::TT, {"Trinidad and Tobago", "Trinidad and Tobago", "TT"}},
    {Country::TV, {"Tuvalu", "Tuvalu", "TV"}},
    {Country::TW, {"Chinese Taipei", "Chinese Taipei", "TW"}},
    {Country::TZ, {"Tanzania", "Tanzania", "TZ"}},
    {Country::UA, {"Ukraine", "Ukraine", "UA"}},
    {Country::UG, {"Uganda", "Uganda", "UG"}},
    {Country::US, {"USA", "USA", "US"}},
    {Country::UU, {"Utopia", "Utopia", "UU"}},
    {Country::UY, {"Uruguay", "Uruguay", "UY"}},
    {Country::UZ, {"Uzbekistan", "Uzbekistan", "UZ"}},
    {Country::VA, {"Vatican City (Holy See)", "Vatican City (Holy See)", "VA"}},
    {Country::VC, {"St. Vincent and the Grenadines", "St. Vincent and the Grenadines", "VC"}},
    {Country::VE, {"Venezuela", "Venezuela", "VE"}},
    {Country::VG, {"Virgin Isl. (British)", "Virgin Isl. (British)", "VG"}},
    {Country::VI, {"Virgin Isl. (USA)", "Virgin Isl. (USA)", "VI"}},
    {Country::VN, {"Viet Nam", "Viet Nam", "VN"}},
    {Country::VU, {"Vanuatu", "Vanuatu", "VU"}},
    {Country::WF, {"Wallis and Futuna Isl.", "Wallis and Futuna Isl.", "WF"}},
    {Country::WS, {"Samoa", "Samoa", "WS"}},
    {Country::XA, {"Stateless Persons", "Stateless Persons", "XA"}},
    {Country::XB, {"Refugee B", "Refugee B", "XB"}},
    {Country::XC, {"Refugee C", "Refugee C", "XC"}},
    {Country::XX, {"Stateless and Refugees", "Stateless and Refugees", "XX"}},
    {Country::YE, {"Yemen", "Yemen", "YE"}},
    {Country::YT, {"Mayotte", "Mayotte", "YT"}},
    {Country::ZA, {"South Africa", "South Africa", "ZA"}},
    {Country::ZM, {"Zambia", "Zambia", "ZM"}},
    {Country::ZW, {"Zimbabwe", "Zimbabwe", "ZW"}},
    {Country::ZZ, {"United Nations", "United Nations", "ZZ"}},
};

static const std::map<DocumentType, ParamValue> DOC_TYPE_MAP = {
    {DocumentType::AliensPassport, {"AliensPassport", "Alien's Passport"}},
    {DocumentType::BirthCertificate, {"BirthCertificate", "Birth Certificate"}},
    {DocumentType::BritishCitizen, {"BritishCitizen", "\"British Citizen\" passport"}},
    {DocumentType::BritishNationalOverseas, {"BritishNationalOverseas", "\"British National (Overseas)\" passport"}},
    {DocumentType::BritishOverseasTerritoriesCitizen, {"BritishOverseasTerritoriesCitizen", "\"British Overseas Territories Citizen\" passport"}},
    {DocumentType::BritishProtectedPerson, {"BritishProtectedPerson", "\"British Protected Person\" passport"}},
    {DocumentType::BritishSubject, {"BritishSubject", "\"British Subject\" passport"}},
    {DocumentType::BritshOverseasCitizen, {"BritshOverseasCitizen", "\"British Overseas Citizen\" passport"}},
    {DocumentType::CDB, {"CDB", "Caribbean Development Bank Travel Certificate"}},
    {DocumentType::CNExitandEntryPermit, {"CNExitandEntryPermit", "CN Exit and Entry Permit"}},
    {DocumentType::CNOneWayPermit, {"CNOneWayPermit", "CN Travel Permit to HK and MO (One-way Permit)"}},
    {DocumentType::CTD1951, {"CTD1951", "Travel Document Convention 1951"}},
    {DocumentType::CTD1954, {"CTD1954", "Travel Document Convention 1954"}},
    {DocumentType::CertificateofCitizenship, {"CertificateofCitizenship", "Certificate of Citizenship"}},
    {DocumentType::CertificateofIdentity, {"CertificateofIdentity", "Certificate of Identity"}},
    {DocumentType::CertificateofNaturalization, {"CertificateofNaturalization", "Certificate of Naturalization"}},
    {DocumentType::ConsularPassport, {"ConsularPassport", " Passport: Consular"}},
    {DocumentType::CrewGeneralDeclarationForm, {"CrewGeneralDeclarationForm", "Crew General Declaration Form"}},
    {DocumentType::CrewMemberCertificate, {"CrewMemberCertificate", "Crew Member Certificate"}},
    {DocumentType::CrewMemberIDCard, {"CrewMemberIDCard", "Crew Member ID Card"}},
    {DocumentType::CrewMemberLicence, {"CrewMemberLicence", "Crew Member Licence"}},
    {DocumentType::Diplomaticpassport, {"Diplomaticpassport", " Passport: Diplomatic"}},
    {DocumentType::DocumentofIdentity, {"DocumentofIdentity", "Document of Identity"}},
    {DocumentType::EmergencyPassport, {"EmergencyPassport", "Emergency Passport"}},
    {DocumentType::FormI327, {"FormI327", "Permit to Re-Enter the USA"}},
    {DocumentType::FormI512, {"FormI512", "Authorization for Parole of an Alien into the United States"}},
    {DocumentType::FormI571, {"FormI571", "US Refugee Travel Document"}},
    {DocumentType::GRPoliceID, {"GRPoliceID", "Hellenic Police ID Card"}},
    {DocumentType::HongKongSARChinapassport, {"HongKongSARChinapassport", " Passport: Hong Kong (SAR China)"}},
    {DocumentType::HuiXiangZheng, {"HuiXiangZheng", "\"Hui Xiang Zheng\""}},
    {DocumentType::ICRCTD, {"ICRCTD", "Travel Document Red Cross"}},
    {DocumentType::InterpolIDCard, {"InterpolIDCard", "Interpol ID Card"}},
    {DocumentType::InterpolPassport, {"InterpolPassport", "Interpol Passport"}},
    {DocumentType::Kinderausweiswithoutphotograph, {"Kinderausweiswithoutphotograph", "\"Kinderausweis\" without photograph"}},
    {DocumentType::Kinderausweiswithphotograph, {"Kinderausweiswithphotograph", "\"Kinderausweis\" with photograph"}},
    {DocumentType::LaissezPasser, {"Laissez-Passer", "\"Laissez-Passer\""}},
    {DocumentType::LuBaoZheng, {"LuBaoZheng", "\"Lu Bao Zheng\""}},
    {DocumentType::LuXingZheng, {"LuXingZheng", "\"Lu Xing Zheng\" "}},
    {DocumentType::MacaoSARChinapassport, {"MacaoSARChinapassport", " Passport: Macao (SAR China) "}},
    {DocumentType::MilitaryIDCard, {"MilitaryIDCard", "Military ID Card"}},
    {DocumentType::MilitaryIdentityDocument, {"MilitaryIdentityDocument", "Military Identity Document"}},
    {DocumentType::NationalIDCard, {"nationalidcard", "National ID Card"}},
    {DocumentType::NexusCard, {"NexusCard", "NEXUS Card "}},
    {DocumentType::None, {"none", "No Document Type held"}},
    {DocumentType::NotarizedAffidavitofCitizenship, {"NotarizedAffidavitofCitizenship", "Notarized Affidavit of Citizenship"}},
    {DocumentType::OASOfficialTravelDocument, {"OASOfficialTravelDocument", "OAS Official Travel Document"}},
    {DocumentType::OfficialPassport, {"OfficialPassport", " Passport: Official "}},
    {DocumentType::OfficialPhotoID, {"OfficialPhotoID", "Official Photo ID"}},
    {DocumentType::PSTD, {"PSTD", "Travel Document Palestinian Refugees"}},
    {DocumentType::Passport, {"Passport", " Passport: Normal"}},
    {DocumentType::PassportCard, {"PassportCard", "Passport Card"}},
    {DocumentType::PublicAffairsHKMOTravelPermit, {"PublicAffairsHKMOTravelPermit", "Public Affairs HK and MO Travel Permit"}},
    {DocumentType::SJDR, {"SJDR", "Single Journey Document for Resettlement"}},
    {DocumentType::SJTD, {"SJTD", "Single Journey Travel Document"}},
    {DocumentType::SeafarerID, {"SeafarerID", "Seafarers' Identity Document"}},
    {DocumentType::SeamanBook, {"SeamanBook", "Seaman Book"}},
    {DocumentType::ServicePassport, {"ServicePassport", " Passport: Service"}},
    {DocumentType::SpecialPassport, {"SpecialPassport", " Passport: Special"}},
    {DocumentType::TaiBaoZheng, {"TaiBaoZheng", "\"Tai Bao Zheng\""}},
    {DocumentType::TemporaryTravelDocument, {"TemporaryTravelDocument", "Temporary Travel Document "}},
    {DocumentType::Temporarypassport, {"Temporarypassport", "Temporary passport"}},
    {DocumentType::TitredeVoyage, {"TitredeVoyage", "\"Titre de Voyage\""}},
    {DocumentType::TransportationLetter, {"TransportationLetter", "Transportation Letter"}},
    {DocumentType::TravelCertificate, {"TravelCertificate", "Travel Certificate "}},
    {DocumentType::TravelPermit, {"TravelPermit", "Travel Permit"}},
    {DocumentType::TravelPermitHKMO, {"TravelPermitHKMO", "Travel Permit to/from HK and MO"}},
    {DocumentType::UNLaissezPasser, {"U.N.Laissez-Passer", "\"Laissez-Passer\" issued by the United Nations"}},
    {DocumentType::UNMIKTravelDocument, {"UNMIKTravelDocument", "UNMIK Travel Document"}},
    {DocumentType::VisitPermitforResidentsofMacaotoHKSAR, {"VisitPermitforResidentsofMacaotoHKSAR", "Macao (SAR China) \"Visit Permit for Residents of Macao to HKSAR\""}},
    {DocumentType::VotersRegistrationCard, {"VotersRegistrationCard", "Voter's Registration Card"}},
};

static const std::map<SecondaryDocumentType, ParamValue> SEC_DOC_TYPE_MAP = {
    {SecondaryDocumentType::BirthCertificate, {"BirthCertificate", "Birth Certificate"}},
    {SecondaryDocumentType::CNExitandEntryPermit, {"CNExitandEntryPermit", "CN Exit and Entry Permit"}},
    {SecondaryDocumentType::CNOneWayPermit, {"CNOneWayPermit", "CN Travel Permit to HK and MO (One-way Permit)"}},
    {SecondaryDocumentType::HuiXiangZheng, {"HuiXiangZheng", "\"Hui Xiang Zheng\""}},
    {SecondaryDocumentType::InterpolIDCard, {"InterpolIDCard", "Interpol ID Card"}},
    {SecondaryDocumentType::InterpolPassport, {"InterpolPassport", "Interpol Passport"}},
    {SecondaryDocumentType::LuBaoZheng, {"LuBaoZheng", "\"Lu Bao Zheng\""}},
    {SecondaryDocumentType::LuXingZheng, {"LuXingZheng", "\"Lu Xing Zheng\""}},
    {SecondaryDocumentType::MilitaryIDCard, {"MilitaryIDCard", "Military ID Card"}},
    {SecondaryDocumentType::NationalIDCard, {"nationalidcard", "National ID Card"}},
    {SecondaryDocumentType::PublicAffairsHKMOTravelPermit, {"PublicAffairsHKMOTravelPermit", "Public Affairs HK and MO Travel Permit"}},
    {SecondaryDocumentType::SeafarerID, {"SeafarerID", "Seafarers' Identity Document"}},
    {SecondaryDocumentType::SeamanBook, {"SeamanBook", "Seaman Book"}},
    {SecondaryDocumentType::TaiBaoZheng, {"TaiBaoZheng", "\"Tai Bao Zheng\""}},
    {SecondaryDocumentType::TravelPermitHKMO, {"TravelPermitHKMO", "Travel Permit to/from HK and MO"}},
};

static const std::map<DocumentGroup, ParamValue> DOC_GROUP_MAP = {
    {DocumentGroup::AlienResidents, {"AR", "Alien Residents"}},
    {DocumentGroup::AirlineCrew, {"AirlineCrew", "Airline Crew"}},
    {DocumentGroup::GovernmentDutyPassports, {"D", "Government Duty Passports"}},
    {DocumentGroup::MerchantSeamen, {"MS", "Merchant Seamen"}},
    {DocumentGroup::Military, {"Military", "Military"}},
    {DocumentGroup::Normal, {"N", "Normal Passport"}},
    {DocumentGroup::Other, {"Other", "Other"}},
    {DocumentGroup::StatelessAndRefugees, {"SR", "Stateless and Refugees"}},
};

static const std::map<Gender, ParamValue> GENDER_MAP = {
    {Gender::Male, {"M", "Male"}},
    {Gender::Female, {"F", "Female"}},
};

static const std::map<StayType, ParamValue> STAY_TYPE_MAP = {
    {StayType::Business, {"business", "Business"}},
    {StayType::Duty, {"Duty", "Duty"}},
    {StayType::Vacation, {"vacation", " Tourism/Vacation"}},
};

static const std::map<Visa, ParamValue> VISA_MAP = {
    {Visa::NoVisa, {"NoVisa", "I do not have a visa"}},
    {Visa::ValidVisa, {"ValidVisa", "I have valid visa"}},
};

static const std::map<ResidencyDocument, ParamValue> RESIDENCY_DOC_MAP = {
    {ResidencyDocument::AliensPassport, {"AliensPassport", "Alien's Passport"}},
    {ResidencyDocument::PermanentResidentResidentAlienCardFormI551, {"PermanentResidentResidentAlienCardFormI551", " Permanent Resident/Resident Alien Card (Form I-551)"}},
    {ResidencyDocument::ReentryPermit, {"ReentryPermit", " Re-entry Permit"}},
    {ResidencyDocument::ResidencePermit, {"ResidencePermit", "  Residence Permit"}},
};

static const std::map<DocumentFeature, ParamValue> DOC_FEATURE_MAP = {
    {DocumentFeature::Biometric, {"Biometric", "Biometric"}},
    {DocumentFeature::DigitalPhoto, {"DigitalPhoto", "Digital Photo"}},
    {DocumentFeature::MachineReadableDocument, {"MRD", "Machine-readable document"}},
    {DocumentFeature::MachineReadableDocumentWithDigitalPhoto, {"MRDwithDigitalPhoto", "Machine-readable document with digital photo"}},
    {DocumentFeature::None, {"none", "None"}},
};

static const std::map<Ticket, ParamValue> TICKET_MAP = {
    {Ticket::NoTicket, {"NoTicket", "No Ticket"}},
    {Ticket::Ticket, {"Ticket", "Return/Onward Ticket"}},
};

static const std::map<ParameterName, ParamValue> PARAMETER_NAME_MAP = {
    {ParameterName::Country, {"country", "country"}},
    {ParameterName::DataSection, {"dataSection", "dataSection"}},
    {ParameterName::DocumentType, {"documentType", "documentType"}},
    {ParameterName::SecondaryDocumentType, {"secondaryDocumentType", "secondaryDocumentType"}},
    {ParameterName::DocumentGroup, {"documentGroup", "documentGroup"}},
    {ParameterName::Gender, {"gender", "gender"}},
    {ParameterName::StayType, {"stayType", "stayType"}},
    {ParameterName::Visa, {"visa", "visa"}},
    {ParameterName::ResidencyDocument, {"residencyDocument", "residencyDocument"}},
    {ParameterName::DocumentFeature, {"documentFeature", "documentFeature"}},
    {ParameterName::Ticket, {"ticket", "ticket"}},
};

static const std::map<GroupName, ParamValue> GROUP_NAME_MAP = {
    {GroupName::Organisation, {"organisation", "organisation"}},
    {GroupName::Infected, {"infected", "infected"}},
};

//-----------------------------------------------

const ParamValue &getParams(const Country &val)
{
    auto it = COUNTRY_MAP.find(val);
    if (it != COUNTRY_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

const ParamValue &getParams(const DataSection &val)
{
    auto it = DATA_SECTION_MAP.find(val);
    if (it != DATA_SECTION_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

const ParamValue &getParams(const DocumentType &val)
{
    auto it = DOC_TYPE_MAP.find(val);
    if (it != DOC_TYPE_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

const ParamValue &getParams(const SecondaryDocumentType &val)
{
    auto it = SEC_DOC_TYPE_MAP.find(val);
    if (it != SEC_DOC_TYPE_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

const ParamValue &getParams(const DocumentGroup &val)
{
    auto it = DOC_GROUP_MAP.find(val);
    if (it != DOC_GROUP_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

const ParamValue &getParams(const Gender &val)
{
    auto it = GENDER_MAP.find(val);
    if (it != GENDER_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

const ParamValue &getParams(const StayType &val)
{
    auto it = STAY_TYPE_MAP.find(val);
    if (it != STAY_TYPE_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

const ParamValue &getParams(const Visa &val)
{
    auto it = VISA_MAP.find(val);
    if (it != VISA_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

const ParamValue &getParams(const ResidencyDocument &val)
{
    auto it = RESIDENCY_DOC_MAP.find(val);
    if (it != RESIDENCY_DOC_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

const ParamValue &getParams(const DocumentFeature &val)
{
    auto it = DOC_FEATURE_MAP.find(val);
    if (it != DOC_FEATURE_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

const ParamValue &getParams(const Ticket &val)
{
    auto it = TICKET_MAP.find(val);
    if (it != TICKET_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

const ParamValue &getParams(const ParameterName &val)
{
    auto it = PARAMETER_NAME_MAP.find(val);
    if (it != PARAMETER_NAME_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

const ParamValue &getParams(const GroupName &val)
{
    auto it = GROUP_NAME_MAP.find(val);
    if (it != GROUP_NAME_MAP.end())
        return it->second;
    throw NotFoundError(BIGLOG);
}

//-----------------------------------------------

const std::map<Country, ParamValue> &getCountryMap() { return COUNTRY_MAP; }
const std::map<DataSection, ParamValue> &getDataSectionMap() { return DATA_SECTION_MAP; }
const std::map<DocumentType, ParamValue> &getDocumentTypeMap() { return DOC_TYPE_MAP; }
const std::map<SecondaryDocumentType, ParamValue> &getSecondaryDocumentTypeMap() { return SEC_DOC_TYPE_MAP; }
const std::map<ParameterName, ParamValue> &getParameterNameMap() { return PARAMETER_NAME_MAP; }
const std::map<DocumentGroup, ParamValue> &getDocumentGroupMap() { return DOC_GROUP_MAP; }
const std::map<Gender, ParamValue> &getGenderMap() { return GENDER_MAP; }
const std::map<StayType, ParamValue> &getStayTypeMap() { return STAY_TYPE_MAP; }
const std::map<Visa, ParamValue> &getVisaMap() { return VISA_MAP; }
const std::map<ResidencyDocument, ParamValue> &getResidencyDocumentMap() { return RESIDENCY_DOC_MAP; }
const std::map<DocumentFeature, ParamValue> &getDocumentFeatureMap() { return DOC_FEATURE_MAP; }
const std::map<Ticket, ParamValue> &getTicketMap() { return TICKET_MAP; }

//-----------------------------------------------

std::string toString(const Country &val) { return getParams(val).name; }
std::string toString(const DataSection &val) { return getParams(val).name; }
std::string toString(const DocumentType &val) { return getParams(val).name; }
std::string toString(const SecondaryDocumentType &val) { return getParams(val).name; }
std::string toString(const DocumentGroup &val) { return getParams(val).name; }
std::string toString(const Gender &val) { return getParams(val).name; }
std::string toString(const StayType &val) { return getParams(val).name; }
std::string toString(const Visa &val) { return getParams(val).name; }
std::string toString(const ResidencyDocument &val) { return getParams(val).name; }
std::string toString(const DocumentFeature &val) { return getParams(val).name; }
std::string toString(const Ticket &val) { return getParams(val).name; }
std::string toString(const ParameterName &val) { return getParams(val).name; }
std::string toString(const GroupName &val) { return getParams(val).name; }

std::string toString(const boost::posix_time::ptime &val, const DateFormat &df)
{
    if (!val.is_special()) {
        // YYYY-MM-DDTHH:MM:SS (19 symbols)
        const auto value = boost::posix_time::to_iso_extended_string(val);
        switch(df) {
            case DateFormat::Date: return value.substr(0, 10);
            case DateFormat::DateTime: return value.substr(0, 19);
        }
    }
    return {};
}

//-----------------------------------------------

ErrorType::ErrorType(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "message"))
            message = xGetStr(child);
        else if (xCmpNames(child, "code"))
            code = xGetStr(child);
    }
}

//-----------------------------------------------

MessageType::MessageType(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "text"))
            text = xGetStr(child);
        else if (xCmpNames(child, "code"))
            code = xGetStr(child);
    }
}

//-----------------------------------------------

ParameterType::ParameterType(const xmlNodePtr node)
    : mandatory(false)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "parameterName"))
            parameterName = xGetStr(child);
        else if (xCmpNames(child, "mandatory"))
            mandatory = xGetBool(child);
    }
}

//-----------------------------------------------

ParamValue::ParamValue(const char *pn, const char *pd)
    : name(pn), displayName(pd)
{
}

ParamValue::ParamValue(const char *pn, const char *pd, const char *pc)
    : ParamValue(pn, pd)
{
    code = pc;
}

ParamValue::ParamValue(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "name"))
            name = xGetStr(child);
        else if (xCmpNames(child, "displayName"))
            displayName = xGetStr(child);
        else if (xCmpNames(child, "code"))
            code = xGetStr(child);
    }
}

//----------------------------------------------

CityDetail::CityDetail(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "cityName"))
            cityName = xGetStr(child);
        else if (xCmpNames(child, "cityCode"))
            cityCode = xGetStr(child);
    }
}

//----------------------------------------------

AlternateDetail::AlternateDetail(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "countryName"))
            countryName = xGetStr(child);
        else if (xCmpNames(child, "countryCode"))
            countryCode = xGetStr(child);
    }
}

//----------------------------------------------

VisaType::VisaType(const xmlNodePtr node)
{
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (xCmpNames(child, "visaType"))
            visaType = xGetStr(child);
        else if (xCmpNames(child, "issuedBy"))
            issuedBy = xGetStr(child);
        else if (xCmpNames(child, "applicableFor"))
            applicableFor = xGetStr(child);
        else if (xCmpNames(child, "expiryDate"))
            expiryDateGMT = getPTime(xGetStr(child));
    }
}

//----------------------------------------------

void StayDuration::validate() const
{
    if (days_ && *days_ < 0)
        throw ValidateError(BIGLOG, "days=%d", *days_);

    if (hours_ && *hours_ < 0)
        throw ValidateError(BIGLOG, "hours=%d", *hours_);
}

void StayDuration::fill(xmlNodePtr node) const
{
    auto child = xmlNewChild(node, nullptr, "stayDuration", nullptr);
;
    if (days_)
        xmlNewChild(child, nullptr, "days", std::to_string(*days_));

    if (hours_)
        xmlNewChild(child, nullptr, "hours", std::to_string(*hours_));
}

//-----------------------------------------------

void VisaData::validate() const
{
    if (visaType_.size() < 1 || visaType_.size() > 10)
        throw ValidateError(BIGLOG, "visaType=%s", visaType_.c_str());

    if (issuedBy_.size() < 2 || issuedBy_.size() > 4)
        throw ValidateError(BIGLOG, "issuedBy=%s", issuedBy_.c_str());

    if (expiryDateGMT_.is_special())
        throw ValidateError(BIGLOG, "expiryDateGMT is special");
}

void VisaData::fill(xmlNodePtr node) const
{
    auto child = xmlNewChild(node, nullptr, "visaData", nullptr);
    xmlNewChild(child, nullptr, "visaType", visaType_);
    xmlNewChild(child, nullptr, "issuedBy", issuedBy_);
    xmlNewChild(child, nullptr, "expiryDate", toString(expiryDateGMT_, DateFormat::DateTime));
}

//-----------------------------------------------

void TransitCountry::validate() const
{
    if (airportCode_.size() < 2 || airportCode_.size() > 3)
        throw ValidateError(BIGLOG, "airportCode=%s", airportCode_.c_str());

    if (arrivalTimestamp_ && arrivalTimestamp_->is_special())
        throw ValidateError(BIGLOG, "arrivalTimestamp is special");

    if (departTimestamp_ && departTimestamp_->is_special())
        throw ValidateError(BIGLOG, "departTimestamp is special");

    if (visa_ && (visa_ < Visa::NoVisa || visa_ > Visa::ValidVisa))
        throw ValidateError(BIGLOG, "visa=%d", (int)*visa_);

    if (ticket_ && (ticket_ < Ticket::NoTicket || ticket_ > Ticket::Ticket))
        throw ValidateError(BIGLOG, "ticket=%d", (int)*ticket_);

    if (visaData_)
        visaData_->validate();
}

void TransitCountry::fill(xmlNodePtr node) const
{
    auto child = xmlNewChild(node, nullptr, "transitCountry", nullptr);
    xmlNewChild(child, nullptr, "airportCode", airportCode_);

    if (arrivalTimestamp_)
        xmlNewChild(child, nullptr, "arrivalTimestamp", toString(*arrivalTimestamp_, DateFormat::DateTime));

    if (departTimestamp_)
        xmlNewChild(child, nullptr, "departTimestamp", toString(*departTimestamp_, DateFormat::DateTime));

    if (visa_)
        xmlNewChild(child, nullptr, "visa", toString(*visa_));

    if(ticket_)
        xmlNewChild(child, nullptr, "ticket", toString(*ticket_));

    if (visaData_)
        visaData_->fill(child);
}

//-----------------------------------------------

Config::Config(const std::string &username, const std::string &subUsername, const std::string &password, const std::string &host, const int port)
    : username(username),
      subUsername(subUsername),
      password(password),
      host(host),
      port(port)
{
}

//-----------------------------------------------



//-----------------------------------------------

DataSection getDataSection(const std::string &val)
{
    if (val == "Passport,Visa,Health" || val == "passport,visa,health")
        return DataSection::PassportVisaHealth;

    if (val == "Passport,Visa" || val == "passport,visa")
        return DataSection::PassportVisa;

    if (val == "Passport" || val == "passport")
        return DataSection::Passport;

    if (val == "Health" || val == "health")
        return DataSection::Health;

    if (val == "All" || val == "all")
        return DataSection::All;

    return DataSection::All;
}

SufficientDocumentation getSufficientDocumentation(const std::string &val)
{
    if (val == "Yes" || val == "yes")
        return SufficientDocumentation::Yes;
    else if (val == "Conditional" || val == "conditional")
        return SufficientDocumentation::Conditional;
    else if (val == "No" || val == "no")
        return SufficientDocumentation::No;

    return SufficientDocumentation::No;
}

ParagraphType getParagraphType(const std::string &val)
{
    if (val == "Information" || val == "information")
        return ParagraphType::Information;

    if (val == "Restriction" || val == "restriction")
        return ParagraphType::Restriction;

    if (val == "Exception" || val == "exception")
        return ParagraphType::Exception;

    if (val == "Requirement" || val == "requirement")
        return ParagraphType::Requirement;

    if (val == "Recommendation" || val == "recommendation")
        return ParagraphType::Recommendation;

    if (val == "Applicable" || val == "applicable")
        return ParagraphType::Applicable;

    return ParagraphType::Information;
}

//-----------------------------------------------

boost::posix_time::ptime getPTime(const std::string &val) try
{
    static constexpr size_t datePartSize = 10;

    if (val.empty())
        return boost::posix_time::ptime();

    std::string dateStr;
    std::string timeTzStr;
    boost::date_time::split(val, 'T', dateStr, timeTzStr);
    if (timeTzStr.empty()) {
        if (val.size() > datePartSize) {
            dateStr = val.substr(0, datePartSize);
            timeTzStr = StrUtils::replaceSubstrCopy(val.substr(datePartSize), " ", "");
        } else {
            return boost::posix_time::ptime();
        }
    }

    const boost::posix_time::ptime::date_type date = boost::date_time::parse_date<boost::posix_time::ptime::date_type>(dateStr);
    boost::char_separator<char> sep("-+Z", "-+Z", boost::keep_empty_tokens);
    boost::tokenizer<boost::char_separator<char>> tokens(timeTzStr, sep);
    const std::vector<std::string> arr(tokens.begin(), tokens.end());

    if (arr.empty())
        return boost::posix_time::ptime();

    boost::posix_time::ptime p(date, boost::date_time::parse_delimited_time_duration<boost::posix_time::time_duration>(arr[0]));

    // Due to the Timatic Application servers being hosted in London all dates and times are GMT.
    // The offset should be used if local times are required.
    if (arr.size() == 1)
        return p;

    if (arr.size() != 3)
        return boost::posix_time::ptime();

    if (arr[1] == "+")
        p -= boost::date_time::parse_delimited_time_duration<boost::posix_time::time_duration>(arr[2]);
    else if (arr[1] == "-")
        p += boost::date_time::parse_delimited_time_duration<boost::posix_time::time_duration>(arr[2]);

    return p;

} catch (const std::exception &) {
    return boost::posix_time::ptime();
}


/*
// TODO: need Boost 1.63
boost::posix_time::ptime getPTime(const std::string &val)
{
    if (!val.empty())
        return boost::posix_time::from_iso_extended_string(val);
    return boost::posix_time::not_a_date_time;
}
*/

} // Timatic
