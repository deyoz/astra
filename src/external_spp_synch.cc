#include "external_spp_synch.h"
#include "exceptions.h"
#include "points.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace EXCEPTIONS;
using namespace std;
using namespace BASIC;

void TParseFlight::add_airline( const std::string &value ) {
  try {
    airline.code = ElemToElemId( etAirline, value, airline.fmt, false );
    if ( airline.fmt == efmtUnknown )
      throw EConvertError("");
      if ( airline.fmt == efmtCodeInter || airline.fmt == efmtCodeICAOInter )
          trip_type = "м";  //!!!vlad а правильно ли так определять тип рейса? не уверен. Проверка при помощи маршрута. Если в маршруте все п.п. принадлежат одной стране то "п" иначе "м"
    else
      trip_type = "п";
  }
  catch( EConvertError &e ) {
      throw Exception( "Неизвестная авиакомпания, значение=%s", value.c_str() );
  }
  if ( !fltNo.empty() ) {
     add_fltno( fltNo );
  }
}

void TParseFlight::add_fltno( const std::string &value ) {
  fltNo = value;
  if ( airline.code.empty() ) {
    return;
  }
  fltNo = airline.code + fltNo;
  string tmp_airline;
  parseFlt( fltNo, tmp_airline, flt_no, suffix.code );
  if ( !suffix.code.empty() ) {
    try {
      suffix.code = ElemToElemId( etSuffix, suffix.code, suffix.fmt, false );
      if ( suffix.fmt == efmtUnknown )
       throw EConvertError("");
    }
    catch( EConvertError &e ) {
        throw Exception( "Ошибка формата номера рейса, значение=%s", suffix.code.c_str() );
    }
  }
}

void TParseFlight::add_scd( const std::string &value ) {
  if ( value.empty() )
        throw Exception( "Ошибка формата планового времени, значение=%s", value.c_str() );
  std::string tmp_value = value;
  if ( tmp_value.size() == 23 ) {
    tmp_value = tmp_value.substr(0, 19); //отсекаем миллисекунды
  }
  if ( StrToDateTime( tmp_value.c_str(), FormatFlightDateTime.c_str(), scd ) == EOF )
        throw Exception( "Ошибка формата планового времени, значение=%s", value.c_str() );
  try {
      scd = LocalToUTC( scd, own_region );
    }
    catch( boost::local_time::ambiguous_result ) {
        throw Exception( "Плановое время выполнения рейса определено не однозначно" );
  }
  catch( boost::local_time::time_label_invalid ) {
    throw Exception( "Плановое время выполнения рейса не существует" );
  }
}

void TParseFlight::add_est( const std::string &value ) {
  if ( value.empty() )
        throw Exception( "Ошибка формата расчетного времени, значение=%s", value.c_str() );
  std::string tmp_value = value;
  if ( tmp_value.size() == 23 ) {
    tmp_value = tmp_value.substr(0, 19); //отсекаем миллисекунды
  }
  if ( StrToDateTime( tmp_value.c_str(), FormatFlightDateTime.c_str(), est ) == EOF )
        throw Exception( "Ошибка формата расчетного времени, значение=%s", value.c_str() );
  try {
      est = LocalToUTC( est, own_region );
    }
    catch( boost::local_time::ambiguous_result ) {
        throw Exception( "Расчетное время выполнения рейса определено не однозначно" );
  }
  catch( boost::local_time::time_label_invalid ) {
    throw Exception( "Расчетное время выполнения рейса не существует" );
  }
}

void TParseFlight::add_craft( const std::string &value ) {
    craft.code = value;
  bool pr_craft_error = true;
  if ( !craft.code.empty() ) {
      try {
      craft.code = ElemCtxtToElemId( ecDisp, etCraft, craft.code, craft.fmt, false );
      pr_craft_error = false;
    }
    catch( EConvertError &e ) {
      craft.code.clear();
    }
  }
}

void TParseFlight::add_dests( const std::string &value ) {
  typedef boost::char_separator<char> token_func_type;
  typedef boost::tokenizer<token_func_type> tokenizer_type;
  token_func_type sep("-");
  tokenizer_type tok(value, sep);
  bool pr_own = false;
  for ( tokenizer_type::iterator itok=tok.begin(); itok!=tok.end(); itok++ ) {
    if ( *itok == own_airp ) {
      pr_own = true;
      continue;
    }
    TCode airp;
    try {
       airp.code = ElemCtxtToElemId( ecDisp, etAirp, *itok, airp.fmt, false );
       if ( airp.fmt == efmtUnknown ) {
         throw EConvertError( "" );
       }
    }
    catch( EConvertError &e ) {
      throw Exception( "Неизвестный код аэропорта, значение=%s", value.c_str() );
    }
    if ( pr_own ) {
      airps_out.push_back( airp );
    }
    else {
      airps_in.push_back( airp );
    }
  }
  if ( airps_in.size() + airps_out.size() < 1 ) {
    throw Exception( "Маршрут имеет менее 2-х аэропортов %i", airps_in.size() + airps_out.size() );
  }
}

TParseFlight& TParseFlight::operator << (const FlightProperty &prop) {
  if ( !error.empty() ) {
    return *this;
  }
  try {
    if ( prop.name == "AK" ) {
      add_airline( prop.value );
      return *this;
    }
    if ( prop.name == "NREIS" ) {
      add_fltno( prop.value );
      return *this;
    }
    if ( prop.name == "RASPDATETIME" ) {
      add_scd( prop.value );
      return *this;
    }
    if ( prop.name == "FACTDATETIME" ) {
      add_est( prop.value );
      return *this;
    }
    if ( prop.name == "TYPEVS" ) {
      add_craft( prop.value );
      return *this;
    }
    if ( prop.name == "MARSHRUT" ) {
      add_dests( prop.value );
      return *this;
    }
    if ( prop.name == "REISSTATE" ) {
      add_status( prop.value );
      return *this;
    }
    if ( prop.name == "OP" ) {
      add_prlanding( prop.value );
      return *this;
    }
  }
  catch( Exception &e ) {
    error = string( "property " ) + prop.name + ", value " + prop.value + " " + e.what();
    ProgTrace( TRACE5, "error=%s", error.c_str() );
  }
  return *this;
}

void TParseFlight::clear() {
  error.clear();
  airline.clear();
  fltNo.clear();
  flt_no = ASTRA::NoExists;
  suffix.clear();
  trip_type.clear();
  scd = ASTRA::NoExists;
  est = ASTRA::NoExists;
  craft.clear();
  airps_in.clear();
  airps_out.clear();
  pr_landing = false;
  status.clear();
  record.clear();
};

struct AnswerContent {
   std::string airline;
   int rec_no;
   std::string msg;
   std::string type;
   TDateTime time;
};

void HTTPRequestsIface::SaveSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace( TRACE5, "SaveSPP: desk=%s, airp=%s", TReqInfo::Instance()->desk.code.c_str(), TReqInfo::Instance()->desk.airp.c_str() );
  xmlNodePtr contentNode = GetNode( "content", reqNode );
  if ( contentNode == NULL ) {
    return;
  }
  TDateTime filter_scd;
  modf(UTCToLocal( NowUTC(), TReqInfo::Instance()->desk.tz_region ),&filter_scd);
  TQuery sQry( &OraSession );
  sQry.SQLText =
    "BEGIN "
    " UPDATE aodb_spp_files SET rec_no=NVL(rec_no,-1)+1 "
    "  WHERE filename=:filename AND point_addr=:point_addr AND airline=:airline;"
    " IF SQL%NOTFOUND THEN "
    "   INSERT INTO aodb_spp_files(filename,point_addr,rec_no,airline) "
    "    VALUES(:filename,:point_addr,0,:airline); "
    " END IF;"
    "END;";
  sQry.CreateVariable( "filename", otString, DateTimeToStr( filter_scd, "SPPyymmdd.txt" ) );
  sQry.CreateVariable( "point_addr", otString, TReqInfo::Instance()->desk.code );
  sQry.DeclareVariable( "airline", otString );
  TQuery eQry( &OraSession );
  eQry.SQLText =
    " INSERT INTO aodb_events(filename,point_addr,rec_no,record,msg,type,time,airline) "
    "  VALUES(:filename,:point_addr,:rec_no,:record,:msg,:type,:time,:airline)";
  eQry.CreateVariable( "filename", otString, DateTimeToStr( filter_scd, "SPPyymmdd.txt" ) );
  eQry.CreateVariable( "point_addr", otString, TReqInfo::Instance()->desk.code );
  eQry.DeclareVariable( "rec_no", otInteger );
  eQry.DeclareVariable( "record", otString );
  eQry.DeclareVariable( "msg", otString );
  eQry.DeclareVariable( "type", otString );
  eQry.DeclareVariable( "time", otDate );
  eQry.DeclareVariable( "airline", otString );

  string buffer = NodeAsString( contentNode );
  typedef boost::char_separator<char> token_func_type;
  typedef boost::tokenizer<token_func_type> tokenizer_type;
  //typedef std::vector<std::string> token_vector_type;
  token_func_type sep("\n");
  token_func_type fields_sep("\t");
  tokenizer_type tok(buffer, sep);
  std::vector<std::string> fields, values;

  typedef std::vector<std::string> splitted_vector_type;
  splitted_vector_type split_vec;

  std::map<std::string,map<bool, TParseFlight> > flights;
  std::map<std::string,AnswerContent> content;
  int rec_no = 0;
  for ( tokenizer_type::iterator itok=tok.begin(); itok!=tok.end(); itok++ ) {
     string line_buffer = boost::algorithm::trim_copy(*itok);
     if ( line_buffer.empty() ) {
       continue;
     }
    boost::algorithm::split(split_vec, line_buffer, boost::is_any_of("\t"));
    string val;
    if ( itok == tok.begin() ) {  //fields name
       std::copy (
            split_vec.begin(),
            split_vec.end(),
            std::back_inserter<splitted_vector_type>(fields) );
    }
    else {
      content[ line_buffer ].msg = " -parse error";
      content[ line_buffer ].rec_no = rec_no;
      rec_no++;
      values.clear();
      std::copy (
           split_vec.begin(),
           split_vec.end(),
           std::back_inserter<splitted_vector_type>(values) );
    }
    if ( fields.size() != values.size() ) {
      ProgTrace( TRACE5, ">>>fields.size(%zu) != values.size(%zu)", fields.size(), values.size() );
      continue;
    }
    TParseFlight flight( TReqInfo::Instance()->desk.airp );
    flight.record = line_buffer;
    for ( std::vector<std::string>::iterator ifield=fields.begin(), ivalue=values.begin();
         ifield!=fields.end(), ivalue!=values.end(); ifield++, ivalue++ ) {
       flight<<FlightProperty( *ifield, *ivalue );
    }

    if ( flight.scd != ASTRA::NoExists ) {
      if ( flight.scd == flight.est &&
           ( ( flight.pr_landing && flight.status != "ПРИЛЕТЕЛ" ) ||
             ( !flight.pr_landing && flight.status != "ВЫЛЕТЕЛ" ) ) ) {
        flight.est = ASTRA::NoExists;
      }
    }
    if ( flight.is_valid() ) {
      if ( flight.scd <= filter_scd - 1 || flight.scd > filter_scd + 2 ) {        
        flight.error = "scd not in SPP";
        ProgTrace( TRACE5, "record=%s, error=%s", flight.record.c_str(), flight.error.c_str() );
      }
    }
    if ( flight.is_valid() ) {
      ProgTrace( TRACE5, "flights Add airline=%s, flt_no=%d, scd=%f, pr_landing=%d",
                 flight.airline.code.c_str(),
                 flight.flt_no,
                 flight.scd,
                 flight.pr_landing );

      try {
        flights[ flight.key() ].insert( make_pair( flight.pr_landing, flight ) );
      }
      catch( ... ) { //double flight
        flight.error = "double flight";
        ProgTrace( TRACE5, ">>> double flight %s", flight.key().c_str() );
      }
    }
    if ( !flight.is_valid() ) {
      content[ flight.record ].msg = " -parse error " + flight.error;
      content[ flight.record ].type = EncodeEventType( ASTRA::evtProgError );
    }
    else {
      content[ flight.record ].airline = flight.airline.code;
    }
  }
  saveFlights( flights );
  for ( std::map<std::string,map<bool, TParseFlight> >::iterator iflight = flights.begin();
        iflight != flights.end(); iflight ++ ) {

    map<bool, TParseFlight>::iterator fl_in = iflight->second.find( true );

    if ( fl_in != iflight->second.end() ) {
      if ( fl_in->second.error.empty() ) {
        content[ fl_in->second.record ].msg = " land data not used";
      }
      else {
        content[ fl_in->second.record ].msg = " " + fl_in->second.error;
      }
      content[ fl_in->second.record ].time = NowUTC();
      content[ fl_in->second.record ].type = fl_in->second.error == EncodeEventType( ASTRA::evtFlt )?fl_in->second.error:EncodeEventType( ASTRA::evtProgError );
    }
    map<bool, TParseFlight>::iterator fl_out = iflight->second.find( false );
    if ( fl_out != iflight->second.end() ) {
      if ( fl_out->second.error.empty() ) {
        content[ fl_out->second.record ].msg = " takeoff data not used";
      }
      else {
        content[ fl_out->second.record ].msg = " " + fl_out->second.error;
      }
      content[ fl_out->second.record ].time = NowUTC();
      content[ fl_out->second.record ].type = fl_out->second.error == EncodeEventType( ASTRA::evtFlt )?fl_out->second.error:EncodeEventType( ASTRA::evtProgError );
    }    
  }
  string res = "\n";
  for ( std::map<std::string,AnswerContent>::iterator istr=content.begin(); istr!=content.end(); istr++ ) {
    ProgTrace( TRACE5, "record=%s, result=%s", istr->first.c_str(), istr->second.msg.c_str() );
    res += istr->first + istr->second.msg + "\n";
    sQry.SetVariable( "airline", istr->second.airline.empty()?"ЮТ":istr->second.airline );
    sQry.Execute();
    eQry.SetVariable( "rec_no", istr->second.rec_no );
    eQry.SetVariable( "record", istr->first );
    eQry.SetVariable( "msg", istr->second.msg.substr(0,1000) );
    eQry.SetVariable( "type", istr->second.type );
    eQry.SetVariable( "time", istr->second.time );
    eQry.SetVariable( "airline", istr->second.airline.empty()?"ЮТ":istr->second.airline );
    eQry.Execute();
  }
  NodeSetContent( resNode, res );
//  ProgTrace( TRACE5, "finish, context=%s", content.c_str() );
}

void saveFlights( std::map<std::string,map<bool, TParseFlight> > &flights )
{
  ProgTrace( TRACE5, "saveFlights" );
  TQuery uQry( &OraSession );
  uQry.SQLText =
    "SELECT p.point_id, point_num FROM points p, aodb_points a"
    " WHERE p.point_id=a.point_id AND p.move_id=:move_id AND airp=:airp AND pr_del<>-1"
    " ORDER BY point_num";
  uQry.DeclareVariable( "move_id", otInteger );
  uQry.CreateVariable( "airp", otString, TReqInfo::Instance()->desk.airp );
  TQuery pQry( &OraSession );
  pQry.SQLText =
   "BEGIN "
   " UPDATE aodb_points SET rec_no_flt=NVL(rec_no_flt,-1)+1 "
   "  WHERE point_id=:point_id AND point_addr=:point_addr; "
   "  IF SQL%NOTFOUND THEN "
   "    INSERT INTO aodb_points(point_id,point_addr,aodb_point_id,rec_no_flt,rec_no_pax,rec_no_bag,rec_no_unaccomp) "
   "      VALUES(:point_id,:point_addr,NULL,0,-1,-1,-1); "
   "  END IF; "
   "END;";
  pQry.DeclareVariable( "point_id", otInteger );
  pQry.CreateVariable( "point_addr", otString, TReqInfo::Instance()->desk.code );
  for ( std::map<std::string,map<bool, TParseFlight> >::iterator iflight = flights.begin();
        iflight != flights.end(); iflight ++ ) {
    TPoints points;
    std::string airline;
    int flt_no;
    std::string suffix;
    std::string airp;
    BASIC::TDateTime scd_in;
    BASIC::TDateTime scd_out;
    bool pr_land = false, pr_takeoff= false;
    int doubleMove_id = ASTRA::NoExists;
    int doublePoint_id = ASTRA::NoExists;
    vector<TCode> airps;
    map<bool, TParseFlight>::iterator fl_in = iflight->second.find( true );
    map<bool, TParseFlight>::iterator fl_out = iflight->second.find( false );
    try {
      if ( fl_in != iflight->second.end() && fl_in->second.is_valid() ) {
        airps.clear();
        airline = fl_in->second.airline.code;
        flt_no = fl_in->second.flt_no;
        suffix = fl_in->second.suffix.code;
        airp = fl_in->second.own_airp;
        scd_in = fl_in->second.scd;
        airps.insert( airps.end(), fl_in->second.airps_in.begin(), fl_in->second.airps_in.end() );
        TCode c;
        c.code = ElemCtxtToElemId( ecDisp, etAirp, fl_in->second.own_airp, c.fmt, false );
        airps.push_back( c );
        airps.insert( airps.end(), fl_in->second.airps_out.begin(), fl_in->second.airps_out.end() );
  /*      for ( vector<TCode>::iterator iairp= airps.begin(); iairp!= airps.end(); iairp++ ) {*/
          pr_land = points.isDouble( ASTRA::NoExists, airline, flt_no, suffix, airp, scd_in, ASTRA::NoExists, doubleMove_id, doublePoint_id );
          ProgTrace( TRACE5, "fl_in found: airline=%s, flt_no=%d, airp=%s, scd_in=%s, pr_land=%d",
                     airline.c_str(), flt_no, airp.c_str(), DateTimeToStr( scd_in ).c_str(), pr_land );
  /*        if ( pr_land ) {
            tst();
            break;
          }
        }*/
      }
      if ( fl_out != iflight->second.end() && fl_out->second.is_valid() ) {
        airps.clear();
        airline = fl_out->second.airline.code;
        flt_no = fl_out->second.flt_no;
        suffix = fl_out->second.suffix.code;
        airp = fl_out->second.own_airp;
        scd_out = fl_out->second.scd;
        airps.insert( airps.end(), fl_out->second.airps_in.begin(), fl_out->second.airps_in.end() );
        TCode c;
        c.code = ElemCtxtToElemId( ecDisp, etAirp, fl_out->second.own_airp, c.fmt, false );
        airps.push_back( c );
        airps.insert( airps.end(), fl_out->second.airps_out.begin(), fl_out->second.airps_out.end() );
        /*for ( vector<TCode>::iterator iairp= airps.begin(); iairp!= airps.end(); iairp++ ) {*/
        pr_takeoff = points.isDouble( ASTRA::NoExists, airline, flt_no, suffix, airp, ASTRA::NoExists, scd_out, doubleMove_id, doublePoint_id );
        ProgTrace( TRACE5, "fl_out found: airline=%s, flt_no=%d, airp=%s, scd_out=%s, pr_takeoff=%d",
                   airline.c_str(), flt_no, airp.c_str(), DateTimeToStr( scd_out ).c_str(), pr_takeoff );
/*        if ( pr_takeoff ) {
          tst();
          break;
        }
      }  */
      }
/*    if ( pr_land || pr_takeoff ) {
      if ( fl_in != iflight->second.end() ) {
        ProgTrace( TRACE5, "flight landing not writed %s", fl_in->second.key().c_str() );
      }
      if ( fl_out != iflight->second.end() ) {
        ProgTrace( TRACE5, "flight takeoff not writed %s", fl_out->second.key().c_str() );
      }
      continue;
    }*/
      bool pr_own = false;
      TPointDests dests;
      if ( doubleMove_id != ASTRA::NoExists ) {
        BitSet<TUseDestData> FUseData;
        FUseData.clearFlags();
        FUseData.setFlag( udDelays );
        FUseData.setFlag( udStages );
        points.dests.Load( doubleMove_id, FUseData );
      }
      for ( std::vector<TCode>::iterator iairp=airps.begin(); iairp!=airps.end(); iairp++ ) {
        TPointsDest dest;
        dest.airp = iairp->code;
        dest.airp_fmt = iairp->fmt;
        if ( !pr_own &&
             ( (fl_in != iflight->second.end() && fl_in->second.own_airp == dest.airp) ||
               (fl_out != iflight->second.end() && fl_out->second.own_airp == dest.airp) ) ) {
          ProgTrace( TRACE5, "own airp found" );
          pr_own = true;
        }
        if ( fl_in != iflight->second.end() && fl_in->second.is_valid() ) {
          ProgTrace( TRACE5, "flight in  found flt_no=%d", fl_in->second.flt_no );
          if ( !pr_own ) {
            tst();
            dest.airline = fl_in->second.airline.code;
            dest.airline_fmt = fl_in->second.airline.fmt;
            dest.flt_no = fl_in->second.flt_no;
            dest.suffix = fl_in->second.suffix.code;
            dest.suffix_fmt = fl_in->second.suffix.fmt;
            dest.craft = fl_in->second.craft.code;
            dest.craft_fmt = fl_in->second.craft.fmt;
            dest.trip_type = fl_in->second.trip_type;
            if ( fl_in->second.status == "ОТМЕНЕН" ) {
              dest.pr_del = 1;
            }
          }
          if ( fl_in->second.own_airp == dest.airp ) {
            dest.scd_in = fl_in->second.scd;
            if ( fl_in->second.status == "ПРИЛЕТЕЛ" ) {
              dest.act_in = fl_in->second.est;
              tst();
            }
            else {
              dest.est_in = fl_in->second.est;
            }
          }
        }
        if ( fl_out != iflight->second.end() && fl_out->second.is_valid() ) {
          ProgTrace( TRACE5, "flight out  found flt_no=%d", fl_out->second.flt_no );
          if ( pr_own ) {
            tst();
            if ( iairp != airps.end() - 1 ) {
              dest.airline = fl_out->second.airline.code;
              dest.airline_fmt = fl_out->second.airline.fmt;
              dest.flt_no = fl_out->second.flt_no;
              dest.suffix = fl_out->second.suffix.code;
              dest.suffix_fmt = fl_out->second.suffix.fmt;
              dest.craft = fl_out->second.craft.code;
              dest.craft_fmt = fl_out->second.craft.fmt;
              dest.trip_type = fl_out->second.trip_type;
            }
            if ( fl_out->second.own_airp != dest.airp ) {
              if ( fl_out->second.status == "ОТМЕНЕН" ) {
                dest.pr_del = 1;
              }
            }
          }
          if ( fl_out->second.own_airp == dest.airp ) {
            dest.scd_out = fl_out->second.scd;
            if ( fl_out->second.status == "ВЫЛЕТЕЛ" ) {
              dest.act_out = fl_out->second.est;
            }
            else {
              dest.est_out = fl_out->second.est;
            }
          }
        }
        tst();
        dests.items.push_back( dest );
      }
      //синхронизация маршрута, но не уже существующих пунктов???
      bool pr_change_dests = true;
      if ( doubleMove_id != ASTRA::NoExists ) {
        //синхронизируем маршрут (удаление/добавление пунктов только для рейсов созданных Уфой
        uQry.SetVariable( "move_id", doubleMove_id );
        uQry.Execute();
        pr_change_dests = uQry.Eof;
      }
      ProgTrace( TRACE5, "dests.size()=%zu, pr_change_dests=%d", dests.items.size(), pr_change_dests );
      points.dests.sychDests( dests, pr_change_dests, true );
      ProgTrace( TRACE5, "doubleMove_id=%d", doubleMove_id );
      if ( doubleMove_id != ASTRA::NoExists ) {
        for ( std::vector<TPointsDest>::iterator idest=dests.items.begin(); idest!= dests.items.end(); idest++ ) {
          if ( idest->point_id == ASTRA::NoExists ) {
            tst();
            continue;
          }
          for ( std::vector<TPointsDest>::iterator jdest=points.dests.items.begin(); jdest!= points.dests.items.end(); jdest++ ) {
            ProgTrace( TRACE5, "idest->point_id=%d, jdest->point_id=%d", idest->point_id,  jdest->point_id );
            if ( idest->point_id == jdest->point_id ) {
              ProgTrace( TRACE5, "jdest->est_in=%f, idest->est_in=%f, jdest->est_out=%f, idest->est_out=%f, jdest->act_out=%f, idest->act_out=%f, jdest->craft=%s, idest->craft=%s",
                         jdest->est_in, idest->est_in, jdest->est_out, idest->est_out, jdest->act_out, idest->act_out, jdest->craft.c_str(), idest->craft.c_str() );
              if ( idest->est_in != ASTRA::NoExists ) {
                jdest->est_in = idest->est_in;
              }
              jdest->act_in = idest->act_in;
              if ( idest->est_out != ASTRA::NoExists ) {
                jdest->est_out = idest->est_out;
              }
              tst();
              jdest->act_out = idest->act_out;
              jdest->craft = idest->craft;
              jdest->craft_fmt = idest->craft_fmt;
              jdest->trip_type = idest->trip_type;
              jdest->pr_del = idest->pr_del;
              break;
            }
          }
        }
      }
      points.move_id = doubleMove_id;
    }
    catch( Exception &e ) {
      ProgError( STDLOG, "setFlight: exception=%s", e.what() );
      if ( fl_in != iflight->second.end() && fl_in->second.is_valid() ) {
           fl_in->second.error = string("sets error: ") + e.what();
      }
      if ( fl_out != iflight->second.end() && fl_out->second.is_valid() ) {
        fl_out->second.error = string("sets error: ") + e.what();
      }
      continue;
    }
    catch( ... ) {
      ProgError( STDLOG, "setFlight: unknown error move_id=%d", points.move_id );
      if ( fl_in != iflight->second.end() ) {
        fl_in->second.error = "sets error: unknown";
      }
      if ( fl_out != iflight->second.end() ) {
        fl_out->second.error = "sets error: unknown";
      }
      continue;
    }

    try {
      if ( !points.dests.items.empty() ) {
        try {
          points.Save( false );
          if ( fl_in != iflight->second.end() ) {
            fl_in->second.error = EncodeEventType( ASTRA::evtFlt );
          }
          if ( fl_out != iflight->second.end() ) {
            fl_out->second.error = EncodeEventType( ASTRA::evtFlt );
          }
          //надо сохранить рейс в aodb_points, чтобы знать, что рейс из СПП Уфа
          for ( std::vector<TPointsDest>::iterator idest=points.dests.items.begin();
                idest!=points.dests.items.end(); idest++ ) {
            if ( idest->airp == TReqInfo::Instance()->desk.airp ) {
              ProgTrace( TRACE5, "aodb_points update point_id=%d", idest->point_id );
              pQry.SetVariable( "point_id", idest->point_id );
              pQry.Execute();
              break;
            }
          }
          OraSession.Commit();
        }
        catch( Exception &e ) {
          try { OraSession.Rollback(); } catch(...){};
          ProgError( STDLOG, "saveFlights: exception=%s", e.what() );
          if ( fl_in != iflight->second.end() ) {
            fl_in->second.error = string("save error: ") + e.what();
          }
          if ( fl_out != iflight->second.end() ) {
            fl_out->second.error = string("save error: ") + e.what();
          }
        }
        catch( ... ) {
          ProgError( STDLOG, "saveFlights: unknown error move_id=%d", points.move_id );
          try { OraSession.Rollback(); } catch(...){};
          if ( fl_in != iflight->second.end() ) {
            fl_in->second.error = "save error: unknown";
          }
          if ( fl_out != iflight->second.end() ) {
            fl_out->second.error = "save error: unknown";
          }
        }
      }
    }
    catch( Exception &e ) {
      tst();
      if ( fl_in != iflight->second.end() ) {
        fl_in->second.error = e.what();
        tst();
      }
      if ( fl_out != iflight->second.end() ) {
        fl_out->second.error = e.what();
        tst();
      }
    }
  }
}
