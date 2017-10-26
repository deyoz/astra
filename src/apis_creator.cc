#include "apis_creator.h"
#include <boost/scoped_ptr.hpp>
#include "apis.h"
#include <fstream>
#include "obrnosir.h"

#define NICKNAME "GRIG"
#include "serverlib/test.h"
#include "serverlib/slogger.h"

// TODO приведение типов в стиле C++
// TODO убрать все дубликаты относительно apis.cc

#define MAX_PAX_PER_EDI_PART 15
#define MAX_LEN_OF_EDI_PART 3000

#if APIS_TEST
bool TApisDataset::FromDB(int point_id, const string& task_name, TApisTestMap* test_map)
#else
bool TApisDataset::FromDB(int point_id, const string& task_name)
#endif
{
  try
  {
#if APIS_TEST
    if (test_map == nullptr)
      return false;
#endif
    TQuery Qry(&OraSession);
    Qry.SQLText =
    "SELECT airline, flt_no, suffix, airp, scd_out, act_out, "
    "       point_num, first_point, pr_tranzit, "
    "       country "
    "FROM points,airps,cities "
    "WHERE points.airp=airps.code AND airps.city=cities.code AND "
    "      point_id=:point_id AND points.pr_del=0 AND points.pr_reg<>0 ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof)
      return false;

    airline_code_qry = Qry.FieldAsString("airline");
    string country_dep = Qry.FieldAsString("country");

    TTripRoute route;
    route.GetRouteAfter(NoExists,
                        point_id,
                        Qry.FieldAsInteger("point_num"),
                        Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
                        Qry.FieldAsInteger("pr_tranzit")!=0,
                        trtNotCurrent, trtNotCancelled);

    TQuery RouteQry(&OraSession);
    RouteQry.SQLText=
    "SELECT airp,scd_in,scd_out,country "
    "FROM points,airps,cities "
    "WHERE points.airp=airps.code AND airps.city=cities.code AND point_id=:point_id";
    RouteQry.DeclareVariable("point_id",otInteger);

    TQuery ApisSetsQry(&OraSession);
    ApisSetsQry.Clear();
#if APIS_TEST_ALL_FORMATS
      ApisSetsQry.SQLText = apis_test_text;
#else
      ApisSetsQry.SQLText=
      "SELECT dir,edi_addr,edi_own_addr,format "
      "FROM apis_sets "
      "WHERE airline=:airline AND country_dep=:country_dep AND country_arv=:country_arv AND pr_denial=0";
      ApisSetsQry.CreateVariable("airline", otString, airline_code());
      ApisSetsQry.CreateVariable("country_dep", otString, country_dep);
      ApisSetsQry.DeclareVariable("country_arv", otString);
#endif

    TQuery PaxQry(&OraSession);
    PaxQry.SQLText=
    // "SELECT pax.pax_id, pax.surname, pax.name, pax.pr_brd, pax.grp_id, pax.crew_type, "
    "SELECT pax.*, "
    "       tckin_segments.airp_arv AS airp_final, pax_grp.status, pers_type, ticket_no, "
    "       ckin.get_bagAmount2(pax.grp_id, pax.pax_id, pax.bag_pool_num) AS bag_amount, "
    "       ckin.get_bagWeight2(pax.grp_id, pax.pax_id, pax.bag_pool_num) AS bag_weight "
    "FROM pax_grp,pax,tckin_segments "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.grp_id=tckin_segments.grp_id(+) AND tckin_segments.pr_final(+)<>0 AND "
    "      pax_grp.point_dep=:point_dep AND pax_grp.point_arv=:point_arv AND pr_brd IS NOT NULL AND "
    "      (pax.name IS NULL OR pax.name<>'CBBG') "
    "ORDER BY pax.reg_no, pax.pax_id";
    PaxQry.CreateVariable("point_dep",otInteger,point_id);
    PaxQry.DeclareVariable("point_arv",otInteger);

    TQuery SeatsQry(&OraSession);
    SeatsQry.SQLText=
      "SELECT yname AS seat_row, xname AS seat_column "
      "FROM pax_seats "
      "WHERE pax_id=:pax_id AND point_id=:point_id AND NVL(pr_wl,0)=0";
    SeatsQry.CreateVariable("point_id",otInteger,point_id);
    SeatsQry.DeclareVariable("pax_id",otInteger);

    TQuery CustomsQry(&OraSession);

    map<string /*country_regul_arv*/, string /*first airp_arv*/> CBPAirps;

    for(TTripRoute::const_iterator iRoute = route.begin(); iRoute != route.end(); iRoute++)
    {
#if APIS_TEST
      test_map->try_key.set(iRoute->point_id, "");
#endif
      TApisRouteData rd;
      rd.dataset_point_id = point_id;
      rd.route_point_id = iRoute->point_id;
      rd.task_name = task_name;
      rd.airline_code_qry = airline_code_qry;
      rd.country_dep = country_dep;

      //получим информацию по пункту маршрута
      RouteQry.SetVariable("point_id",rd.route_point_id);
      RouteQry.Execute();
      if (RouteQry.Eof)
        continue;

      const TCountriesRow& country_arv = (const TCountriesRow&)base_tables.get("countries").get_row("code",RouteQry.FieldAsString("country"));
      rd.country_arv_code = country_arv.code;
      rd.country_arv_code_lat = country_arv.code_lat;
      rd.country_regul_dep = APIS::GetCustomsRegulCountry(rd.country_dep, CustomsQry);
      rd.country_regul_arv = APIS::GetCustomsRegulCountry(rd.country_arv_code, CustomsQry);
      rd.use_us_customs_tasks = rd.country_regul_dep==US_CUSTOMS_CODE || rd.country_regul_arv==US_CUSTOMS_CODE;
      map<string, string>::iterator iCBPAirp = CBPAirps.find(rd.country_regul_arv);
      if (iCBPAirp==CBPAirps.end())
        iCBPAirp=CBPAirps.insert(make_pair(rd.country_regul_arv, RouteQry.FieldAsString("airp"))).first;
      if (iCBPAirp==CBPAirps.end())
        throw Exception("iCBPAirp==CBPAirps.end()");

      if  (!( rd.task_name.empty()
              ||
              (rd.use_us_customs_tasks &&
              (rd.task_name==BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL || rd.task_name==BEFORE_TAKEOFF_60_US_CUSTOMS_ARRIVAL ))
              ||
              (!rd.use_us_customs_tasks &&
              (rd.task_name==ON_TAKEOFF || rd.task_name==ON_CLOSE_CHECKIN || rd.task_name==ON_CLOSE_BOARDING )) ))
        continue;

      //получим информацию по настройке APIS
#if !APIS_TEST_ALL_FORMATS
      ApisSetsQry.SetVariable("country_arv", rd.country_arv_code);
#endif
      ApisSetsQry.Execute();
      if (ApisSetsQry.Eof)
        continue;

      rd.flt_no = Qry.FieldAsInteger("flt_no");

      if (!Qry.FieldIsNULL("suffix"))
      {
        const TTripSuffixesRow& suffixRow = (const TTripSuffixesRow&)base_tables.get("trip_suffixes").get_row("code",Qry.FieldAsString("suffix"));
        if (suffixRow.code_lat.empty())
          throw Exception("suffixRow.code_lat empty (code=%s)",suffixRow.code.c_str());
        rd.suffix = suffixRow.code_lat;
      }

      string tz_region;

      rd.airp_dep_qry = Qry.FieldAsString("airp");
      tz_region = AirpTZRegion(rd.airp_dep_code());
      if (Qry.FieldIsNULL("scd_out"))
        throw Exception("scd_out empty (airp_dep=%s)",rd.airp_dep_code().c_str());
      rd.scd_out_local	= UTCToLocal(Qry.FieldAsDateTime("scd_out"), tz_region);

      rd.final_apis = ( rd.task_name==ON_TAKEOFF || rd.task_name==ON_CLOSE_BOARDING ||
                        (rd.task_name.empty() && !Qry.FieldIsNULL("act_out")) );

      rd.airp_arv_qry = RouteQry.FieldAsString("airp");
      tz_region = AirpTZRegion(rd.airp_arv_code());
      if (RouteQry.FieldIsNULL("scd_in"))
        throw Exception("scd_in empty (airp_arv=%s)",rd.airp_arv_code().c_str());
      rd.scd_in_local = UTCToLocal(RouteQry.FieldAsDateTime("scd_in"),tz_region);

      rd.airp_cbp_qry = iCBPAirp->second;

      rd.airline_name = rd.airline().short_name_lat;
      if (rd.airline_name.empty())
        rd.airline_name = rd.airline().name_lat;
      if (rd.airline_name.empty())
        rd.airline_name = rd.airline_code_lat();

      //Flight legs
      TTripRoute legs_route, legs_tmp;
      legs_route.GetRouteBefore(NoExists, rd.dataset_point_id, trtWithCurrent, trtNotCancelled);
      legs_tmp.GetRouteAfter(NoExists, rd.dataset_point_id, trtNotCurrent, trtNotCancelled);
      legs_route.insert(legs_route.end(), legs_tmp.begin(), legs_tmp.end());
      for (TTripRoute::const_iterator r = legs_route.begin(); r != legs_route.end(); r++)
      {
        FlightlegDataset leg;
        RouteQry.SetVariable("point_id",r->point_id);
        RouteQry.Execute();
        if (RouteQry.Eof)
          continue;
        const TAirpsRow& airp = (const TAirpsRow&)base_tables.get("airps").get_row("code",RouteQry.FieldAsString("airp"));
        leg.airp_code_lat = airp.code_lat;
        leg.airp_code = airp.code;
        string tz_region = AirpTZRegion(airp.code);
        leg.scd_in_local = leg.scd_out_local = ASTRA::NoExists;
        if (!RouteQry.FieldIsNULL("scd_out"))
          leg.scd_out_local	= UTCToLocal(RouteQry.FieldAsDateTime("scd_out"),tz_region);
        if (!RouteQry.FieldIsNULL("scd_in"))
          leg.scd_in_local = UTCToLocal(RouteQry.FieldAsDateTime("scd_in"),tz_region);
        const TCountriesRow& countryRow = (const TCountriesRow&)base_tables.get("countries").get_row("code",RouteQry.FieldAsString("country"));
        leg.country_code_iso = countryRow.code_iso;
        leg.country_code = countryRow.code;
        rd.lstLegs.push_back(leg);
      }

      for(; !ApisSetsQry.Eof; ApisSetsQry.Next())
      {
        TApisSetsData sd;
        sd.fmt = ApisSetsQry.FieldAsString("format");

#if APIS_TEST
        test_map->try_key.set(iRoute->point_id, sd.fmt);
#endif
        sd.edi_own_addr = ApisSetsQry.FieldAsString("edi_own_addr");
        sd.edi_addr = ApisSetsQry.FieldAsString("edi_addr");
        sd.dir = ApisSetsQry.FieldAsString("dir");
        rd.lstSetsData.push_back(sd);
      } // for ApisSetsQry
      if (rd.lstSetsData.empty())
        continue;

      PaxQry.SetVariable("point_arv",rd.route_point_id);
      PaxQry.Execute();

      for (;!PaxQry.Eof;PaxQry.Next())
      {
        TApisPaxData pax;
        pax.fromDB(PaxQry);

        pax.status = DecodePaxStatus(PaxQry.FieldAsString("status"));

        if (!PaxQry.FieldIsNULL("airp_final"))
        {
          const TAirpsRow& airp_final = (const TAirpsRow&)base_tables.get("airps").get_row("code",PaxQry.FieldAsString("airp_final"));
          pax.airp_final_lat = airp_final.code_lat;
          pax.airp_final_code = airp_final.code;
        }
        else
          pax.airp_final_lat = rd.airp_arv_code_lat();

        LoadPaxDoc(pax.id, (CheckIn::TPaxDocItem&)pax.doc);
        pax.doco = CheckIn::LoadPaxDoco(pax.id);
        pax.docaD = CheckIn::LoadPaxDoca(pax.id, CheckIn::docaDestination);
        pax.docaR = CheckIn::LoadPaxDoca(pax.id, CheckIn::docaResidence);
        pax.docaB = CheckIn::LoadPaxDoca(pax.id, CheckIn::docaBirth);

        SeatsQry.SetVariable("pax_id", pax.id);
        SeatsQry.Execute();
        for(;!SeatsQry.Eof;SeatsQry.Next())
          pax.seats.push_back(make_pair(SeatsQry.FieldAsInteger("seat_row"), SeatsQry.FieldAsString("seat_column")));

        pax.amount = 0;
        if (!PaxQry.FieldIsNULL("bag_amount"))
          pax.amount = PaxQry.FieldAsInteger("bag_amount");
        pax.weight = 0;
        if (!PaxQry.FieldIsNULL("bag_weight"))
          pax.weight = PaxQry.FieldAsInteger("bag_weight");

        rd.lstPaxData.push_back(pax);
      } // for PaxQry
      lstRouteData.push_back(rd);
    } // for TTripRoute
  } // try
  catch(Exception &E)
  {
    throw Exception("TApisCreator::FromDB: %s",E.what());
  }
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool omit_incomplete_apis(int point_id, const TApisPaxData& pax, const TAPISFormat& format)
{
  TTripInfo fltInfo;
  if (fltInfo.getByPointId(point_id))
  {
    bool noCtrlDocsCrew = (pax.status == psCrew) && GetTripSets(tsNoCtrlDocsCrew, fltInfo);
    bool noCtrlDocsExtraCrew =  (pax.status != psCrew) &&
                                (pax.crew_type == ASTRA::TCrewType::ExtraCrew ||
                                  pax.crew_type == ASTRA::TCrewType::DeadHeadCrew ||
                                  pax.crew_type == ASTRA::TCrewType::MiscOperStaff) &&
                                GetTripSets(tsNoCtrlDocsExtraCrew, fltInfo);
    if (noCtrlDocsCrew || noCtrlDocsExtraCrew)
    {
      TAPISFormat::TPaxType pax_type = (pax.status == psCrew) ? TAPISFormat::crew : TAPISFormat::pass;
      std::set<TAPIType> apis_doc_set = TCompleteAPICheckInfo::get_apis_doc_set();
      for (std::set<TAPIType>::const_iterator api = apis_doc_set.begin(); api != apis_doc_set.end(); ++api)
      {
        long int required_fields = format.required_fields(pax_type, *api);
        const CheckIn::TPaxAPIItem* pItem = nullptr;

        switch (*api)
        {
          case apiDoc:                    pItem = &pax.doc;         break;
          case apiDoco:   if (pax.doco)   pItem = &pax.doco.get();  break;
          case apiDocaB:  if (pax.docaB)  pItem = &pax.docaB.get(); break;
          case apiDocaR:  if (pax.docaR)  pItem = &pax.docaR.get(); break;
          case apiDocaD:  if (pax.docaD)  pItem = &pax.docaD.get(); break;
          default: throw EXCEPTIONS::Exception("Unhandled TAPIType %d", *api);
        }

        if (pItem != nullptr)
        {
          if ((pItem->getNotEmptyFieldsMask() & required_fields) != required_fields)
            return true;
        }
        else
        {
          if (required_fields != NO_FIELDS)
            return true;
        }
      }
    }
  }
  return false;
}

/////////////////////////////////////////////////////////////////////////////////////////

void CreateEdi( const TApisRouteData& route,
                const TAPISFormat& format,
                Paxlst::PaxlstInfo& FPM,
                Paxlst::PaxlstInfo& FCM)
{
  for(int pass=0; pass<2; pass++)
  {
    Paxlst::PaxlstInfo& paxlstInfo=(pass==0?FPM:FCM);

    paxlstInfo.settings().setRespAgnCode(format.respAgnCode());
    paxlstInfo.settings().setAppRef(format.appRef());
    paxlstInfo.settings().setMesRelNum(format.mesRelNum());
    paxlstInfo.settings().setMesAssCode(format.mesAssCode());
    paxlstInfo.settings().setViewUNGandUNE(format.viewUNGandUNE());

    list<TAirlineOfficeInfo> offices;
    GetAirlineOfficeInfo(route.airline_code(), route.country_arv_code, route.airp_arv_code(), offices);
    if (!offices.empty())
    {
      paxlstInfo.setPartyName(offices.begin()->contact_name);
      paxlstInfo.setPhone(format.ProcessPhoneFax(offices.begin()->phone));
      paxlstInfo.setFax(format.ProcessPhoneFax(offices.begin()->fax));
    }

    if (format.rule(_notSetSenderRecipient))
    {
      paxlstInfo.setSenderCarrierCode(route.airline_code_lat());
    }
    else
    {
      vector<string> strs;
      vector<string>::const_iterator i;
      SeparateString(format.edi_own_addr, ':', strs);
      i=strs.begin();
      if (i!=strs.end())
        paxlstInfo.setSenderName(*i++);
      if (i!=strs.end())
        paxlstInfo.setSenderCarrierCode(*i++);
      SeparateString(format.edi_addr, ':', strs);
      i=strs.begin();
      if (i!=strs.end())
        paxlstInfo.setRecipientName(*i++);
      if (i!=strs.end())
        paxlstInfo.setRecipientCarrierCode(*i++);
    }

    ostringstream flight;
    if (!format.rule(_omitAirlineCode))
      flight << route.airline_code_lat();
    flight << route.flt_no << route.suffix;
    string iataCode;
    switch (format.IataCodeType())
    {
      case iata_code_UK:
        //"[flight][scd_in[yyyymmddhhnn]]"
        iataCode=Paxlst::createIataCode(flight.str(), route.scd_in_local, "yyyymmddhhnn");
        break;
      case iata_code_TR:
        iataCode=Paxlst::createIataCode(route.airline_code_lat() + flight.str(), route.scd_in_local, "/yymmdd/hhnn");
        break;
      case iata_code_DE:
        iataCode=Paxlst::createIataCode(flight.str(), route.scd_in_local, "yymmddhhnnss");
        break;
      case iata_code_default:
      default:
        iataCode=Paxlst::createIataCode(flight.str(), route.scd_in_local, "/yymmdd/hhnn");
        break;
    }
    // LogTrace(TRACE5) << "iataCode \"" << iataCode << "\" type " << format.IataCodeType();
    paxlstInfo.setIataCode( iataCode );

    if (format.rule(_setCarrier))
      paxlstInfo.setCarrier(route.airline_code_lat());

    paxlstInfo.setFlight(flight.str());
    paxlstInfo.setDepPort(route.airp_dep_code_lat());
    paxlstInfo.setDepDateTime(route.scd_out_local);
    paxlstInfo.setArrPort(route.airp_arv_code_lat());
    paxlstInfo.setArrDateTime(route.scd_in_local);

    if (format.rule(_setFltLegs))
    {
      FlightLegs legs;
      for (list<FlightlegDataset>::const_iterator iLeg = route.lstLegs.begin(); iLeg != route.lstLegs.end(); ++iLeg)
      {
        if (iLeg->airp_code_lat.empty())
          throw EXCEPTIONS::Exception("airp.code_lat empty (code=%s)",iLeg->airp_code.c_str());
        if (iLeg->country_code_iso.empty())
          throw EXCEPTIONS::Exception("countryRow.code_iso empty (code=%s)",iLeg->country_code.c_str());
        legs.push_back(FlightLeg(iLeg->airp_code_lat, iLeg->country_code_iso, iLeg->scd_in_local, iLeg->scd_out_local));
      }
      legs.FillLocQualifier();
      paxlstInfo.setFltLegs(legs);
    }
  } // for int pass

  for ( list<TApisPaxData>::const_iterator iPax = route.lstPaxData.begin();
        iPax != route.lstPaxData.end();
        ++iPax)
  {
    Paxlst::PassengerInfo paxInfo;

    if (iPax->status==psCrew && !format.rule(_notOmitCrew))
      continue;
    if (iPax->status!=psCrew && !iPax->pr_brd && route.final_apis)
      continue;
    if (omit_incomplete_apis(route.dataset_point_id, *iPax, format))
      continue;

    if (format.rule(_setPrBrd))
      paxInfo.setPrBrd(iPax->pr_brd);

    if (format.rule(_setGoShow))
      paxInfo.setGoShow(iPax->status==psGoshow);

    if (format.rule(_setPersType))
    {
      switch(iPax->pers_type)
      {
        case adult: paxInfo.setPersType("Adult"); break;
        case child: paxInfo.setPersType("Child"); break;
        case baby: paxInfo.setPersType("Infant"); break;
        default: throw Exception("DecodePerson failed");
      }
    }

    if (format.rule(_setTicketNumber))
      paxInfo.setTicketNumber(iPax->tkn.no);

    if (format.rule(_setFqts))
    {
      set<CheckIn::TPaxFQTItem> fqts;
      CheckIn::LoadPaxFQT(iPax->id, fqts);
      paxInfo.setFqts(fqts);
    }

    if (format.rule(_addMarkFlt))
    {
      //Marketing flights
      TMktFlight mktflt;
      mktflt.getByPaxId(iPax->id);
      if (!mktflt.empty())
      {
        const TAirlinesRow &mkt_airline = (const TAirlinesRow&)base_tables.get("airlines").get_row("code",mktflt.airline);
        if (mkt_airline.code_lat.empty())
          throw Exception("mkt_airline.code_lat empty (code=%s)",mkt_airline.code.c_str());
        string mkt_flt = IntToString(mktflt.flt_no);
        if (!mktflt.suffix.empty())
        {
          const TTripSuffixesRow &mkt_suffix = (const TTripSuffixesRow&)base_tables.get("trip_suffixes").get_row("code",mktflt.suffix);
          if (mkt_suffix.code_lat.empty())
            throw Exception("mkt_suffix.code_lat empty (code=%s)",mkt_suffix.code.c_str());
          mkt_flt = mkt_flt + mkt_suffix.code_lat;
        }
        if (iPax->status!=psCrew && (mkt_airline.code_lat != FPM.carrier() || mkt_flt != FPM.flight()))
          FPM.addMarkFlt(mkt_airline.code_lat, mkt_flt);
        else if (mkt_airline.code_lat != FCM.carrier() || mkt_flt != FCM.flight())
          FCM.addMarkFlt(mkt_airline.code_lat, mkt_flt);
      }
    }

    if (format.rule(_setSeats))
      paxInfo.setSeats(iPax->seats);

    if (format.rule(_setBagCount) && iPax->amount)
      paxInfo.setBagCount(iPax->amount);

    if (format.rule(_setBagWeight) && iPax->weight)
      paxInfo.setBagWeight(iPax->weight);

    string doc_surname;
    string doc_first_name;
    string doc_second_name;
    if (!iPax->doc.surname.empty())
    {
      doc_surname = transliter(iPax->doc.surname,1,1);
      doc_first_name = transliter(iPax->doc.first_name,1,1);
      doc_second_name = transliter(iPax->doc.second_name,1,1);
    }
    else
    {
      //в терминалах до версии 201107-0126021 невозможен контроль и ввод фамилии из документа
      doc_surname = transliter(iPax->surname,1,1);
      doc_first_name = transliter(iPax->name,1,1);
    }

    if (format.rule(_convertPaxNames))
      format.convert_pax_names(doc_first_name, doc_second_name);

    paxInfo.setSurname(doc_surname);
    paxInfo.setFirstName(doc_first_name);
    paxInfo.setSecondName(doc_second_name);

    paxInfo.setSex(format.Gender(iPax->gender));

    if (iPax->doc.birth_date!=NoExists)
      paxInfo.setBirthDate(iPax->doc.birth_date);

    if (format.rule(_setCBPPort) && route.country_regul_dep!=US_CUSTOMS_CODE)
      paxInfo.setCBPPort(route.airp_cbp_code_lat());

    paxInfo.setDepPort(route.airp_dep_code_lat());
    paxInfo.setArrPort(route.airp_arv_code_lat());
    paxInfo.setNationality(iPax->doc.nationality);

    if (iPax->status!=psCrew)
    {
      //PNR
      vector<TPnrAddrItem> pnrs;
      GetPaxPnrAddr(iPax->id,pnrs);
      if (!pnrs.empty()) paxInfo.setReservNum(convert_pnr_addr(pnrs.begin()->addr, 1));
    }

    string doc_type = iPax->doc_type_lat(); // throws
    string doc_no = iPax->doc.no;

    if (!APIS::isValidDocType(format.fmt, iPax->status, doc_type))
      doc_type.clear();

    if (!doc_type.empty() && format.rule(_processDocType))
      doc_type = format.process_doc_type(doc_type);

    if (format.rule(_processDocNumber))
      doc_no = format.process_doc_no(doc_no);

    if (format.check_doc_type_no(doc_type, doc_no))
    {
      paxInfo.setDocType(doc_type);
      paxInfo.setDocNumber(doc_no);
    }

    if (iPax->doc.expiry_date!=NoExists)
      paxInfo.setDocExpirateDate(iPax->doc.expiry_date);

    paxInfo.setDocCountry(iPax->doc.issue_country);

    if (format.rule(_docaD_US) && iPax->docaD)
    {
      if (iPax->status!=psCrew && route.country_regul_dep!=US_CUSTOMS_CODE)
      {
        paxInfo.setStreet(iPax->docaD.get().address);
        paxInfo.setCity(iPax->docaD.get().city);
        if (route.country_arv_code_lat!="US" || iPax->docaD.get().region.size()==2) //код штата для US
          paxInfo.setCountrySubEntityCode(iPax->docaD.get().region);
        paxInfo.setPostalCode(iPax->docaD.get().postal_code);
        paxInfo.setDestCountry(iPax->docaD.get().country);
      }
    }

    if (format.rule(_setResidCountry) && iPax->docaR)
      paxInfo.setResidCountry(iPax->docaR.get().country);

    if (format.rule(_docaR_US) && iPax->docaR)
    {
      if (iPax->status!=psCrew && route.country_regul_dep!=US_CUSTOMS_CODE)
      {
        paxInfo.setResidCountry(iPax->docaR.get().country);
      }
      if (iPax->status==psCrew)
      {
        paxInfo.setResidCountry(iPax->docaR.get().country);
        paxInfo.setStreet(iPax->docaR.get().address);
        paxInfo.setCity(iPax->docaR.get().city);
        paxInfo.setCountrySubEntityCode(iPax->docaR.get().region);
        paxInfo.setPostalCode(iPax->docaR.get().postal_code);
        paxInfo.setDestCountry(iPax->docaR.get().country);
      }
    }

    if (format.rule(_setBirthCountry) && iPax->docaB)
      paxInfo.setBirthCountry(iPax->docaB.get().country);

    if (format.rule(_docaB_US) && iPax->docaB && iPax->status==psCrew)
    {
      paxInfo.setBirthCity(iPax->docaB.get().city);
      paxInfo.setBirthRegion(iPax->docaB.get().region);
      paxInfo.setBirthCountry(iPax->docaB.get().country);
    }

    if (format.rule(_doco) && iPax->doco)
    {
      paxInfo.setDocoType(iPax->doco_type_lat());
      paxInfo.setDocoNumber(iPax->doco.get().no);
      paxInfo.setDocoCountry(iPax->doco.get().applic_country);
    }

    if (iPax->status != psCrew)
      FPM.addPassenger(paxInfo);
    else
      FCM.addPassenger(paxInfo);
  } // for iPax
}

/////////////////////////////////////////////////////////////////////////////////////////

void CreateEdiFile1(  const TApisRouteData& route,
                      const TAPISFormat& format,
                      const Paxlst::PaxlstInfo& FPM,
                      const Paxlst::PaxlstInfo& FCM,
                      vector< pair<string, string> >& files)
{
  for(int pass=0; pass<2; pass++)
  {
    const Paxlst::PaxlstInfo& paxlstInfo=(pass==0?FPM:FCM);

    if (!(route.task_name.empty() ||
          !route.use_us_customs_tasks ||
          (route.task_name==BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL && pass==0) ||
          (route.task_name==BEFORE_TAKEOFF_60_US_CUSTOMS_ARRIVAL && pass!=0)))
      continue;

    if (!paxlstInfo.passengersList().empty())
    {
      vector<string> parts;
      string file_extension;

      if (format.rule(_fileExtTXT))
        file_extension="TXT";
      else
        file_extension=(pass==0?"FPM":"FCM");

      if (format.rule(_fileSimplePush))
        parts.push_back(paxlstInfo.toEdiString());
      else
      {
        for(unsigned maxPaxPerString=MAX_PAX_PER_EDI_PART; maxPaxPerString>0; maxPaxPerString--)
        {
          parts=paxlstInfo.toEdiStrings(maxPaxPerString);
          vector<string>::const_iterator p=parts.begin();
          for(; p!=parts.end(); ++p)
            if (p->size()>MAX_LEN_OF_EDI_PART)
              break;
          if (p==parts.end())
            break;
        }
      }

      int part_num=parts.size()>1?1:0;
      for(vector<string>::const_iterator p=parts.begin(); p!=parts.end(); ++p, part_num++)
      {
        ostringstream file_name;
        string lst_type;
        if (format.rule(_lstTypeLetter))
          lst_type=(pass==0?"_P":"_C");
        file_name << format.dir
                  << "/"
                  << Paxlst::createEdiPaxlstFileName( route.airline_code_lat(),
                                                      route.flt_no,
                                                      route.suffix,
                                                      route.airp_dep_code_lat(),
                                                      route.airp_arv_code_lat(),
                                                      route.scd_out_local,
                                                      file_extension,
                                                      part_num,
                                                      lst_type);
        files.push_back( make_pair(file_name.str(), *p) );
      } // for p
    } // paxlstInfo
  } // for int pass
}

/////////////////////////////////////////////////////////////////////////////////////////

bool CreateEdiFile2(  const TApisRouteData& route,
                      const TAPISFormat& format,
                      const Paxlst::PaxlstInfo& FPM,
                      const Paxlst::PaxlstInfo& FCM,
                      string& text,
                      string& type,
                      map<string, string>& file_params)
{
  bool result = false;
  int passengers_count = FPM.passengersList().size();
  int crew_count = FCM.passengersList().size();
  // сформируем файл
  if ( format.rule(_file_XML_TR) && ( passengers_count || crew_count ))
  {
    XMLDoc doc;
    doc.set("FlightMessage");
    if (doc.docPtr()==NULL)
      throw EXCEPTIONS::Exception("CreateEdiFile2: CreateXMLDoc failed");
    xmlNodePtr apisNode=xmlDocGetRootElement(doc.docPtr());
    int version = 0;
#if !APIS_TEST
    if (get_trip_apis_param(route.dataset_point_id, "XML_TR", "version", version))
      version++;
    set_trip_apis_param(route.dataset_point_id, "XML_TR", "version", version);
#endif
    FPM.toXMLFormat(apisNode, passengers_count, crew_count, version);
    FCM.toXMLFormat(apisNode, passengers_count, crew_count, version);
    text = GetXMLDocText(doc.docPtr());
    type = APIS_TR;
  }
  else if ( format.rule(_file_LT) && passengers_count )
  {
    type = APIS_LT;
    text = FPM.toEdiString();
  }
  // положим апис в очередь на отправку
  if ( !text.empty() )
  {
    TFileQueue::add_sets_params(route.airp_dep_code(), route.airline_code(), IntToString(route.flt_no),
        OWN_POINT_ADDR(), type, 1, file_params);
    if(not file_params.empty())
    {
      file_params[ NS_PARAM_EVENT_ID1 ] = IntToString( route.dataset_point_id );
      file_params[ NS_PARAM_EVENT_TYPE ] = EncodeEventType( ASTRA::evtFlt );
#if !APIS_TEST
      TFileQueue::putFile(OWN_POINT_ADDR(), OWN_POINT_ADDR(),
          type, file_params, ConvertCodepage( text, "CP866", "UTF-8"));
      LEvntPrms params;
      params << PrmSmpl<string>("fmt", format.fmt) << PrmElem<string>("country_dep", etCountry, route.country_dep)
          << PrmElem<string>("airp_dep", etAirp, route.airp_dep_code())
          << PrmElem<string>("country_arv", etCountry, route.country_arv_code)
          << PrmElem<string>("airp_arv", etAirp, route.airp_arv_code());
      TReqInfo::Instance()->LocaleToLog("EVT.APIS_CREATED", params, evtFlt, route.dataset_point_id);
#endif
      result = true;
    }
  }
  return result;
}

/////////////////////////////////////////////////////////////////////////////////////////

void CreateTxt( const TApisRouteData& route,
                const TAPISFormat& format,
                TTxtDataFormatted& tdf)
{
  int count = 0;
  for ( list<TApisPaxData>::const_iterator iPax = route.lstPaxData.begin();
        iPax != route.lstPaxData.end();
        ++iPax)
  {
    if (iPax->status==psCrew && !format.rule(_notOmitCrew))
      continue;
    if (iPax->status!=psCrew && !iPax->pr_brd && route.final_apis)
      continue;
    if (omit_incomplete_apis(route.dataset_point_id, *iPax, format))
      continue;

    TPaxDataFormatted pdf;

    pdf.status = iPax->status;

    pdf.count_current = count;

    if (!iPax->doc.surname.empty())
    {
      pdf.doc_surname = transliter(iPax->doc.surname,1,1);
      pdf.doc_first_name = transliter(iPax->doc.first_name,1,1);
      pdf.doc_second_name = transliter(iPax->doc.second_name,1,1);
    }
    else
    {
      //в терминалах до версии 201107-0126021 невозможен контроль и ввод фамилии из документа
      pdf.doc_surname = transliter(iPax->surname,1,1);
      pdf.doc_first_name = transliter(iPax->name,1,1);
    }

    if (format.rule(_convertPaxNames))
      format.convert_pax_names(pdf.doc_first_name, pdf.doc_second_name);

    pdf.gender=format.Gender(iPax->gender);

    pdf.doc_type = iPax->doc_type_lat(); // throws
    pdf.doc_no = iPax->doc.no;

    if (!APIS::isValidDocType(format.fmt, iPax->status, pdf.doc_type))
      pdf.doc_type.clear();

    if (!pdf.doc_type.empty() && format.rule(_processDocType))
      pdf.doc_type = format.process_doc_type(pdf.doc_type);

    if (format.rule(_processDocNumber))
      pdf.doc_no = format.process_doc_no(pdf.doc_no);

    pdf.nationality = iPax->doc.nationality;
    pdf.issue_country = iPax->doc.issue_country;

    if (iPax->doc.birth_date!=NoExists && format.rule(_birth_date))
      pdf.birth_date = DateTimeToStr(iPax->doc.birth_date, format.DateTimeFormat(), true);

    if (iPax->doc.expiry_date!=NoExists && format.rule(_expiry_date))
      pdf.expiry_date = DateTimeToStr(iPax->doc.expiry_date, format.DateTimeFormat(), true);

    if (iPax->docaB && format.rule(_birth_country))
      pdf.birth_country = iPax->docaB.get().country;

    pdf.trip_type = "N";
    if (format.rule(_trip_type))
        pdf.trip_type = getTripType(iPax->status, iPax->grp_id,
            format.direction(route.country_dep), format.apis_country());

    pdf.airp_final_lat = iPax->airp_final_lat;
    pdf.airp_final_code = iPax->airp_final_code;

    if (format.rule(_airp_arv_code_lat))
      pdf.airp_arv_code_lat = route.airp_arv_code_lat(); // throws

    if (format.rule(_airp_dep_code_lat))
      pdf.airp_dep_code_lat = route.airp_dep_code_lat(); // throws

    // doco
    if (iPax->doco && format.rule(_doco))
    {
      pdf.doco_exists = iPax->doco != boost::none;
      pdf.doco_type = iPax->doco_type_lat(); // throws
      pdf.doco_no = iPax->doco.get().no;
      pdf.doco_applic_country = iPax->doco.get().applic_country;
    }

    tdf.lstPaxData.push_back(pdf);
    ++count;
  } // for iPax
  tdf.count_overall = count;
}

/////////////////////////////////////////////////////////////////////////////////////////

#if APIS_TEST
bool CreateApisFiles(const TApisDataset& dataset, TApisTestMap* test_map)
#else
bool CreateApisFiles(const TApisDataset& dataset)
#endif
{
#if APIS_TEST
  if (test_map == nullptr)
    return false;
#endif
  bool result = false;
  try
  {
    for (list<TApisRouteData>::const_iterator iRoute = dataset.lstRouteData.begin(); iRoute != dataset.lstRouteData.end(); ++iRoute)
    {
      for (list<TApisSetsData>::const_iterator iApis = iRoute->lstSetsData.begin(); iApis != iRoute->lstSetsData.end(); ++iApis)
      {
#if APIS_TEST
        test_map->try_key.set(iRoute->route_point_id, iApis->fmt);
#endif
        boost::scoped_ptr<const TAPISFormat> pFormat(SpawnAPISFormat(*iApis));
        // https://stackoverflow.com/questions/6718538/does-boostscoped-ptr-violate-the-guideline-of-logical-constness

        if ((iRoute->task_name==ON_CLOSE_CHECKIN && !pFormat->rule(_create_ON_CLOSE_CHECKIN)) ||
            (iRoute->task_name==ON_CLOSE_BOARDING && !pFormat->rule(_create_ON_CLOSE_BOARDING)) ||
            (iRoute->task_name==ON_TAKEOFF && pFormat->rule(_skip_ON_TAKEOFF)))
          continue;

        string lst_type_extra = pFormat->lst_type_extra(iRoute->final_apis);

        Paxlst::PaxlstInfo FPM(Paxlst::PaxlstInfo::FlightPassengerManifest, lst_type_extra);
        Paxlst::PaxlstInfo FCM(Paxlst::PaxlstInfo::FlightCrewManifest, lst_type_extra);

        vector< pair<string, string> > files;
        string text, type;
        map<string, string> file_params;

        TTxtDataFormatted tdf;
        ostringstream body, paxs_body, crew_body;
        ostringstream header, pax_header, crew_header;
        string file_name;

        switch (pFormat->format_type)
        {
          case _format_edi:
            CreateEdi(*iRoute, *pFormat, FPM, FCM);
            switch (pFormat->file_rule)
            {
              case _file_rule_1:
                CreateEdiFile1(*iRoute, *pFormat, FPM, FCM, files);
                break;
              case _file_rule_2:
                result = CreateEdiFile2(*iRoute, *pFormat, FPM, FCM, text, type, file_params);
                break;
              default:
                break;
            }
            break;
          case _format_txt:
            CreateTxt(*iRoute, *pFormat, tdf);
            pFormat->CreateTxtBodies(tdf, body, paxs_body, crew_body);
            if (iRoute->task_name.empty() ||
                !iRoute->use_us_customs_tasks ||
                iRoute->task_name==BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL)
            {
              file_name = pFormat->CreateFilename(*iRoute);
              pFormat->CreateHeaders(*iRoute, header, pax_header, crew_header, crew_body, tdf.count_overall);
              switch (pFormat->file_rule)
              {
                case _file_rule_txt_AE_TH:
                  if ( !paxs_body.str().empty() && !pax_header.str().empty() )
                  {
                    ostringstream paxs;
                    paxs << pax_header.str() << ENDL << pFormat->HeaderStart_AE_TH()  << ENDL
                          << paxs_body.str() << pFormat->HeaderEnd_AE_TH() << ENDL;
                    files.push_back( make_pair( file_name + pFormat->NameTrailPax_AE_TH(), paxs.str() ) );
                  }
                  if ( !crew_body.str().empty() && !crew_header.str().empty() )
                  {
                    ostringstream crew;
                    crew << crew_header.str() << ENDL << pFormat->HeaderStart_AE_TH()  << ENDL
                          << crew_body.str() << pFormat->HeaderEnd_AE_TH() << ENDL;
                    files.push_back( make_pair( file_name + pFormat->NameTrailCrew_AE_TH(), crew.str() ) );
                  }
                  break;
                case _file_rule_txt_common:
                  if(!file_name.empty() && !header.str().empty() && !body.str().empty())
                    files.push_back( make_pair(file_name, string(header.str()).append(body.str())) );
                  break;
                default:
                  break;
              }
            }
            break;
          default:
            break;
        }
#if APIS_TEST
        apis_test_key key(iRoute->route_point_id, iApis->fmt);
        apis_test_value value(files, text, type, file_params);
        test_map->insert(make_pair(key, value));
#else
        if (!files.empty())
        {
#if APIS_TEST_CREATE_FILES
          LogTrace(TRACE5) << "CREATING FILES ON DISK \"" << pFormat->fmt << "\"";
#endif
          for(vector< pair<string, string> >::const_iterator iFile=files.begin();iFile!=files.end();++iFile)
          {
            ofstream f;
            f.open(iFile->first.c_str());
            if (!f.is_open()) throw Exception("Can't open file '%s'",iFile->first.c_str());
            try
            {
              f << iFile->second;
              f.close();
            }
            catch(...)
            {
              try { f.close(); } catch( ... ) { };
              try
              {
                //в случае ошибки запишем пустой файл
                f.open(iFile->first.c_str());
                if (f.is_open()) f.close();
              }
              catch( ... ) { };
              throw;
            }
          }

          LEvntPrms params;
          params << PrmSmpl<string>("fmt", pFormat->fmt) << PrmElem<string>("country_dep", etCountry, iRoute->country_dep)
                  << PrmElem<string>("airp_dep", etAirp, iRoute->airp_dep_code())
                  << PrmElem<string>("country_arv", etCountry, iRoute->country_arv_code)
                  << PrmElem<string>("airp_arv", etAirp, iRoute->airp_arv_code());
          TReqInfo::Instance()->LocaleToLog("EVT.APIS_CREATED", params, evtFlt, iRoute->dataset_point_id);
          result = true;
        }
#endif
      } // for iApis
    } // for iRoute
  } // try
  catch(Exception &E)
  {
    throw Exception("CreateApisFiles: %s",E.what());
  }
  return result;
}

#if USE_NEW_CREATE_APIS
// замена старой функции
bool create_apis_file(int point_id, const string& task_name)
#else
bool CreateApisFiles(int point_id, const string& task_name)
#endif
{
  TApisDataset dataset;
  if (!dataset.FromDB(point_id, task_name))
    return false;
  bool result = CreateApisFiles(dataset);
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////

void WriteCmpFile(ofstream& f, const apis_test_value& v)
{
  f << "FILES:" << endl;
  for (auto i : v.files)
  {
    f << i.first << endl;
    f << i.second << endl;
  }
  f << "TEXT:" << endl;
  f << v.text << endl;
  f << "TYPE:" << endl;
  f << v.type << endl;
  f << "FILE_PARAMS:" << endl;
  for (auto i : v.file_params)
  {
    f << i.first << endl;
    f << i.second << endl;
  }
}

void DumpDiff(  int point_id,
                string task_name,
                const apis_test_key& key,
                const apis_test_value& val_old,
                const apis_test_value& val_new)
{
  ofstream f_old, f_new;
  try
  {
    string dir = "./compare/";
    ostringstream fname_old, fname_new;
    fname_old << dir << "cmp_" << point_id << "_" << task_name << "_" << key.route_point_id << "_" << key.format << "_old.txt";
    fname_new << dir << "cmp_" << point_id << "_" << task_name << "_" << key.route_point_id << "_" << key.format << "_new.txt";

    f_old.open(fname_old.str().c_str(), ios_base::trunc|ios_base::out);
    if (!f_old.is_open())
      throw Exception("Can't open file '%s'",fname_old.str().c_str());
    f_new.open(fname_new.str().c_str(), ios_base::trunc|ios_base::out);
    if (!f_new.is_open())
      throw Exception("Can't open file '%s'",fname_new.str().c_str());

    WriteCmpFile(f_old, val_old);
    WriteCmpFile(f_new, val_new);

    f_old.close();
    f_new.close();
  }
  catch (Exception e)
  {
    LogTrace(TRACE5) << "ERROR write files: " << e.what();
    try { f_old.close(); } catch (...) { }
    try { f_new.close(); } catch (...) { }
  }
}

string TruncExceptionString(const string& s)
{
  stringstream ss(s);
  list<string> lst;
  string item;

  while (getline(ss, item, ':'))
    lst.push_back(item);

  if (!lst.empty())
    return lst.back();
  else
    return "";
}

/////////////////////////////////////////////////////////////////////////////////////////

int apis_test(int argc, char **argv)
{
  if(edifact::init_edifact() < 0)
    throw EXCEPTIONS::Exception("'init_edifact' error!");
  const bool DUMP_EQUAL = argc > 1;

  list<int> lst_point_id;
  TQuery PointIdQry(&OraSession);
  PointIdQry.Clear();
  PointIdQry.SQLText=
  "SELECT point_id FROM points WHERE airline IS NOT NULL AND PR_DEL=0 AND "
  "SCD_OUT between to_date('01.08.16 00:00', 'DD.MM.YY HH24:MI') AND to_date('01.08.17 00:00','DD.MM.YY HH24:MI')";
  PointIdQry.Execute();
  for(; !PointIdQry.Eof; PointIdQry.Next()) lst_point_id.push_back(PointIdQry.FieldAsInteger("point_id"));

  // list<string> lst_task_name = {"ON_TAKEOFF"};
  list<string> lst_task_name = {BEFORE_TAKEOFF_30_US_CUSTOMS_ARRIVAL, BEFORE_TAKEOFF_60_US_CUSTOMS_ARRIVAL, ON_TAKEOFF, ON_CLOSE_CHECKIN, ON_CLOSE_BOARDING};

  int iteration = 0;
  for (int point_id : lst_point_id)
  //for (int point_id : {9138627})
  {
    for (string task_name : lst_task_name)
    {
      ++iteration;
      nosir_wait(iteration, false, 2, 0);

#if APIS_TEST
      ostringstream test_data;
      test_data << "point " << point_id << " task \"" << task_name << "\"";
      TApisTestMap map_old, map_new;

      // TEST OLD
      LogTrace(TRACE5) << "STARTING OLD " << test_data.str();
      try
      {
        create_apis_file(point_id, task_name, &map_old);
      }
      catch (std::exception& E)
      {
        map_old.exception = true;
        map_old.str_exception = E.what();
      }

      // TEST NEW
      LogTrace(TRACE5) << "STARTING NEW " << test_data.str();
      TApisDataset dataset;
      try
      {
        dataset.FromDB(point_id, task_name, &map_new);
        CreateApisFiles(dataset, &map_new);
      }
      catch (std::exception& E)
      {
        map_new.exception = true;
        map_new.str_exception = E.what();
      }

      // COMPARE MAPS
      bool compare_keys = false;

      if (map_old.size() == 0 && map_new.size() == 0)
      {
        LogTrace(TRACE5) << "MAPS EMPTY " << test_data.str();
      }
      else if (map_old.size() == map_new.size())
      {
        LogTrace(TRACE5) << "MAPS EQUAL SIZE " << map_old.size() << " " << test_data.str();
        compare_keys = true;
      }
      else
      {
        LogTrace(TRACE5) << "MAPS DIFFERENT SIZE old " << map_old.size() << " new " << map_new.size() << " " << test_data.str();
        compare_keys = true;
      }

      if (map_old.exception || map_new.exception)
      {
        string str_trunc_old = TruncExceptionString(map_old.str_exception);
        string str_trunc_new = TruncExceptionString(map_new.str_exception);
        string str_result;
        if (map_old.exception == map_new.exception && str_trunc_old == str_trunc_new)
          str_result = "EXCEPTION EQUAL";
        else
          str_result = "EXCEPTION DIFFERENT";
        LogTrace(TRACE5) << str_result  << " old " << map_old.exception << " \"" << map_old.str_exception << "\""
                                        << " new " << map_new.exception << " \"" << map_new.str_exception << "\"";
        if (map_old.exception) LogTrace(TRACE5) << "OLD TRY KEY " << map_old.try_key.ToString();
        if (map_new.exception) LogTrace(TRACE5) << "NEW TRY KEY " << map_new.try_key.ToString();
      }

      if (compare_keys)
      {
        set<apis_test_key, apis_test_key_less> keys_old, keys_new;
        for (auto i : map_old) keys_old.insert(i.first);
        for (auto i : map_new) keys_new.insert(i.first);
        if (keys_old == keys_new)
          LogTrace(TRACE5) << "KEYS EQUAL";
        else
        {
          LogTrace(TRACE5) << "KEYS DIFFERENT";
          for (auto key : keys_old)
            LogTrace(TRACE5) << "OLD KEY " << key.ToString();
          for (auto key : keys_new)
            LogTrace(TRACE5) << "NEW KEY " << key.ToString();
        }
      }

      // COMPARE VALUES
      // сравниваем только по ключам, общим для обоих мап
      set<apis_test_key, apis_test_key_less> keys;
      for (auto i : map_old) keys.insert(i.first);
      for (auto i : map_new) keys.insert(i.first);
      unsigned int cnt = 0, cnt_empty = 0, cnt_eq = 0, cnt_dif = 0;
      for (auto key : keys)
      {
        if (!map_old.count(key) || !map_new.count(key))
          continue;

        ++cnt;
        const apis_test_value& val_old = map_old.find(key)->second;
        const apis_test_value& val_new = map_new.find(key)->second;

        if (val_old.PayloadSize() == 0 && val_new.PayloadSize() == 0)
        {
          ++cnt_empty;
        }
        else if (val_old == val_new)
        {
          ++cnt_eq;
          LogTrace(TRACE5) << "VALUES EQUAL " << key.ToString() << ", " << cnt;
          if (DUMP_EQUAL)
            DumpDiff(point_id, task_name, key, val_old, val_new);
        }
        else
        {
          ++cnt_dif;
          LogTrace(TRACE5) << "VALUES DIFFERENT " << key.ToString() << ", " << cnt;
          DumpDiff(point_id, task_name, key, val_old, val_new);
        }
      }
      LogTrace(TRACE5) << "OVERALL " << cnt << ", empty " << cnt_empty
                        << ", equal " << cnt_eq << ", different " << cnt_dif;
#else

#if APIS_TEST_CREATE_FILES
      try
      {
        CreateApisFiles(point_id, task_name);
      }
      catch (std::exception& E)
      {
        LogTrace(TRACE5) << "EXCEPTION: " << E.what();
      }
#endif

#endif
    } // for (string task_name : lst_task_name)
  } // for (int point_id : lst_point_id)
  return 1;
}

