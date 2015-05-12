#include <sstream>
#include <deque>
#include <boost/algorithm/string.hpp>
#include "app_interaction.h"
#include "exceptions.h"
#include "basic.h"

#define NICKNAME "ANNA"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;

static const std::string ReqTypeCirq = "CIRQ";
static const std::string ReqTypeCicx = "CICX";
static const std::string ReqTypeCimr = "CIMR";

static const std::string AnsTypeCirs = "CIRS";
static const std::string AnsTypeCicc = "CICC";
static const std::string AnsTypeCima = "CIMA";

static const std::pair<std::string, int> FltTypeChk("CHK", 2);
static const std::pair<std::string, int> FltTypeInt("INT", 8);
static const std::pair<std::string, int> FltTypeExo("EXO", 4);
static const std::pair<std::string, int> FltTypeExd("EXD", 4);
static const std::pair<std::string, int> FltTypeInm("INM", 3);

static const std::pair<std::string, int> PaxReqPrq("PRQ", 22);
static const std::pair<std::string, int> PaxReqPcx("PCX", 20);

static const std::pair<std::string, int> PaxAnsPrs("PRS", 27);
static const std::pair<std::string, int> PaxAnsPcc("PCC", 26);

static const std::pair<std::string, int> MftReqMrq("MRQ", 3);
static const std::pair<std::string, int> MftAnsMak("MAK", 4);

static const std::string AnsErrCode = "ERR";

static const int BasicFormatVer = 21;

TReqTrans::TReqTrans ( const std::string& trans_type, const std::string& id, const std::string& data )
{
  code = trans_type;
  airl_data = data;
  user_id = id;
  if (code == ReqTypeCirq) multi_resp = 'N';
  else multi_resp = 0;
  type = 0;
  ver = BasicFormatVer;
}

void TReqTrans::check_data()
{
  if (code != ReqTypeCirq && code != ReqTypeCicx && code != ReqTypeCimr)
    throw Exception("Incorrect transacion code");
  if (airl_data.size() > 14)
    throw Exception("Incorrect airl_data size");
  if (user_id.empty() || user_id.size() > 6)
    throw Exception("Incorrect User ID");
}

std::string TReqTrans::msg()
{
  check_data();
  std::ostringstream msg;
  msg << code << ':' << airl_data << '/' << user_id;
  if (code == ReqTypeCirq) {
    msg << '/' << multi_resp;
    if (type == 0)
      msg << '/' << type;
  }
  msg << '/' << ver;
  return msg.str();
}

TFlightData::TFlightData(std::pair<std::string, int> id, std::string flt, std::string airp_dep,
            TDateTime dep, std::string airp_arr, TDateTime arr)
{
  grp_id = id.first;
  flds_count = id.second;
  flt_num = flt;
  port = airp_dep;
  if (dep != NoExists) {
    date = DateTimeToStr( dep, "yyyymmdd" );
    time = DateTimeToStr( dep, "hhmmss" );
  }
  arr_port = airp_arr;
  if (dep != NoExists) {
    arr_date = DateTimeToStr( arr, "yyyymmdd" );
    arr_time = DateTimeToStr( arr, "hhmmss" );
  }
}

void TFlightData::check_data()
{
  if ( !( ( grp_id == FltTypeChk.first && flds_count == FltTypeChk.second ) || ( grp_id == FltTypeInt.first && flds_count == FltTypeInt.second ) ||
          ( grp_id == FltTypeExo.first && flds_count == FltTypeExo.second ) || ( grp_id == FltTypeExd.first && flds_count == FltTypeExd.second ) ||
          ( grp_id == FltTypeInm.first && flds_count == FltTypeInm.second ) ) )
    throw Exception( "Incorrect group identifier of flight or flight fields count" );
  if ( flt_num.empty() || flt_num.size() > 8 )
    throw Exception(" Incorrect flt_num" );
  if ( port.empty() || port.size() > 5 )
    throw Exception( "Incorrect airport" );
  if ( ( grp_id == FltTypeInt.first && arr_port.empty() ) || arr_port.size() > 5 )
    throw Exception( "Incorrect arrival airport" );
  if ( ( grp_id != FltTypeChk.first && date.empty() ) || date.size() > 8 )
    throw Exception( "Incorrect date format" );
  if ( grp_id != FltTypeChk.first && grp_id != FltTypeInm.first && ( time.empty() || time.size() > 6 ))
    throw Exception( "Incorrect time format" );
  if ( ( grp_id == FltTypeInt.first && arr_date.empty() ) || arr_date.size() > 8 )
    throw Exception( "Incorrect arrival date format" );
  if ( ( grp_id == FltTypeInt.first && arr_time.empty() ) || arr_time.size() > 6 )
    throw Exception( "Incorrect arrival time format" );
}

std::string TFlightData::msg()
{
  check_data();
  std::ostringstream msg;
  msg << grp_id << '/' << flds_count;
  if (grp_id == FltTypeInt.first)
    msg  << '/' << is_scheduled;
  if (grp_id == FltTypeInt.first || grp_id == FltTypeInm.first)
    msg << '/' << flt_num << '/' << port;
  else
    msg << '/' << port << '/' << flt_num;
  if (grp_id == FltTypeInt.first)
    msg  << '/' << arr_port;
  if (grp_id != FltTypeChk.first)
    msg  << '/' << date;
  if (grp_id != FltTypeChk.first && grp_id != FltTypeInm.first)
    msg  << '/' << time;
  if (grp_id == FltTypeInt.first) {
    msg << '/' << arr_date << '/' << arr_time;
  }
  return msg.str();
}

TReqPaxData::TReqPaxData(const int num, const bool is_crew, const CheckIn::TPaxDocItem& pax_doc, const int app_pax_id)
{
  if (app_pax_id) {
    grp_id = PaxReqPrq.first;
    flds_count = PaxReqPrq.second;
  }
  else {
    grp_id = PaxReqPcx.first;
    flds_count = PaxReqPcx.second;
  }
  seq_num = num;
  passenger_id = app_pax_id;
  pax_crew = is_crew?'C':'P';
  nationality = pax_doc.nationality;
  issuing_state = pax_doc.issue_country;
  passport = pax_doc.no;
  check_char = 0;
  if (pax_doc.type == "P")
    doc_type = 'P';
  else
    doc_type = 'O';
  expiry_date = DateTimeToStr( pax_doc.expiry_date, "yyyymmdd" );
  sup_check_char = 0;
  family_name = pax_doc.surname;
  given_names = (pax_doc.first_name + pax_doc.second_name).substr(0, 24);
  birth_date = DateTimeToStr( pax_doc.birth_date, "yyyymmdd" );
  if (pax_doc.gender == "M")
    sex = 'M';
  else if (pax_doc.gender == "F")
    sex = 'F';
  else
    sex = 'U';
  endorsee = 0;
  trfer_at_origin = 0;
  trfer_at_dest = 0;
}

void TReqPaxData::check_data()
{
  if (grp_id != PaxReqPrq.first && grp_id != PaxReqPcx.first)
    throw Exception("Incorrect grp_id");
  if (flds_count != PaxReqPrq.second && flds_count != PaxReqPcx.second)
    throw Exception("Incorrect grp_id");
  if (seq_num == 0 || (grp_id == PaxReqPrq.first && seq_num > 5) ||
      (grp_id == PaxReqPcx.first && seq_num > 10))
    throw Exception("Incorrect seq_num");
  if (grp_id == PaxReqPcx.first && passenger_id == 0)
    throw Exception("Incorrect seq_num");
  if (pax_crew != 'C' && pax_crew != 'P' )
    throw Exception("Incorrect pax_crew");
  if (nationality.empty())
    throw Exception("Empty nationality");
  if (grp_id == PaxReqPrq.first && issuing_state.empty())
    throw Exception("Empty issuing_state");
  if(passport.empty())
    throw Exception("Empty passport");
  if(doc_type != 0 && doc_type != 'P' && doc_type != 'O' && doc_type != 'N')
    throw Exception("Incorrect doc_type");
  if(expiry_date.empty())
    throw Exception("Empty expiry_date");
  if(family_name.empty() || family_name.size() < 2 || family_name.size() > 24)
    throw Exception("Incorrect family_name");
  if(given_names.size() > 24)
    throw Exception("Incorrect given_names");
  if(birth_date.empty())
    throw Exception("Empty birth_date");
  if(sex != 0 && sex != 'M' && sex != 'F' && sex != 'U' && sex != 'X')
    throw Exception("Incorrect sex");
  if(endorsee != 0 && endorsee != 'S')
    throw Exception("Incorrect endorsee");
}

std::string TReqPaxData::msg()
{
  check_data();
  std::ostringstream msg;
  msg << grp_id << '/' << flds_count << '/' << seq_num;
  if (grp_id == PaxReqPcx.first)
    msg  << '/' << passenger_id;
  msg << '/' << pax_crew << '/' << nationality << '/' << issuing_state << '/' << passport <<
         '/' << check_char << '/' << doc_type << '/' << expiry_date << '/' << sup_doc_type <<
         '/' << sup_passport << '/' << sup_check_char << '/' << family_name << '/';
  if (!given_names.empty()) msg << given_names << '/';
  else msg << "-/";
  msg << birth_date << '/' << sex << '/' << birth_country << '/' << endorsee << '/' <<
         trfer_at_origin << '/' << trfer_at_dest;
  if (grp_id == PaxReqPrq.first)
    msg  << '/' << override_codes << '/' << pnr_source << '/' << pnr_locator;
  return msg.str();
}

TReqMft::TReqMft(std::string state, char pax_req, char crew_req)
{
  grp_id = MftReqMrq.first;
  flds_count = MftReqMrq.second;
  country = state;
  mft_pax = pax_req;
  mft_crew = crew_req;
}

void TReqMft::check_data()
{
  if (grp_id != MftReqMrq.first)
    throw Exception("Incorrect grp_id");
  if (flds_count != MftReqMrq.second)
    throw Exception("Incorrect flds_count");
  if (country.empty())
    throw Exception("Empty country");
  if (mft_pax != 'C')
    throw Exception("Incorrect mft_pax");
  if (mft_crew != 'C')
    throw Exception("Incorrect mft_crew");
}

std::string TReqMft::msg()
{
  check_data();
  std::ostringstream msg;
  msg << grp_id << '/' << flds_count << '/' << country << '/' << mft_pax << '/' << mft_crew;
  return msg.str();
}

std::string APPRequest::ComposeMsg()
{
  std::ostringstream msg;
  msg << trans.msg() << '/';
  if (trans.code == ReqTypeCirq && !ckin_flt.empty())
    msg << ckin_flt.msg() << '/';
  if (trans.code == ReqTypeCirq || trans.code == ReqTypeCicx) {
    if (int_flt.empty())
      throw Exception("Internation flight is empty!");
    tst();
    msg << int_flt.msg() << '/';
  }
  if (trans.code == ReqTypeCirq && !exp_flt.empty()) {
    for (std::vector<TFlightData>::iterator it = exp_flt.begin(); it != exp_flt.end(); it++) {
      msg << it->msg() << '/';
    }
  }
  if (trans.code == ReqTypeCirq || trans.code == ReqTypeCicx) {
    if (passengers.empty())
      throw Exception("Passengers is empty!");
    for (std::vector<TReqPaxData>::iterator it = passengers.begin(); it != passengers.end(); it++) {
      msg << it->msg() << '/';
    }
  }
  if (trans.code == ReqTypeCimr)
    msg << inm_flt.msg() << '/' << mft_req.msg() << '/';
  return msg.str();
}

std::string TAnsTrans::toString()
{
  std::string res = "code: " + code + "\n" + "airl_data: " + airl_data + "\n";
  return res;
}

std::string TAnsPaxData::toString()
{
  std::ostringstream res;
  res << "grp_id: " << grp_id << std::endl << "flds_count: " << flds_count << std::endl <<
         "seq_num: " << seq_num << std::endl << "country: " << country << std::endl <<
         "pax_crew: " << pax_crew << std::endl << "nationality: " << nationality << std::endl;
  if(!issuing_state.empty()) res << "issuing_state: " << issuing_state << std::endl;
  if(!passport.empty()) res << "passport: " << passport << std::endl;
  if(check_char != 0) res << "check_char: " << check_char << std::endl;
  if(doc_type != 0) res << "doc_type: " << doc_type << std::endl;
  if(!expiry_date.empty()) res << "expiry_date: " << expiry_date << std::endl;
  if(!sup_doc_type.empty()) res << "sup_doc_type: " << sup_doc_type << std::endl;
  if(!sup_passport.empty()) res << "sup_passport: " << sup_passport << std::endl;
  if(sup_check_char != 0) res << "sup_check_char: " << sup_check_char << std::endl;
  if(!family_name.empty()) res << "family_name: " << family_name << std::endl;
  if(!given_names.empty()) res << "given_names: " << given_names << std::endl;
  if(!birth_date.empty()) res << "birth_date: " << birth_date << std::endl;
  if(sex != 0) res << "sex: " << sex << std::endl;
  if(!birth_country.empty()) res << "birth_country: " << birth_country << std::endl;
  if(endorsee != 0) res << "endorsee: " << endorsee << std::endl;
  res << "ckin_code: " << ckin_code << std::endl << "ckin_status: " << ckin_status << std::endl;
  if(passenger_id != 0) res << "passenger_id: " << passenger_id << std::endl;
  if(error_code1 != 0) res << "error_code1: " << error_code1 << std::endl;
  if(!error_text1.empty()) res << "error_text1: " << error_text1 << std::endl;
  if(error_code2 != 0) res << "error_code2: " << error_code2 << std::endl;
  if(!error_text2.empty()) res << "error_text2: " << error_text2 << std::endl;
  if(error_code3 != 0) res << "error_code3: " << error_code3 << std::endl;
  if(!error_text3.empty()) res << "error_text3: " << error_text3 << std::endl;
  return res.str();
}

std::string TAnsError::toString()
{
  if(grp_id.empty())
    return std::string("");
  std::ostringstream res;
  res << "flds_count: " << flds_count << std::endl;
  for (std::vector<TError>::iterator it = errors.begin(); it != errors.end(); it++)
    res << "country: " << it->country << std::endl << "error_code: " << it->error_code << std::endl <<
           "error_text: " << it->error_text << std::endl;
  return res.str();
}

std::string TAnsMft::toString()
{
  if(grp_id.empty())
    return std::string("");
  std::ostringstream res;
  res << "flds_count: " << flds_count << std::endl << "country: " << country << std::endl <<
         "resp_code: " << resp_code << std::endl;
  if (error_code != 0)
    res << "error_code: " << error_code << std::endl << "error_text: " << error_text << std::endl;
  return res.str();
}

void APPAnswer::parse()
{
  if(source.empty())
    throw Exception("Answer is empty");

  ProgTrace(TRACE5, "APP answer: %s", source.c_str());
  std::vector<std::string> temp;
  trans.code = source.substr(0, 4);
  source = source.substr(5, source.size() - 6); // отрезаем код транзакции и замыкающий '/' (XXXX:text_to_parse/)
  boost::split(temp, source, boost::is_any_of("/"));

  std::vector<std::string>::iterator it = temp.begin();
  if (trans.code != AnsTypeCirs && trans.code != AnsTypeCicc && trans.code != AnsTypeCima)
    throw Exception( std::string( "Unknown transaction code: " + trans.code ) );
  trans.airl_data = *(it++);
  while(it < temp.end() && *it != AnsErrCode) {
    if ((trans.code == AnsTypeCirs && *it == PaxAnsPrs.first)
        || (trans.code == AnsTypeCicc && *it == PaxAnsPcc.first)) {
      TAnsPaxData pax;
      pax.grp_id = *(it++);
      pax.flds_count = toInt(*(it++));
      pax.seq_num = toInt(*(it++));
      pax.country = *(it++);
      pax.pax_crew = (*(it++)).front();
      pax.nationality = *(it++);
      pax.issuing_state = *(it++);
      pax.passport = *(it++);
      pax.check_char = (*(it++)).front();
      pax.doc_type = (*(it++)).front();
      pax.expiry_date = *(it++);
      pax.sup_doc_type = (*(it++));
      pax.sup_passport = *(it++);
      pax.sup_check_char = (*(it++)).front();
      pax.family_name = *(it++);
      if (*it != "-") pax.given_names = *(it++);
      pax.birth_date = *(it++);
      pax.sex = (*(it++)).front();
      pax.birth_country = *(it++);
      pax.endorsee = (*(it++)).front();
      pax.ckin_code = toInt(*(it++));
      pax.ckin_status = (*(it++)).front();
      if (trans.code == AnsTypeCirs)
        pax.passenger_id = toInt(*(it++));
      pax.error_code1 = toInt(*(it++));
      pax.error_text1 = *(it++);
      pax.error_code2 = toInt(*(it++));
      pax.error_text2= *(it++);
      pax.error_code3 = toInt(*(it++));
      pax.error_text3 = *(it++);
      passengers.push_back(pax);
    }
    else if (trans.code == AnsTypeCima && *it == MftAnsMak.first) {
      mft_grp.grp_id = *(it++);
      mft_grp.flds_count = toInt(*(it++));
      mft_grp.country = *(it++);
      mft_grp.resp_code = toInt(*(it++));
      mft_grp.error_code = toInt(*(it++));
      mft_grp.error_text = *(it++);
    }
    else
      throw Exception( std::string( "Unknown grp_id: " ) + trans.code + " or " + *it );
  }
  if(it >= temp.end()) return;
  err_grp.grp_id = *(it++);
  err_grp.flds_count = toInt(*(it++));
  while(it < temp.end()) {
    TError error;
    error.country = *(it++);
    error.error_code = toInt(*(it++));
    error.error_text = *(it++);
    err_grp.errors.push_back(error);
  }
}

std::string APPAnswer::toString()
{
  std::ostringstream res;
  res << trans.toString();
  for (std::vector<TAnsPaxData>::iterator it = passengers.begin(); it != passengers.end(); it++)
    res << it->toString();
  res << mft_grp.toString() << err_grp.toString();
  return res.str();
}

int test_app_interaction(int argc,char **argv)
{
  std::deque<std::string> examples;
  examples.push_back("CIRS:/PRS/28/001/AU/P/USA/USA/Z4351213//P/20011114////SMITH/SAMANTHA/19640912/F/USA//8501/B/2128666///////");
  examples.push_back("CIRS:/PRS/28/001/AU/P/USA//Z4351213//P/////SMIT//////8502/D////////");
  examples.push_back("CIRS:111/PRS/28/001/MY/P/USA/USA/Z4351213//P/20011114////SAMANTHA TAYLOR SMITH//19640912/F/USA//8501/B/18743///////PRS/28/001/AU/P/USA/USA/Z4351213//P/20011114////SMITH/SAMANTHA/19640912/F/USA//8501/B/18743///////");
  examples.push_back("CIRS:/PRS/28/001/AU/P/USA/USA/Z4351213//P/20011114////SMITH/SAMANTHA/19640912/F/USA//8507/U////////PRS/28/001/AU/P/USA/USA/Z4351213//P/20011114////SMITH/BELINDA/19940421/F/USA//8507/U////////");
  examples.push_back("CIRS:/PRS/28/001/AU/P/USA//351213//P/////SMIT//////0000/E//6033/INVALID PASSPORT NUMBER FORMAT/////");
  examples.push_back("CIRS:/ERR/3/AU/6999/AP ERROR: PL-SQL FAILED/");
  examples.push_back("CIRS:NZ1123/PRS/28/001/AU/P/USA/USA/Z4351213/E/P/20011114////SMITH/SAMANTHA/19640912/F/USA//8501/B/2128666///////");
  examples.push_back("CIRS:/PRS/28/001/AU/P/USA/USA/Z4351213//P/20011114////SMITH/SAMANTHA/19640912/F/USA//8501/B/345234///////PRS/28/002/AU/P/USA/USA/Z4354753//P/20011114////SMITH/ANDREW JACKSON/19620511/M/USA//8501/B/345235///////");
  examples.push_back("CIRS:/PRS/28/001/AU/P/USA/USA/Z4351213//P/20011114////SMITH/SAMANTHA/19640912/F/USA//8501/B/345234///////PRS/28/002/AU/P/USA//Z43547//P/////SMIT//////0000/E//6033/INVALID PASSPORT NUMBER FORMAT/////");
  examples.push_back("CIRS:/PRS/28/001/AU/P/NZL//Z4351213///////SMIT//////8507/U////////PRS/28/002/AU/P/NZL//Z4354888//P/20011114////SMITH/SAMUEL JACKSON/19320721/M/NZL//8501/B/345236///////");
  examples.push_back("CIRS:111/PRS/28/001/MY/P/USA/USA/Z4351213//P/20011114////SAMANTHA TAYLOR SMITH//19640912/F/USA//8501/B////////PRS/28/001/AU/P/USA/USA/Z4351213//P/20011114////SMITH/SAMANTHA/19640912/F/USA//8502/D////////");
  examples.push_back("CIRS:111/PRS/28/001/MY/P/USA/USA/Z4351213//P/20011114////SAMANTHA TAYLOR SMITH//19640912/F/USA//8501/B/18743///////PRS/28/001/AU/P/USA/USA/Z4351213//P/20011114////SMITH/SAMANTHA/19640912/F/USA//8501/B/18743///////PRS/28/002/MY/P/USA/USA/Z4354753//P/20011114////SMITH/ANDREW JACKSON/19620511/M/USA//8502/D////////PRS/28/002/AU/P/USA/USA/Z4354753//P/20011114////SMITH/ANDREW JACKSON/19620511/M/USA//8502/D////////");
  examples.push_back("CIRS:111/PRS/28/001/MY/P/USA/USA/Z4354753//P/20011114////SMITH/ANDREW JACKSON/19620511/M/USA//8517/B/18777///////PRS/28/001/AU/P/USA/USA/Z4354753//P/20011114////SMITH/ANDREWJACKSON/19620511/M/USA//8517/B/18777///////");
  examples.push_back("CIRS:111/PRS/28/001/MY/P/USA/USA/Z4351213//P/20011114////SAMANTHA TAYLOR SMITH//19640912/F/USA//8501/B/2129999///////PRS/28/001/AU/P/USA//Z4351213//P///////////0000/T////////");
  examples.push_back("CIRS:111/ERR/3//5057/NO RESPONSE FROM ETA-APP SYSTEM. PLEASE TRY AGAIN LATER.");
  examples.push_back("CIRS:111/PRS/28/001/AU/P/USA//Z4351213//P///////////0000/E//6979/NO RESPONSE FROM GOVERNMENT SYSTEM. PLEASE TRY AGAIN LATER./////");
  examples.push_back("CICC:/PCC/26/001/AU/P/USA/USA/Z4351213//P/////SMITH/SAMANTHA/19640912/F/USA//8505/C///////");
  examples.push_back("CICC:/PCC/26/001/AU/P/USA//Z4351213//P/////SMIT//////8506/N///////");
  examples.push_back("CICC:/PCC/26/001/AU/P/USA//Z43512//P/////SMIT//////0000/E/6033/INVALID PASSPORT NUMBER FORMAT/////");
  examples.push_back("CICC:/PCC/26/001/AU/P/USA//Z4351213//P/////SMITH/SAMANTHA/19640912/F/USA//8505/C///////PCC/26/002/AU/P/USA//Z4351213//P/////SMITH/BELINDA/19940421/F/USA//8505/C///////");
  examples.push_back("CIMA:/MAK/4/US/8700///");

  for(std::deque<std::string>::iterator it = examples.begin(); it != examples.end(); it++) {
    APPAnswer ans(*it);
    try {
      ans.parse();
    }
    catch (Exception &e) {
      ProgTrace( TRACE5, "%s", ans.toString().c_str() );
      throw e;
    }
    ProgTrace( TRACE5, "%s", ans.toString().c_str() );
  }

  APPRequest req;
  req.trans.code = ReqTypeCirq;
  req.trans.ver = BasicFormatVer;
  req.trans.user_id = "123456";
  req.trans.multi_resp = 'N';
  req.int_flt.grp_id = FltTypeInt.first;
  req.int_flt.flds_count =FltTypeInt.second;
  req.int_flt.is_scheduled = 'S';
  req.int_flt.flt_num = "BA033";
  req.int_flt.port = "KUL";
  req.int_flt.arr_port = "SYD";
  req.int_flt.date = "19981030";
  req.int_flt.time = "1400";
  req.int_flt.arr_date = "19981030";
  req.int_flt.arr_time = "2300";
  TReqPaxData pax;
  pax.grp_id = PaxReqPrq.first;
  pax.flds_count = PaxReqPrq.second;
  pax.seq_num = 1;
  pax.pax_crew = 'P';
  pax.nationality = "USA";
  pax.issuing_state = "USA";
  pax.passport = "351213";
  pax.expiry_date = "20170315";
  pax.family_name = "SMIT";
  pax.given_names = "SAMANTA";
  pax.birth_date = "19800213";
  req.passengers.push_back(pax);
  std::cout << req.ComposeMsg() << std::endl;
  return 0;
}

int toInt(const std::string& str) {
  if(str.empty())
    return 0;
  try {
    return std::stoi(str);
  }
  catch(std::exception &e) {
    std::cout << e.what() << str << std::endl;
    throw e;
  }
}
