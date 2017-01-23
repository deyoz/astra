#include "external_spp_synch.h"
#include "exceptions.h"
#include "points.h"
#include "aodb.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace EXCEPTIONS;
using namespace std;
using namespace BASIC::date_time;

#define SEARCH_SYNCHRON_FLIGHT_RANGE 24;

void TParseFlight::add_airline( const std::string &value ) {
  try {
    airline.code = ElemToElemId( etAirline, value, airline.fmt, false );
    if ( airline.fmt == efmtUnknown )
      throw EConvertError("");
/*      if ( airline.fmt == efmtCodeInter || airline.fmt == efmtCodeICAOInter )
          trip_type = "м";  //!!!vlad а правильно ли так определять тип рейса? не уверен. Проверка при помощи маршрута. Если в маршруте все п.п. принадлежат одной стране то "п" иначе "м"
    else
    */
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
  TCheckerFlt checkerFltNo;
  TFltNo fltNoStruct;
  checkerFltNo.parseFltNo( fltNo, fltNoStruct );
  //parseFlt( fltNo, tmp_airline, flt_no, suffix.code );
  flt_no = fltNoStruct.flt_no;
  suffix = fltNoStruct.suffix;
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
  TCheckerFlt check;
  try {
    craft = check.checkCraft( value, TCheckerFlt::etNormal, true );
  }
  catch( EConvertError &e ) {
    craft.clear();
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
    TElemStruct airp;
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
};

void ToEvent( const std::string &airline, const std::string &type,
              const std::string &record, const std::string &msg )
{
  TDateTime filter_scd;
  modf(UTCToLocal( NowUTC(), TReqInfo::Instance()->desk.tz_region ),&filter_scd);
  TQuery sQry( &OraSession );
  sQry.SQLText =
    "DECLARE "
    " vrec_no aodb_events.rec_no%TYPE;"
    "BEGIN "
    " UPDATE aodb_spp_files SET rec_no=NVL(rec_no,-1)+1 "
    "  WHERE filename=:filename AND point_addr=:point_addr AND airline=:airline "
    " RETURNING rec_no INTO vrec_no; "
    " IF SQL%NOTFOUND THEN "
    "   INSERT INTO aodb_spp_files(filename,point_addr,rec_no,airline) "
    "    VALUES(:filename,:point_addr,0,:airline); "
    "   vrec_no := 0;"
    " END IF;"
    " INSERT INTO aodb_events(filename,point_addr,rec_no,record,msg,type,time,airline) "
    "  VALUES(:filename,:point_addr,vrec_no,:record,:msg,:type,:time,:airline);"
    "END;";
  sQry.CreateVariable( "filename", otString, DateTimeToStr( filter_scd, "SPPyymmdd.txt" ) );
  sQry.CreateVariable( "point_addr", otString, TReqInfo::Instance()->desk.code );
  sQry.CreateVariable( "airline", otString, airline.empty()?"ЮТ":airline );
  sQry.CreateVariable( "record", otString, record.substr(0,4000) );
  sQry.CreateVariable( "msg", otString, msg.substr(0,1000) );
  sQry.CreateVariable( "type", otString, type );
  sQry.CreateVariable( "time", otDate, NowUTC() );
  sQry.Execute();
}

void HTTPRequestsIface::SaveSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace( TRACE5, "SaveSPP: desk=%s, airp=%s", TReqInfo::Instance()->desk.code.c_str(), TReqInfo::Instance()->desk.airp.c_str() );
  xmlNodePtr contentNode = GetNode( "content", reqNode );
  if ( contentNode == NULL ) {
    return;
  }
  TDateTime filter_scd;
  modf(UTCToLocal( NowUTC(), TReqInfo::Instance()->desk.tz_region ),&filter_scd);
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
      content[ line_buffer ].type = EncodeEventType( ASTRA::evtProgError );
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
      ProgTrace( TRACE5, "flights Add airline=%s, flt_no=%d, scd=%f, pr_landing=%d, key=%s",
                 flight.airline.code.c_str(),
                 flight.flt_no,
                 flight.scd,
                 flight.pr_landing,
                 flight.key().c_str() );

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
      content[ fl_out->second.record ].type = fl_out->second.error == EncodeEventType( ASTRA::evtFlt )?fl_out->second.error:EncodeEventType( ASTRA::evtProgError );
    }
  }
  string res = "\n";
  for ( std::map<std::string,AnswerContent>::iterator istr=content.begin(); istr!=content.end(); istr++ ) {
    ProgTrace( TRACE5, "record=%s, result=%s", istr->first.c_str(), istr->second.msg.c_str() );
    res += istr->first + istr->second.msg + "\n";
    ToEvent( istr->second.airline, istr->second.type, istr->first, istr->second.msg );
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
    TDateTime scd_in;
    TDateTime scd_out;
    bool pr_land = false, pr_takeoff= false;
    int doubleMove_id = ASTRA::NoExists;
    int doublePoint_id = ASTRA::NoExists;
    vector<TElemStruct> airps;
    map<bool, TParseFlight>::iterator fl_in = iflight->second.find( true );
    map<bool, TParseFlight>::iterator fl_out = iflight->second.find( false );
    ProgTrace( TRACE5, "fl_in=%d,valid=%d, fl_out=%d,valid=%d",
               fl_in != iflight->second.end(),
               fl_in != iflight->second.end() && fl_in->second.is_valid(),
               fl_out != iflight->second.end(),
               fl_out != iflight->second.end() && fl_out->second.is_valid() );
    bool pr_change_dests = false;
    try {
      if ( fl_in != iflight->second.end() && fl_in->second.is_valid() ) {
        tst();
        airps.clear();
        airline = fl_in->second.airline.code;
        flt_no = fl_in->second.flt_no;
        suffix = fl_in->second.suffix.code;
        airp = fl_in->second.own_airp;
        scd_in = fl_in->second.scd;
        airps.insert( airps.end(), fl_in->second.airps_in.begin(), fl_in->second.airps_in.end() );
        TElemStruct c;
        c.code = ElemCtxtToElemId( ecDisp, etAirp, fl_in->second.own_airp, c.fmt, false );
        airps.push_back( c );
        airps.insert( airps.end(), fl_in->second.airps_out.begin(), fl_in->second.airps_out.end() );
  /*      for ( vector<TElemStruct>::iterator iairp= airps.begin(); iairp!= airps.end(); iairp++ ) {*/
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
        tst();
        airps.clear();
        airline = fl_out->second.airline.code;
        flt_no = fl_out->second.flt_no;
        suffix = fl_out->second.suffix.code;
        airp = fl_out->second.own_airp;
        scd_out = fl_out->second.scd;
        airps.insert( airps.end(), fl_out->second.airps_in.begin(), fl_out->second.airps_in.end() );
        TElemStruct c;
        c.code = ElemCtxtToElemId( ecDisp, etAirp, fl_out->second.own_airp, c.fmt, false );
        airps.push_back( c );
        airps.insert( airps.end(), fl_out->second.airps_out.begin(), fl_out->second.airps_out.end() );
        /*for ( vector<TElemStruct>::iterator iairp= airps.begin(); iairp!= airps.end(); iairp++ ) {*/
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
      for ( std::vector<TElemStruct>::iterator iairp=airps.begin(); iairp!=airps.end(); iairp++ ) {
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
            ProgTrace( TRACE5, "dest.craft=%s", dest.craft.c_str() );
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
      if ( dests.items.size() >= 2 ) {
        dests.items.front().scd_in = ASTRA::NoExists;
        dests.items.front().est_in = ASTRA::NoExists;
        dests.items.front().act_in = ASTRA::NoExists;
        dests.items.back().scd_out = ASTRA::NoExists;
        dests.items.back().est_out = ASTRA::NoExists;
        dests.items.back().act_out = ASTRA::NoExists;
        dests.items.back().airline.clear();
        dests.items.back().flt_no = ASTRA::NoExists;
        dests.items.back().craft.clear();
        dests.items.back().craft_fmt = efmtUnknown;
        dests.items.back().bort.clear();
      }
      //синхронизация маршрута, но не уже существующих пунктов???
      pr_change_dests = true;
      if ( doubleMove_id != ASTRA::NoExists ) {
        //синхронизируем маршрут (удаление/добавление пунктов только для рейсов созданных Уфой
        uQry.SetVariable( "move_id", doubleMove_id );
        uQry.Execute();
        ProgTrace( TRACE5, "move_id=%d", doubleMove_id );
        pr_change_dests = !uQry.Eof; //получен из синхронизации с Уфой
      }
      ProgTrace( TRACE5, "dests.size()=%zu, pr_change_dests=%d", dests.items.size(), pr_change_dests );
      points.dests.sychDests( dests, pr_change_dests, dtSomeLocalSCD );
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
              if ( jdest->act_out == ASTRA::NoExists ) {
                jdest->act_out = idest->act_out;
                tst();
              }
              if ( !idest->craft.empty() && jdest->craft.empty() ) {
                ProgTrace( TRACE5, "set craft point_id=%d, craft=%s", idest->point_id,  idest->craft.c_str() );
                jdest->craft = idest->craft;
                jdest->craft_fmt = idest->craft_fmt;
              }
              jdest->trip_type = idest->trip_type;
              jdest->pr_del = idest->pr_del;
              break;
            }
          }
        }
      }
      points.move_id = doubleMove_id;
    }
    catch( std::exception &e ) {
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
            if ( pr_change_dests && idest->airp == TReqInfo::Instance()->desk.airp ) {
              ProgTrace( TRACE5, "aodb_points update point_id=%d", idest->point_id );
              pQry.SetVariable( "point_id", idest->point_id );
              pQry.Execute();
              break;
            }
          }
          OraSession.Commit();
        }
        catch( std::exception &e ) {
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
    catch( std::exception &e ) {
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

void IntWriteDests( double aodb_point_id, int range_hours, TPointDests &dests, std::string &warning );
/////////////////////////////////////////SINCHRON SVO///////////////////////////////
void parse_saveFlights( int range_hours, xmlNodePtr reqNode, xmlNodePtr resNode )
{
  xmlNodePtr node = GetNode( "flights", reqNode );
  resNode = NewTextChild( resNode, "flights" );
  if ( node == NULL ) {
    throw Exception( "Unknown xml: node Flights not found" );
  }
  std::string airp = TReqInfo::Instance()->desk.airp;
  node = node->children;
  TXMLFlightParser parser;
  string msg;
  string event;
  int flight_number = 1;
  double aodb_point_id;
  while ( node != NULL && (string)"flight" == (char*)node->name ) {
    TPointDests dests;
    bool prerror = true;
    try {
      std::string warning1, warning2;
      event = "parse";
      parser.parse( node, airp, dests, warning1 );
      StrToFloat( parser.id.c_str(), aodb_point_id );
      event = "write";      
      IntWriteDests( aodb_point_id, range_hours, dests, warning2 );
      msg = event + ": " + "flight_number=" + IntToString( flight_number ) + ",id=" + parser.id;
      if ( !warning1.empty() || !warning2.empty() ) {
        msg += ",warning=" + warning1 + warning2;
      }
      msg += ", ok";
      prerror = false;
      OraSession.Commit();
    }
    catch( Exception &e ) {
      OraSession.Rollback();
      msg = event + ": " + "flight_number=" + IntToString( flight_number ) + ",id=" + parser.id + ", error: " + e.what();
      ProgError( STDLOG, "%s", msg.c_str() );
    }
    catch( std::exception &e ) {
      OraSession.Rollback();
      msg = event + ": " + "flight_number=" + IntToString( flight_number ) + ",id=" + parser.id + ", internal error: " + e.what();
      ProgError( STDLOG, "%s", msg.c_str() );
    }
    catch( ... ) {
      OraSession.Rollback();
      msg = event + ": " + "flight_number=" + IntToString( flight_number ) + ",id=" + parser.id + ", unknown error";
      ProgError( STDLOG, "%s", msg.c_str() );
    }
    tst();
    xmlNodePtr ansNode = NewTextChild( resNode, "flight" );
    SetProp( ansNode, "flight_number", flight_number );
    SetProp( ansNode, "id", parser.id );
    if ( prerror ) {
      SetProp( ansNode, "error", msg );
    }
    else {
      SetProp( ansNode, "msg", msg );
    }
    //скопировать кусок XML-дерева начиная с тега flight
    xmlDocPtr flDoc = CreateXMLDoc( "flight" );
    string record;
    try {
      CopyNodeList( flDoc->children, node );
      record = XMLTreeToText( flDoc );
    }
    catch(...) {}
    xmlFreeDoc( flDoc );
    ToEvent( "", prerror?"!":"РЕЙ", record, msg );
    flight_number++;
    node = node->next;
  }

}

/* будут отдельно приходить рейсы на прилет и рейсы на вылет
<flights>
<flight>
    <status>N- new,U-update,D-delete cancel="true" - отмена рейса> при удалении учитываются теги     <id>, <flight_no>, <airline>, <suffix>
    <id> ? ид. рейса
    <flight_no> ? номер рейса
    <airline> - авиакомпания
    <suffix> - суффикс
    <litera> - литера
    <trip_type status={"D","I"} I - international, D  - domestic> регулярный - J,чартерный - C, дополнительный - Y
    <terminal> - терминал
    <park> - парковка
    <krm> !!! не используется
    <max_commerce> ? коммерческая загрузка
    <craft> ? тип ВС
    <bort> - борт
    <checkin_begin> ? начала регистрации
    <checkin_end> ? окончание регистрации
    <boarding_begin> ? начало посадки
    <boarding_end> ? окончание посадки
    <dests> ? маршрут включая пункт ШРМ
        <dest num=""> ? описание пункта вылета/прилета
            <airp> ? код аэропорта
            <scd_in> ? время прилета по расписанию
            <est_in> ? время прилета расчетное
            <act_in> ? время прилета  фактическое
            <scd_out> ? время вылета по расписанию
            <est_out> ? время вылета расчетное
            <act_out> ? время вылета фактическо
        </dest>
    ...
    </dests>
   <stations> ? описание стоек и выходов
        <station name=""  tp={?Р?,?П?}/>
        ...
    </stations>
<flight>
...
</flights>

 *
 *
 */

/*
  dests - маршрут. по нашему пукту заполнены структуры stages, stations, по всем времена прилета/вылета
  отдельно передается рейс на прилет и рейс на вылет
*/
void TXMLFlightParser::parse( xmlNodePtr flightNode, const std::string &airp, TPointDests &dests, std::string &warning )
{  
  warning.clear();
  dests.clear();
  id.clear();  
  if ( flightNode == NULL ) {
    throw EConvertError( "node 'flight' not found" );
  }
  TPointsDest dest;
  xmlNodePtr propNode;
  std::string region = getRegion( airp );
  flightNode = flightNode->children;
  //id
  string prop = NodeAsStringFast( "id", flightNode, "" );
  double fid;
  if ( StrToFloat( prop.c_str(), fid ) == EOF || fid < 0 || fid > 9999999999.0 ) {
    throw Exception( "Ошибка идентификатора рейса, значение=%s", prop.c_str() );
  }
  id = prop;
  propNode = GetNodeFast( "status", flightNode );
  if ( propNode == NULL ) {
    throw EConvertError( "node 'status' not found" );
  }
  prop = NodeAsString( propNode );
  if ( prop != "N" && prop != "U" && prop != "D" ) {
    throw EConvertError( "node 'status' unknown state '%s'", prop.c_str() );
  }
  if ( prop == "N" ) {
    dest.status = tdInsert;
  }
  else {
    if ( prop == "U" ) {
      dest.status = tdUpdate;
    }
    else {
      dest.status = tdDelete;      
    }
  }
  propNode = GetNodeFast( "@cancel", flightNode->children );
  if ( propNode ) {
    dest.pr_del = 1;
  }
  //airline,flight_no,suffix
  prop = NodeAsStringFast( "airline", flightNode );
  prop += NodeAsStringFast( "flight_no", flightNode );
  prop += NodeAsStringFast( "suffix", flightNode, "" );
  TCheckerFlt checkerFlt;
  TQuery Qry(&OraSession);
  TElemStruct elem;
  ProgTrace(TRACE5,"check fltNo");
  TFltNo fltNo = checkerFlt.parse_checkFltNo( prop, TCheckerFlt::CheckMode::etExtAODB, Qry );  
  dest.airline = fltNo.airline.code;
  dest.airline_fmt = fltNo.airline.fmt;
  dest.flt_no = fltNo.flt_no;
  dest.suffix = fltNo.suffix.code;
  dest.suffix_fmt = fltNo.suffix.fmt;
  if ( string("C") == NodeAsStringFast( "trip_type", flightNode, "п" ) ) { //важно, чтобы сохранялся тип рейса Чартер при "C" - только так надо строить условия определения типа
    dest.trip_type = "ч";
    tst();
  }
  else {
    propNode = GetNodeFast( "trip_type", flightNode );
    propNode = GetNode( "@status", propNode );
    if ( propNode != NULL && string("I") == NodeAsString( propNode) ) {
      dest.trip_type = "м";
    }
    else {
      dest.trip_type = "п";
    }
  }
  if ( dest.status == tdDelete ) { //airp не задан!!!
      dest.pr_del = -1;
/*    elem = checkerFlt.checkAirp( TReqInfo::Instance()->desk.airp, TCheckerFlt::CheckMode::etExtAODB, true, Qry );
    dest.airp = elem.code;
    dest.airp_fmt = elem.fmt;
    dests.items.push_back( dest );
    tst();
    return;*/
  }
  //litera
  ProgTrace(TRACE5,"check litera");
  dest.litera = checkerFlt.checkLitera( NodeAsStringFast( "litera", flightNode, "" ), TCheckerFlt::CheckMode::etExtAODB, Qry );
  //terminal
  ProgTrace(TRACE5,"check terminal");
  int terminal = checkerFlt.checkTerminalNo( NodeAsStringFast( "terminal", flightNode ) );
  //park
  prop = NodeAsStringFast( "park", flightNode, "" );
  dest.park_out = TrimString( prop ).substr( 0, 3 );
  //max_commerce
  ProgTrace(TRACE5,"check maxcommerce");
  dest.max_commerce.SetValue( checkerFlt.checkMaxCommerce( string(NodeAsStringFast( "max_commerce", flightNode, "" )) ) );
  //craft
  ProgTrace(TRACE5,"check craft");
  elem = checkerFlt.checkCraft( NodeAsStringFast( "craft", flightNode, "" ), TCheckerFlt::CheckMode::etExtAODB, false, Qry );
  dest.craft = elem.code;
  dest.craft_fmt = elem.fmt;
  ProgTrace( TRACE5, "craft=%s, fmt=%d", dest.craft.c_str(), dest.craft_fmt );
  //bort
  prop = NodeAsStringFast( "bort", flightNode, "" );
  dest.bort =  TrimString( prop ).substr( 0, 10 );
  ProgTrace(TRACE5,"check stages");
  //checkin_begin
  TTripStage stage;
  stage.stage = sOpenCheckIn;
  stage.scd = checkerFlt.checkLocalTime( string(NodeAsStringFast( "checkin_begin", flightNode, "" )), region, "Начало регистрации", false );
  dest.stages.SetStage( stage.stage, stage );
  //checkin_end
  stage.stage = sCloseCheckIn;
  stage.scd = checkerFlt.checkLocalTime( string(NodeAsStringFast( "checkin_end", flightNode, "" )), region, "Окончание регистрации", false );
  dest.stages.SetStage( stage.stage, stage );
  //boarding_begin
  stage.stage = sOpenBoarding;
  stage.scd = checkerFlt.checkLocalTime( string(NodeAsStringFast( "boarding_begin", flightNode, "" )), region, "Начало посадки", false );
  dest.stages.SetStage( stage.stage, stage );
  //boarding_end
  stage.stage = sCloseBoarding;
  stage.scd = checkerFlt.checkLocalTime( string(NodeAsStringFast( "boarding_end", flightNode, "" )), region, "Окончание посадки", false );
  dest.stages.SetStage( stage.stage, stage );
  dest.airp = airp;
  //stations - важен порядок - перед маршрутом выполнить
  ProgTrace(TRACE5,"check stations");
  xmlNodePtr n = GetNodeFast( "stations", flightNode );
  if ( n != NULL ) {
    n = n->children;
    while ( n != NULL && string("station") == (char*)n->name ) {
      propNode = GetNode( "@name", n );
      if ( propNode == NULL ) {
        throw EConvertError( "node 'station @name' not found" );
      }
      std::string name = NodeAsString( propNode );
      propNode = GetNode( "@tp", n );
      if ( propNode == NULL ) {
        throw EConvertError( "node 'station @tp' not found" );
      }
      std::string work_mode = NodeAsString( propNode );
      try {
        TSOPPStation station = checkerFlt.checkStation( airp, terminal, name, work_mode, TCheckerFlt::CheckMode::etNormal, Qry );
        dest.stations.Add( station );
      }
      catch( EConvertError &e ) {
        warning += string(" ;") + e.what();
      }
      n = n->next;
    }
  }
  ProgTrace(TRACE5,"dests");
  //dests - прилет или вылет
  n = GetNodeFast( "dests", flightNode );
  if ( n == NULL ) {
    throw EConvertError( "node 'dests' not found" );
  }
  n = n->children;
  map<int,TPointsDest> dsts;
  while ( n != NULL && string("dest") == (char*)n->name ) {
    propNode = n->children;
    TPointsDest p;
    TPointsDest *ppoint;
    ProgTrace(TRACE5,"check airp");
    elem = checkerFlt.checkAirp( NodeAsStringFast( "airp", propNode, "" ), TCheckerFlt::CheckMode::etExtAODB, true, Qry );
    if ( elem.code == dest.airp ) {
      ppoint = &dest;
    }
    else {
      ppoint = &p;
    }
    ppoint->airp = elem.code;
    ppoint->airp_fmt = elem.fmt;
    ProgTrace(TRACE5,"check times");
    ppoint->scd_in = checkerFlt.checkLocalTime( string(NodeAsStringFast( "scd_in", propNode, "" )), region, "Время прилета плановое " + ppoint->airp, false );
    ppoint->est_in = checkerFlt.checkLocalTime( string(NodeAsStringFast( "est_in", propNode, "" )), region, "Время прилета расчетное " + ppoint->airp, false );
    ppoint->act_in = checkerFlt.checkLocalTime( string(NodeAsStringFast( "act_in", propNode, "" )), region, "Время прилета фактическое " + ppoint->airp, false );
    ppoint->scd_out = checkerFlt.checkLocalTime( string(NodeAsStringFast( "scd_out", propNode, "" )), region, "Время вылета плановое " + ppoint->airp, false );
    ppoint->est_out = checkerFlt.checkLocalTime( string(NodeAsStringFast( "est_out", propNode, "" )), region, "Время вылета расчетное " + ppoint->airp, false );
    ppoint->act_out = checkerFlt.checkLocalTime( string(NodeAsStringFast( "act_out", propNode, "" )), region, "Время вылета фактическое " + ppoint->airp, false );
    propNode = GetNode( "@num", n );
    if ( propNode == NULL ) {
      throw EConvertError( "node 'dest @num' not found" );
    }
    prop = NodeAsString( propNode );
    ProgTrace(TRACE5,"check num");
    ppoint->point_num = checkerFlt.checkPointNum( prop );
    if ( dsts.find( ppoint->point_num ) != dsts.end() ) {
      throw EConvertError( "Дублирование номеров пункта посадки '%s'", prop.c_str() );
    }
    dsts[ ppoint->point_num ] = *ppoint;
    n = n->next;
  }
  for ( map<int,TPointsDest>::iterator idest=dsts.begin(); idest!=dsts.end(); idest++ ) {
    dests.items.push_back( idest->second );
    ProgTrace(TRACE5,"trip_type='%s'",idest->second.trip_type.c_str());
  }
  ProgTrace(TRACE5,"end check, dests size()=%zu", dests.items.size());
}

/*
 * на входе рейс из синхрона flt
 * ищем в Астре рейсы чартерные без факта вылета, на велет, не отмененные, в диапазоне range_hours
 * наиболее приближенный по дате к рейсу из Синхрона и имеющий в Астре признак того, что по рейсу приходило удаление
 */
class ConnectSinchronAstraCharterFlight
{
private:
  bool isDeleteStatusAODB( int point_id, TQuery &Qry ) {
    Qry.SetVariable( "point_id", point_id );
    Qry.Execute();
    return ( !Qry.Eof && Qry.FieldAsInteger( "pr_del" ) );
  }
public:
   void search( int range_hours, const TAdvTripInfo &flt, TAdvTripInfo &idealFlt ) {
     idealFlt.Clear();
     TQuery Qry(&OraSession);
     Qry.SQLText =
       "SELECT pr_del from aodb_points WHERE point_id=:point_id";
     Qry.DeclareVariable( "point_id", otInteger );
     TSearchFltInfo filter;
     filter.airline = flt.airline;
     filter.airp_dep = flt.airp;
     filter.flt_no = flt.flt_no;
     filter.suffix = flt.suffix;
     filter.dep_date_flags.setFlag(ddtEST);
     filter.scd_out_in_utc = true;
     filter.only_with_reg = true;
     double f, l;
     modf( flt.scd_out - range_hours/24.0, &f );
     modf( flt.scd_out + range_hours/24.0, &l );
     list<TAdvTripInfo> flts;
     for ( int d=(int)f; d<=l; d++ ) {
       filter.scd_out = d;
       flts.clear();
       SearchFlt( filter, flts ); //utc за сутки т.к. нет времени, а только дата
       for ( list<TAdvTripInfo>::iterator iflt=flts.begin(); iflt!=flts.end(); iflt++ ) {
         TPointsDest dest;
         BitSet<TUseDestData> UseData;
         UseData.clearFlags();
         dest.Load( iflt->point_id, UseData );
         if ( dest.trip_type != string("ч") ||
              dest.pr_del != 0 ||
              dest.scd_out == ASTRA::NoExists ||
              dest.act_out != ASTRA::NoExists ) {
           ProgTrace( TRACE5, "point_id=%d, continue", iflt->point_id );
           continue;
         }
         if ( idealFlt.point_id == ASTRA::NoExists ) { //first init
           if ( isDeleteStatusAODB( iflt->point_id, Qry ) ) {
             idealFlt = *iflt;
           }
           continue;
         }
         if ( fabs( idealFlt.real_out - flt.scd_out ) > fabs( iflt->real_out - flt.scd_out ) ) {
           if ( isDeleteStatusAODB( iflt->point_id, Qry ) ) {
             idealFlt = *iflt;
           }
         }
       }
     }
   }
};

void IntWriteDests( double aodb_point_id, int range_hours, TPointDests &dests, std::string &warning )
{
  tst();
  warning.clear();
  if ( dests.items.empty() ||
       (dests.items.size() == 1 && dests.items.begin()->status != tdDelete) ) {
    throw EConvertError( "flight dests empty" );
  }
  //ищем ШРМ, относительно которого завели рейс
  bool pr_find = false;
  TPoints points;  
  bool pr_takeoff;
  bool pr_charter_range = false;
  TQuery Qry(&OraSession);
  TPointsDest d;
  {
    std::vector<TPointsDest>::iterator idest;
    for ( idest=dests.items.begin(); idest!=dests.items.end(); idest++ ) {
      if ( !idest->airline.empty() ) { // нашли - ищем рейс в СПП
        int point_id;
        pr_find = TPoints::isDouble( ASTRA::NoExists, *idest, points.move_id, point_id );
        ProgTrace( TRACE5, "pr_find=%d, point_id=%d", pr_find, point_id );
        break;
      }
    }
    if ( idest == dests.items.end() ) { // на всякий случай
      throw EConvertError( "node 'airline' is empty" );
    }
    pr_takeoff = ( dests.items.begin() == idest );
    d = *idest;
  }
  if ( //!pr_find && //рейса новый или он переносится на другую дату - задержка? - ищем ближайший по времени
       d.trip_type == "ч" &&
       //d.status == tdInsert &&
       pr_takeoff &&
       d.scd_out != ASTRA::NoExists ) { // возможно это рейс, который в Синхроне удален и добавлен заново с новым плановым временем вылета
    Qry.Clear();
    Qry.SQLText =
      "SELECT point_id FROM aodb_points WHERE aodb_point_id=:aodb_point_id";
    Qry.CreateVariable( "aodb_point_id", otFloat, aodb_point_id );
    Qry.Execute();
    TAdvTripInfo astraFlt;
    if ( Qry.Eof ) {
      ConnectSinchronAstraCharterFlight searchAstraFlt;
      TAdvTripInfo sinchronFlt;
      sinchronFlt.airline = d.airline;
      sinchronFlt.airp = d.airp;
      sinchronFlt.flt_no = d.flt_no;
      sinchronFlt.suffix = d.suffix;
      sinchronFlt.scd_out = d.scd_out;
      searchAstraFlt.search( range_hours, sinchronFlt, astraFlt );
      tst();
    }
    else {
      astraFlt.point_id = Qry.FieldAsInteger( "point_id" );
      tst();
    }
    if ( astraFlt.point_id != ASTRA::NoExists ) {
      pr_find = true;
      Qry.Clear();
      Qry.SQLText =
        "SELECT move_id FROM points WHERE point_id=:point_id";
      Qry.CreateVariable( "point_id", otInteger, astraFlt.point_id );
      Qry.Execute();
      points.move_id = Qry.FieldAsInteger( "move_id" );
      pr_charter_range = true;
    }
  }
  ProgTrace( TRACE5, "pr_find=%d, move_id=%d", pr_find, points.move_id );

  if ( pr_find ) { // рейс нашелся, надо зачитать
    if ( d.status == tdDelete ) {
      ProgTrace( TRACE5, "flight status=delete, but not save this event to db, ignore" );
      //aodb_points - set pr_del for charter or update aodb_point_id to NULL!!!
      Qry.Clear();
      Qry.SQLText =
        "UPDATE aodb_points SET pr_del=1 WHERE aodb_point_id=:aodb_point_id";
      Qry.CreateVariable( "aodb_point_id", otFloat, aodb_point_id  );
      Qry.Execute();
      return;
    }
    BitSet<TUseDestData> UseData;
    UseData.clearFlags();
    UseData.setFlag( udStages );
    UseData.setFlag( udMaxCommerce );
    //UseData.setFlag( udStations );
    points.dests.Load( points.move_id, UseData );
  }
  else { // новый рейс, а можно ли его создать?
    TTripInfo info;
    info.airline =  d.airline;
    info.airp = d.airp;
    info.flt_no = d.flt_no;
    if ( !GetTripSets( tsAODBCreateFlight, info ) ) {
      ProgTrace( TRACE5, "IntWriteDests: missing right for create flight" );
      throw EConvertError( "missing right for create flight" );
    }
    if ( d.craft.empty() ) {
      warning += ";Не задан тип ВС";
      //!!!throw EConvertError( "Не задан тип ВС" );
    }
    else
      if ( d.craft_fmt == efmtUnknown ) {
        warning += ";Неизвестный тип ВС, значение='" + d.craft + "'";
        d.craft.clear();
        //!!!throw EConvertError( "Неизвестный тип ВС, значение=%s", d.craft.c_str() );
      }
  }
  // задаем всему маршруту АК, номер рейса, суффикс, тип ВС, борт..
  for ( std::vector<TPointsDest>::iterator chdest=dests.items.begin(); chdest!=dests.items.end(); chdest++ ) {
    chdest->airline = d.airline;
    chdest->airline_fmt = d.airline_fmt;
    chdest->flt_no = d.flt_no;
    chdest->suffix = d.suffix;
    chdest->suffix_fmt = d.suffix_fmt;
    chdest->craft = d.craft;
    chdest->craft_fmt = d.craft_fmt;
    chdest->bort = d.bort;
    chdest->trip_type = d.trip_type;
    chdest->litera = d.litera;
    chdest->pr_del = d.pr_del;
  }
  if ( d.status == tdDelete && !pr_find ) {
    throw EConvertError( ";delete flight not exists" );
  }
  if ( d.status == tdInsert && pr_find ) {
    warning += ";new flight already exist";
    d.status = tdUpdate;
  }
  if ( d.status == tdUpdate && !pr_find ) {
    warning += ";update flight not exist";
    d.status = tdInsert;
  }
  std::vector<TPointsDest>::iterator owndest = points.dests.items.end();
  // синхронизируем маршрут

  if ( d.status == tdUpdate ) { // рейс передается либо только на прилет либо только на вылет
    std::vector<TPointsDest>::iterator pd = dests.items.begin();
    bool prown = false;
    for ( owndest=points.dests.items.begin(); owndest!=points.dests.items.end(); owndest++ ) {
      if ( pr_takeoff ) { // добавляем пункты на прилет, т.к. их нет в маршруте, а маршрут надо синхронизировать
        if ( d.airp == owndest->airp ) {
          break;
        }
        pd = dests.items.insert( pd, *owndest );
      }
      else {
        if ( d.airp == owndest->airp ) {
          prown = true;
          continue;
        }
        if ( prown ) {
          dests.items.push_back( *owndest );
        }
      }
    }
  }

  if ( owndest != points.dests.items.end() && owndest->point_id != ASTRA::NoExists ) { // надо добавить изменения аккуратно
    if ( d.craft.empty() || d.craft_fmt == efmtUnknown ) {
      if ( d.craft.empty() ) {
        warning += " ;Не задан тип ВС, оставляем старое значение '" + owndest->craft + "'";
      }
      else
        if ( d.craft_fmt == efmtUnknown ) {
          warning += " ;Неизвестный тип ВС, значение '" + d.craft + "', оставляем старое значение '" + owndest->craft + "'";
        }
      d.craft = owndest->craft;
      d.craft_fmt = owndest->craft_fmt;
    }
    if ( d.bort.empty() && !owndest->bort.empty() ) {
      warning += " ;Не задан борт ВС, оставляем старое значение '" + owndest->bort + "'";
      d.bort = owndest->bort;
    }
    if ( d.litera.empty() && !owndest->litera.empty() ) {
      warning += " ;Не задана литера, оставляем старое значение '" + owndest->litera + "'";
      d.litera = owndest->litera;
    }
    if ( d.max_commerce.GetValue() == ASTRA::NoExists && owndest->max_commerce.GetValue() != ASTRA::NoExists ) {
      warning += " ;Не задана макс. ком. загрузка, оставляем старое значение '" + IntToString( owndest->max_commerce.GetValue() ) + "'";
      d.max_commerce.SetValue( owndest->max_commerce.GetValue() );
    }
    owndest->airline = d.airline;
    owndest->airline_fmt = d.airline_fmt;
    owndest->flt_no = d.flt_no;
    owndest->suffix = d.suffix;
    owndest->suffix_fmt = d.suffix_fmt;
    owndest->trip_type = d.trip_type;
    owndest->litera = d.litera;
    owndest->bort = d.bort;
    owndest->craft = d.craft;
    owndest->craft_fmt = d.craft_fmt;
    owndest->park_in = d.park_in;
    owndest->park_out = d.park_out;
    owndest->scd_in = d.scd_in;
    owndest->est_in = d.est_in;
    owndest->act_in = d.act_in;
    owndest->est_out = d.est_out;
    if ( pr_charter_range ) { // был перенесен рейс, изменилась плановая дата вылета в Синхроне, у нас старый рейс, изменяем расчетное время вылета
      //!!!if ( owndest->est_out == ASTRA::NoExists ) {
        if ( d.est_out == ASTRA::NoExists ) {
          owndest->est_out = d.scd_out;
        }
        if ( pr_takeoff ) {
          dests.items.begin()->scd_out = owndest->scd_out; //возвращаем дату на ту, по которой создан рейс
        }
//!!!      }
      //надо проставить старое плановое время вылета, иначе произойдет удаление ШРМ и добавление нового
    }
    else {
      owndest->scd_out = d.scd_out;
    }
    owndest->act_out = d.act_out;
    owndest->pr_del = d.pr_del;
  }
  if ( !dests.items.empty() ) { //очистить последний пункт посадок иначе не будет проходить синхронизация
    dests.items.rbegin()->airline.clear();
    dests.items.rbegin()->flt_no = ASTRA::NoExists;
    dests.items.rbegin()->suffix.clear();
  }
  if ( d.status == tdDelete ) {
    dests.items.clear();
  }
  //синхронизация пунктов посадки с удалением пунктов
  points.dests.sychDests( dests, true, dtSomeLocalSCD );
  // сохраняем
  //try {
    points.Save( false );
    for ( owndest=points.dests.items.begin(); owndest!=points.dests.items.end(); owndest++ ) {
      if ( d.airp == owndest->airp ) {
        break;
      }
    }
    if ( owndest->point_id != ASTRA::NoExists ) {
      // имеем point_id
      //сохраняем стойки и выходы
      tst();
      d.stations.Save( owndest->point_id );
      //max_commerce
      d.max_commerce.Save( owndest->point_id );
      //stages and delay for next dest!!!
      //bindingAODBFlt была вызвана, когда сохраняли маршрут ???
      bindingAODBFlt( TReqInfo::Instance()->desk.code, owndest->point_id, aodb_point_id );
    }
/*  }
  catch( Exception &e ) {
    OraSession.Rollback();
    warning += string(" ;write flight error : ") + e.what();
    ProgError( STDLOG, "%s", warning.c_str() );
  }
  catch( std::exception &e ) {
    OraSession.Rollback();
    warning += string(" ;write flight error : ") + e.what();
    ProgError( STDLOG, "%s", warning.c_str() );
  }
  catch( ... ) {
    OraSession.Rollback();
    warning += " ;write flight error : unknown";
    ProgError( STDLOG, "%s", warning.c_str() );
  }*/
}




/// \brief HTTPRequestsIface::SaveSinhronSPP
/// \param ctxt
/// \param reqNode
/// \param resNode
///

void HTTPRequestsIface::SaveSinhronSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace( TRACE5, "SaveSinhronSPP: desk=%s, airp=%s", TReqInfo::Instance()->desk.code.c_str(), TReqInfo::Instance()->desk.airp.c_str() );
  int range_hours = SEARCH_SYNCHRON_FLIGHT_RANGE; //+-24 часа для поиска чартера
  parse_saveFlights( range_hours, reqNode, resNode );
}

void testSinchron()
{
  string s = "<?xml version='1.0' encoding='UTF-8'?>"
      "<flights>"
      "<flight>"
      "<status>U</status>"
      "<id>19</id>"
      "<flight_no>187</flight_no>"
      "<airline>N4</airline>"
      "<terminal>3</terminal>"
      "<park>B55</park>"
      "<craft>321</craft>"
      "<bort>VQBOD</bort>"
      "<checkin_begin>07.11.2016 17:00</checkin_begin>"
      "<checkin_end>07.11.2016 22:20</checkin_end>"
      "<boarding_begin>07.11.2016 22:24</boarding_begin>"
      "<boarding_end>07.11.2016 22:50</boarding_end>"
      "<trip_type status='D'>J</trip_type>"
      "<dests>"
      "<dest num='1'>"
      "<airp>SVO</airp>"
      "<scd_out>07.11.2016 23:00</scd_out>"
      "<act_out>07.11.2016 23:45</act_out>"
      "</dest>"
      "<dest num='2'>"
      "<airp>ROV</airp>"
      "</dest>"
      "</dests>"
      "<stations>"
      "<station tp='Р' name='96'/>"
      "<station tp='Р' name='97'/>"
      "<station tp='Р' name='98'/>"
      "<station tp='Р' name='99'/>"
      "<station tp='П' name='2'/>"
      "</stations>"
      "</flight>"
      "</flights>";
}
