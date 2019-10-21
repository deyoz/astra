#include "apis_creator.h"
#include <boost/scoped_ptr.hpp>
#include <fstream>
#include "obrnosir.h"
#include "franchise.h"
#include "exch_checkin_result.h"
#include "jms/jms.hpp"
#include "apis_settings.h"

#define NICKNAME "GRISHA"
#include "serverlib/test.h"
#include "serverlib/slogger.h"

// TODO приведение типов в стиле C++
// TODO убрать все дубликаты относительно apis.cc

#define MAX_PAX_PER_EDI_PART 15
#define MAX_LEN_OF_EDI_PART 3000

#define APIS_TEST_ALL_FORMATS 0
#define APIS_TEST_IGNORE_EMPTY 1

bool TApisDataset::FromDB(int point_id, const string& task_name, TApisTestMap* test_map)
{
  try
  {
#if APIS_TEST
    if (test_map == nullptr)
      return false;
#endif
    TAdvTripInfo fltInfo;
    if (!fltInfo.getByPointId(point_id, FlightProps(FlightProps::NotCancelled, FlightProps::WithCheckIn))) return false;

    // Франчайзинг
    Franchise::TProp franchise_prop;
    if (franchise_prop.get(point_id, Franchise::TPropType::apis))
    {
      if (franchise_prop.val == Franchise::pvNo)
      {
        fltInfo.airline = franchise_prop.franchisee.airline;
        fltInfo.flt_no = franchise_prop.franchisee.flt_no;
        fltInfo.suffix = franchise_prop.franchisee.suffix;
      }
      else /*if (franchise_prop.val == Franchise::pvYes)*/
      {
        fltInfo.airline = franchise_prop.oper.airline;
        fltInfo.flt_no = franchise_prop.oper.flt_no;
        fltInfo.suffix = franchise_prop.oper.suffix;
      }
    }

    TAdvTripRoute routeAfterWithCurrent;
    routeAfterWithCurrent.GetRouteAfter(fltInfo, trtWithCurrent, trtNotCancelled);
    TAdvTripRoute routeAfterWithoutCurrent(routeAfterWithCurrent);
    if (!routeAfterWithoutCurrent.empty() && routeAfterWithoutCurrent.begin()->point_id == point_id)
      routeAfterWithoutCurrent.erase(routeAfterWithoutCurrent.begin());

    TQuery PaxQry(&OraSession);
    PaxQry.SQLText=
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

    map<string /*country_regul_arv*/, string /*first airp_arv*/> CBPAirps;

    for(TAdvTripRoute::const_iterator iRoute = routeAfterWithCurrent.begin(); iRoute != routeAfterWithCurrent.end(); ++iRoute)
    {
      if (iRoute->point_id == point_id) continue; //пропускаем пункт вылета
#if APIS_TEST
      test_map->try_key.set(iRoute->point_id, "");
#endif
      TApisRouteData rd;
      rd.depInfo=fltInfo;
      for(TAdvTripRoute::const_iterator i=routeAfterWithCurrent.begin(); i!=iRoute+1; ++i)
        rd.paxLegs.push_back(*i);
      rd.task_name = task_name;
      rd.country_regul_dep = APIS::getCustomsRegulCountry(rd.country_dep());
      rd.country_regul_arv = APIS::getCustomsRegulCountry(rd.country_arv());
      rd.use_us_customs_tasks = rd.country_regul_dep==US_CUSTOMS_CODE || rd.country_regul_arv==US_CUSTOMS_CODE;
      map<string, string>::iterator iCBPAirp = CBPAirps.find(rd.country_regul_arv);
      if (iCBPAirp==CBPAirps.end())
        iCBPAirp=CBPAirps.emplace(rd.country_regul_arv, rd.arvInfo().airp).first;
      if (iCBPAirp==CBPAirps.end())
        throw Exception("iCBPAirp==CBPAirps.end()");

      if  (!( rd.task_name.empty()
              ||
              (rd.use_us_customs_tasks &&
              (rd.task_name==BEFORE_TAKEOFF_30 || rd.task_name==BEFORE_TAKEOFF_60 ))
              ||
              (!rd.use_us_customs_tasks &&
              (rd.task_name==ON_TAKEOFF ||
               rd.task_name==ON_CLOSE_CHECKIN ||
               rd.task_name==ON_CLOSE_BOARDING ||
               rd.task_name==ON_FLIGHT_CANCEL)) ))
        continue;

      //получим информацию по настройке APIS
#if APIS_TEST_ALL_FORMATS
      APIS::Settings pattern("РФ", "", "ESAPIS:ZZ", "AIR EUROPA:UX", TRANSPORT_TYPE_FILE, "mvd_czech_edi");
      rd.lstSetsData.getForTesting(pattern);
#else
      rd.lstSetsData.getByCountries(rd.depInfo.airline, rd.country_dep(), rd.country_arv());
#endif
      if (rd.lstSetsData.empty()) continue;

      rd.scd_out_dep_local(); // специально вызываем чтобы свалиться в exception, если NoExists
      rd.scd_in_arv_local();  // специально вызываем чтобы свалиться в exception, если NoExists

      rd.airp_cbp = iCBPAirp->second;

      //Flight legs
      rd.allLegs.GetRouteBefore(rd.depInfo, trtWithCurrent, trtNotCancelled);
      rd.allLegs.insert(rd.allLegs.end(), routeAfterWithoutCurrent.begin(), routeAfterWithoutCurrent.end());

#if APIS_TEST
      for(const auto& i : rd.lstSetsData)
      {
        const APIS::Settings& settings=i.second;
        test_map->try_key.set(iRoute->point_id, settings.format());

      }
#endif

      PaxQry.SetVariable("point_arv",rd.arvInfo().point_id);
      PaxQry.Execute();

      for (;!PaxQry.Eof;PaxQry.Next())
      {
        TApisPaxData pax;
        pax.fromDB(PaxQry);

        pax.status = DecodePaxStatus(PaxQry.FieldAsString("status"));

        pax.airp_arv=rd.arvInfo().airp;
        pax.airp_arv_final=PaxQry.FieldAsString("airp_final");
        if (pax.airp_arv_final.empty()) pax.airp_arv_final=pax.airp_arv;

        LoadPaxDoc(pax.id, pax.doc);
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

        multiset<TBagTagNumber> tags;
        GetTagsByPool(pax.grp_id, pax.bag_pool_num, tags, false);
        FlattenBagTags(tags, pax.tags);

        rd.lstPaxData.push_back(pax);
      } // for PaxQry
      lstRouteData.push_back(rd);
    } // for TTripRoute

    mergeRouteData();
  } // try
  catch(const Exception &E)
  {
    throw Exception("TApisCreator::FromDB: %s",E.what());
  }
  return true;
}

bool TApisRouteData::moveFrom(const APIS::Settings& settings, TApisRouteData& data)
{
  if (!data.lstSetsData.settingsExists(settings)) return false;

  if (depInfo.point_id==data.depInfo.point_id &&
      task_name==data.task_name &&
      lstSetsData.size()==1 && lstSetsData.begin()->second==settings)
  {
    LogTrace(TRACE5) << __func__ << ": " << settings.traceStr() << " "
                     << data.airp_dep_code_lat() << "-" << data.airp_arv_code_lat()
                     << " -> "
                     << airp_dep_code_lat() << "-" << airp_arv_code_lat();

    lstPaxData.insert(lstPaxData.end(), data.lstPaxData.begin(), data.lstPaxData.end());
    data.lstSetsData.erase(settings);
    return true;
  }

  return false;
}

bool TApisDataset::equalSettingsFound(const APIS::Settings& settings) const
{
  int count=0;
  for(const TApisRouteData& data: lstRouteData)
    if (data.lstSetsData.settingsExists(settings)) count++;
  return count>1;
}

bool TApisDataset::mergeRouteData()
{
  list<TApisRouteData> lstRouteData2;

  for(auto r=lstRouteData.rbegin(); r!=lstRouteData.rend(); ++r)
    for(auto s=r->lstSetsData.begin(); s!=r->lstSetsData.end();)
    {
      bool removeCurrSettings=false;

      const APIS::Settings& settings=s->second;
      std::unique_ptr<const TAPISFormat> pFormat(SpawnAPISFormat(settings));

      if (pFormat->rule(r_setPaxLegs) && equalSettingsFound(settings))
      {
        auto rNew=r;
        if (r->lstSetsData.size()!=1)
        {
          //создаем новый TApisRouteData и помещаем в lstRouteData2
          lstRouteData2.push_back(*r);
          rNew=lstRouteData2.rbegin();
          rNew->lstSetsData.clear();
          rNew->lstSetsData.add(settings);
          removeCurrSettings=true;

          LogTrace(TRACE5) << __func__ << ": create new " << settings.traceStr() << " "
                           << rNew->airp_dep_code_lat() << "-" << rNew->airp_arv_code_lat();
        }

        for(auto r2=r; r2!=lstRouteData.rend(); ++r2)
        {
          if (r2==r) continue;
          rNew->moveFrom(settings, *r2);
        }
      }

      if (removeCurrSettings)
        s=r->lstSetsData.erase(s);
      else
        ++s;
    }

  lstRouteData.insert(lstRouteData.end(), lstRouteData2.begin(), lstRouteData2.end());

  return false;
}

//---------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------

void createFlightStops( const TApisRouteData& route,
                        const TAPISFormat& format,
                        Paxlst::PaxlstInfo& paxlstInfo)
{
  if (format.rule(r_setPaxLegs))
  {
    std::string priorCountry;
    vector<Paxlst::FlightStops> routeParts;
    for(const TAdvTripRouteItem& i : route.paxLegs)
    {
      std::string currCountry=getCountryByAirp(i.airp).code;
      if (priorCountry.empty() ||
          (priorCountry==format.settings.countryControl()) != (currCountry==format.settings.countryControl()))
        routeParts.emplace_back();
      Paxlst::FlightStops& flightStops = routeParts.back();
      flightStops.emplace_back(TApisRouteData::airp_code_lat(i.airp),
                               i.scd_in_local(),
                               i.scd_out_local());
      priorCountry=currCountry;
    }

    paxlstInfo.stopsBeforeBorder().clear();
    paxlstInfo.stopsAfterBorder().clear();
    vector<Paxlst::FlightStops>::const_iterator part=routeParts.begin();
    if (part!=routeParts.end())
      paxlstInfo.stopsBeforeBorder()=*(part++);
    if (part!=routeParts.end())
      paxlstInfo.stopsAfterBorder()=*part;
  }
  else
  {
    paxlstInfo.setCrossBorderFlightStops(route.airp_dep_code_lat(),
                                         route.scd_out_dep_local(),
                                         route.airp_arv_code_lat(),
                                         route.scd_in_arv_local());
  }
}

void CreateEdi(const TApisRouteData& route,
               const TAPISFormat& format,
               Paxlst::PaxlstInfo& FPM,
               Paxlst::PaxlstInfo& FCM,
               const GetPaxlstInfoHandler& getPaxlstInfoHandler)
{
  for(int pass=0; pass<2; pass++)
  {
    Paxlst::PaxlstInfo& paxlstInfo=(pass==0?FPM:FCM);

    paxlstInfo.settings().setRespAgnCode(format.respAgnCode());
    paxlstInfo.settings().setAppRef(format.appRef());
    paxlstInfo.settings().setMesRelNum(format.mesRelNum());
    paxlstInfo.settings().setMesAssCode(format.mesAssCode());
    if (format.rule(r_setUnhNumber))
      paxlstInfo.settings().set_unh_number(format.unhNumber());
    paxlstInfo.settings().setViewUNGandUNE(format.viewUNGandUNE());


    if (format.rule(r_view_RFF_TN)) paxlstInfo.settings().set_view_RFF_TN(true);

    APIS::AirlineOfficeList offices;
    offices.get(route.depInfo.airline, format.settings.countryControl());
    if (!offices.empty())
    {
      const APIS::AirlineOfficeInfo& info=offices.front();
      paxlstInfo.setPartyName(info.contactName());
      paxlstInfo.setPhone(format.ProcessPhoneFax(info.phone()));
      paxlstInfo.setFax(format.ProcessPhoneFax(info.fax()));
    }

    if (format.rule(r_notSetSenderRecipient))
    {
      paxlstInfo.setSenderCarrierCode(route.airline_code_lat());
    }
    else
    {
      paxlstInfo.setSenderName(format.settings.ediOwnAddr());
      paxlstInfo.setSenderCarrierCode(format.settings.ediOwnAddrExt());
      paxlstInfo.setRecipientName(format.settings.ediAddr());
      paxlstInfo.setRecipientCarrierCode(format.settings.ediAddrExt());
    }

    ostringstream flight;
    if (!format.rule(r_omitAirlineCode))
      flight << route.airline_code_lat();
    flight << route.flt_no() << route.suffix_code_lat();
    string iataCode;
    switch (format.IataCodeType())
    {
      case iata_code_UK:
        //"[flight][scd_in[yyyymmddhhnn]]"
        iataCode=Paxlst::createIataCode(flight.str(), route.scd_in_arv_local(), "yyyymmddhhnn");
        break;
      case iata_code_TR:
        iataCode=Paxlst::createIataCode(route.airline_code_lat() + flight.str(), route.scd_in_arv_local(), "/yymmdd/hhnn");
        break;
      case iata_code_DE:
        iataCode=Paxlst::createIataCode(flight.str(), route.scd_in_arv_local(), "yymmddhhnnss");
        break;
      case iata_code_default:
      default:
        iataCode=Paxlst::createIataCode(flight.str(), route.scd_in_arv_local(), "/yymmdd/hhnn");
        break;
    }
    // LogTrace(TRACE5) << "iataCode \"" << iataCode << "\" type " << format.IataCodeType();
    paxlstInfo.setIataCode( iataCode );

    if (format.rule(r_setCarrier))
      paxlstInfo.setCarrier(route.airline_code_lat());

    paxlstInfo.setFlight(flight.str());
    createFlightStops(route, format, paxlstInfo);

    if (format.rule(r_setFltLegs))
    {
      FlightLegs legs;
      for(const TAdvTripRouteItem& leg : route.allLegs)
      {
        legs.emplace_back(TApisRouteData::airp_code_lat(leg.airp),
                          TApisRouteData::country_code_iso(leg.airp),
                          leg.scd_in_local(),
                          leg.scd_out_local());
      }
      legs.FillLocQualifier();
      paxlstInfo.setFltLegs(legs);
    }
  } // for int pass

  for ( TApisPaxDataList::const_iterator iPax = route.lstPaxData.begin();
        iPax != route.lstPaxData.end();
        ++iPax)
  {
    Paxlst::PaxlstInfo& paxlstInfo=getPaxlstInfoHandler(route, format, *iPax, FPM, FCM);

    if (paxlstInfo.passengersListAlwaysEmpty()) continue;

    Paxlst::PassengerInfo paxInfo;

    if (iPax->status==psCrew && !format.rule(r_notOmitCrew))
      continue;
    if (iPax->status!=psCrew && format.rule(r_omitPassengers))
      continue;
    if (iPax->status!=psCrew && !iPax->pr_brd && route.final_apis())
      continue;
    if (omit_incomplete_apis(route.depInfo.point_id, *iPax, format))
      continue;

    if (format.rule(r_setPrBrd))
      paxInfo.setPrBrd(iPax->pr_brd);

    if (format.rule(r_setGoShow))
      paxInfo.setGoShow(iPax->status==psGoshow);

    if (format.rule(r_setPersType))
    {
      switch(iPax->pers_type)
      {
        case adult: paxInfo.setPersType("Adult"); break;
        case child: paxInfo.setPersType("Child"); break;
        case baby: paxInfo.setPersType("Infant"); break;
        default: throw Exception("DecodePerson failed");
      }
    }

    if (format.rule(r_setTicketNumber))
      paxInfo.setTicketNumber(format.process_tkn_no(iPax->tkn));

    if (format.rule(r_setFqts))
    {
      set<CheckIn::TPaxFQTItem> fqts;
      CheckIn::LoadPaxFQT(iPax->id, fqts);
      paxInfo.setFqts(fqts);
    }

    if (format.rule(r_addMarkFlt))
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
        if (mkt_airline.code_lat != paxlstInfo.carrier() || mkt_flt != paxlstInfo.flight())
          paxlstInfo.addMarkFlt(mkt_airline.code_lat, mkt_flt);
      }
    }

    if (format.rule(r_setSeats))
      paxInfo.setSeats(iPax->seats);

    if (format.rule(r_setBagCount) && iPax->amount)
      paxInfo.setBagCount(iPax->amount);

    if (format.rule(r_setBagWeight) && iPax->weight)
      paxInfo.setBagWeight(iPax->weight);

    if (format.rule(r_bagTagSerials))
      paxInfo.setBagTags(iPax->tags);

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

    if (format.rule(r_convertPaxNames))
      format.convert_pax_names(doc_first_name, doc_second_name);

    paxInfo.setSurname(doc_surname);
    paxInfo.setFirstName(doc_first_name);
    paxInfo.setSecondName(doc_second_name);

    paxInfo.setSex(format.Gender(iPax->gender));

    if (iPax->doc.birth_date!=NoExists)
      paxInfo.setBirthDate(iPax->doc.birth_date);

    if (format.rule(r_setCBPPort) && format.NeedCBPPort(route.country_regul_dep))
      paxInfo.setCBPPort(route.airp_cbp_code_lat());

    paxInfo.setDepPort(route.airp_dep_code_lat());
    paxInfo.setArrPort(iPax->airp_arv_code_lat());
    paxInfo.setNationality(format.ConvertCountry(iPax->doc.nationality));

    if (iPax->status!=psCrew)
    {
      //PNR
      string pnr_addr=TPnrAddrs().firstAddrByPaxId(iPax->id, TPnrAddrInfo::AddrOnly);
      if (!pnr_addr.empty())
        paxInfo.setReservNum(convert_pnr_addr(pnr_addr, 1));
    }

    if (format.rule(r_setPaxReference))
      paxInfo.setPaxRef(IntToString(iPax->id));

    string doc_type = iPax->doc_type_lat(); // throws
    string doc_no = iPax->doc.no;

    if (!APIS::isValidDocType(format.settings.format(), iPax->status, doc_type))
      doc_type.clear();

    if (!doc_type.empty() && format.rule(r_processDocType))
      doc_type = format.process_doc_type(doc_type);

    if (format.rule(r_processDocNumber))
      doc_no = format.process_doc_no(doc_no);

    if (format.check_doc_type_no(doc_type, doc_no))
    {
      paxInfo.setDocType(doc_type);
      paxInfo.setDocNumber(doc_no);
    }

    if (iPax->doc.expiry_date!=NoExists)
      paxInfo.setDocExpirateDate(iPax->doc.expiry_date);

    paxInfo.setDocCountry(format.ConvertCountry(iPax->doc.issue_country));

    if (format.rule(r_docaD_US) && iPax->docaD)
    {
      if (iPax->status!=psCrew && route.country_regul_dep!=US_CUSTOMS_CODE)
      {
        paxInfo.setStreet(iPax->docaD.get().address);
        paxInfo.setCity(iPax->docaD.get().city);
        if (route.country_arv_code_lat()!="US" || iPax->docaD.get().region.size()==2) //код штата для US
          paxInfo.setCountrySubEntityCode(iPax->docaD.get().region);
        paxInfo.setPostalCode(iPax->docaD.get().postal_code);
        paxInfo.setDestCountry(format.ConvertCountry(iPax->docaD.get().country));
      }
    }

    if (format.rule(r_setResidCountry) && iPax->docaR)
      paxInfo.setResidCountry(format.ConvertCountry(iPax->docaR.get().country));

    if (format.rule(r_docaR_US) && iPax->docaR)
    {
      if (iPax->status!=psCrew && route.country_regul_dep!=US_CUSTOMS_CODE)
      {
        paxInfo.setResidCountry(format.ConvertCountry(iPax->docaR.get().country));
      }
      if (iPax->status==psCrew)
      {
        paxInfo.setResidCountry(format.ConvertCountry(iPax->docaR.get().country));
        paxInfo.setStreet(iPax->docaR.get().address);
        paxInfo.setCity(iPax->docaR.get().city);
        paxInfo.setCountrySubEntityCode(iPax->docaR.get().region);
        paxInfo.setPostalCode(iPax->docaR.get().postal_code);
        paxInfo.setDestCountry(format.ConvertCountry(iPax->docaR.get().country)); // docaD?
      }
    }

    if (format.rule(r_setBirthCountry) && iPax->docaB)
      paxInfo.setBirthCountry(format.ConvertCountry(iPax->docaB.get().country));

    if (format.rule(r_docaB_US) && iPax->docaB && iPax->status==psCrew)
    {
      paxInfo.setBirthCity(iPax->docaB.get().city);
      paxInfo.setBirthRegion(iPax->docaB.get().region);
      paxInfo.setBirthCountry(format.ConvertCountry(iPax->docaB.get().country));
    }

    if (format.rule(r_doco) && iPax->doco)
    {
      paxInfo.setDocoType(iPax->doco_type_lat());
      paxInfo.setDocoNumber(iPax->doco.get().no);
      if (format.rule(r_issueCountryInsteadApplicCountry))
      {
        TElemFmt fmt;
        paxInfo.setDocoCountry(format.ConvertCountry(issuePlaceToPaxDocCountryId(iPax->doco.get().issue_place, fmt)));
      }
      else
        paxInfo.setDocoCountry(format.ConvertCountry(iPax->doco.get().applic_country));
      if (iPax->doco.get().expiry_date!=NoExists)
        paxInfo.setDocoExpirateDate(iPax->doco.get().expiry_date);

    }

    paxInfo.setProcInfo(iPax->processingIndicator);

    paxlstInfo.addPassenger(paxInfo);
  } // for iPax
}

//---------------------------------------------------------------------------------------

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
          (route.task_name==BEFORE_TAKEOFF_30 && pass==0) ||
          (route.task_name==BEFORE_TAKEOFF_60 && pass!=0)))
      continue;

    if (!paxlstInfo.passengersList().empty() ||
        paxlstInfo.passengersListMayBeEmpty() ||
        paxlstInfo.passengersListAlwaysEmpty())
    {
      vector<string> parts;
      string file_extension;

      if (format.rule(r_fileExtTXT))
        file_extension="TXT";
      else
        file_extension=(pass==0?"FPM":"FCM");

      if (paxlstInfo.passengersList().empty() ||
          format.rule(r_fileSimplePush))
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
        if (format.rule(r_lstTypeLetter))
          lst_type=(pass==0?"_P":"_C");
        file_name << format.dir()
                  << "/"
                  << Paxlst::createEdiPaxlstFileName( route.airline_code_lat(),
                                                      route.flt_no(),
                                                      route.suffix_code_lat(),
                                                      route.airp_dep_code_lat(),
                                                      route.airp_arv_code_lat(),
                                                      route.scd_out_dep_local(),
                                                      file_extension,
                                                      part_num,
                                                      lst_type);
        files.push_back( make_pair(file_name.str(), *p) );
      } // for p
    } // paxlstInfo
  } // for int pass
}

//---------------------------------------------------------------------------------------

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
  boost::optional<int> XML_TR_version;
  if ( format.rule(r_file_XML_TR) && ( passengers_count || crew_count ))
  {
    XMLDoc doc;
    doc.set("FlightMessage");
    if (doc.docPtr()==NULL)
      throw EXCEPTIONS::Exception("CreateEdiFile2: CreateXMLDoc failed");
    xmlNodePtr apisNode=xmlDocGetRootElement(doc.docPtr());
    XML_TR_version = 0;
#if !APIS_TEST
    if (get_trip_apis_param(route.depInfo.point_id, "XML_TR", "version", XML_TR_version.get()))
      XML_TR_version.get()++;
#endif
    FPM.toXMLFormat(apisNode, passengers_count, crew_count, XML_TR_version.get());
    FCM.toXMLFormat(apisNode, passengers_count, crew_count, XML_TR_version.get());
    text = GetXMLDocText(doc.docPtr());
    type = APIS_TR;
  }
  else if ( format.rule(r_file_LT) && passengers_count )
  {
    type = APIS_LT;
    text = FPM.toEdiString();
  }
  // положим апис в очередь на отправку
#if !APIS_TEST
  if ( !text.empty() )
  {
    TFileQueue::add_sets_params(route.depInfo.airp, route.depInfo.airline, IntToString(route.depInfo.flt_no),
        OWN_POINT_ADDR(), type, 1, file_params);
    if(not file_params.empty())
    {
      file_params[ NS_PARAM_EVENT_ID1 ] = IntToString( route.depInfo.point_id );
      file_params[ NS_PARAM_EVENT_TYPE ] = EncodeEventType( ASTRA::evtFlt );
      TFileQueue::putFile(OWN_POINT_ADDR(), OWN_POINT_ADDR(),
          type, file_params, ConvertCodepage( text, "CP866", "UTF-8"));
      LEvntPrms params;
      params << PrmSmpl<string>("fmt", format.settings.format())
             << PrmElem<string>("country_dep", etCountry, route.country_dep())
             << PrmElem<string>("airp_dep", etAirp, route.depInfo.airp)
             << PrmElem<string>("country_arv", etCountry, route.country_arv())
             << PrmElem<string>("airp_arv", etAirp, route.arvInfo().airp);
      TReqInfo::Instance()->LocaleToLog("EVT.APIS_CREATED", params, evtFlt, route.depInfo.point_id);

      if (XML_TR_version)
        set_trip_apis_param(route.depInfo.point_id, "XML_TR", "version", XML_TR_version.get());

      result = true;
    }
  }
#endif
  return result;
}

//---------------------------------------------------------------------------------------

void CreateTxt( const TApisRouteData& route,
                const TAPISFormat& format,
                TTxtDataFormatted& tdf)
{
  int count = 0;
  for ( TApisPaxDataList::const_iterator iPax = route.lstPaxData.begin();
        iPax != route.lstPaxData.end();
        ++iPax)
  {
    if (iPax->status==psCrew && !format.rule(r_notOmitCrew))
      continue;
    if (iPax->status!=psCrew && format.rule(r_omitPassengers))
      continue;
    if (iPax->status!=psCrew && !iPax->pr_brd && route.final_apis())
      continue;
    if (omit_incomplete_apis(route.depInfo.point_id, *iPax, format))
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

    if (format.rule(r_convertPaxNames))
      format.convert_pax_names(pdf.doc_first_name, pdf.doc_second_name);

    pdf.gender=format.Gender(iPax->gender);

    pdf.doc_type = iPax->doc_type_lat(); // throws
    pdf.doc_no = iPax->doc.no;

    if (!APIS::isValidDocType(format.settings.format(), iPax->status, pdf.doc_type))
      pdf.doc_type.clear();

    if (!pdf.doc_type.empty() && format.rule(r_processDocType))
      pdf.doc_type = format.process_doc_type(pdf.doc_type);

    if (format.rule(r_processDocNumber))
      pdf.doc_no = format.process_doc_no(pdf.doc_no);

    pdf.nationality = format.ConvertCountry(iPax->doc.nationality);
    pdf.issue_country = format.ConvertCountry(iPax->doc.issue_country);

    if (iPax->doc.birth_date!=NoExists && format.rule(r_birth_date))
      pdf.birth_date = DateTimeToStr(iPax->doc.birth_date, format.DateTimeFormat(), true);

    if (iPax->doc.expiry_date!=NoExists && format.rule(r_expiry_date))
      pdf.expiry_date = DateTimeToStr(iPax->doc.expiry_date, format.DateTimeFormat(), true);

    if (iPax->docaB && format.rule(r_birth_country))
      pdf.birth_country = format.ConvertCountry(iPax->docaB.get().country);

    pdf.trip_type = "N";
    if (format.rule(r_trip_type))
        pdf.trip_type = getTripType(iPax->status, iPax->grp_id,
            format.direction(route.country_dep()), format.apis_country());

    pdf.airp_arv_final_code_lat = iPax->airp_arv_final_code_lat();

    if (format.rule(r_airp_arv_code_lat))
      pdf.airp_arv_code_lat = iPax->airp_arv_code_lat(); // throws

    if (format.rule(r_airp_dep_code_lat))
      pdf.airp_dep_code_lat = route.airp_dep_code_lat(); // throws

    // doco
    if (iPax->doco && format.rule(r_doco))
    {
      pdf.doco_exists = iPax->doco != boost::none;
      pdf.doco_type = iPax->doco_type_lat(); // throws
      pdf.doco_no = iPax->doco.get().no;
      if (format.rule(r_issueCountryInsteadApplicCountry))
      {
        TElemFmt fmt;
        pdf.doco_country = format.ConvertCountry(issuePlaceToPaxDocCountryId(iPax->doco.get().issue_place, fmt));
      }
      else
        pdf.doco_country = format.ConvertCountry(iPax->doco.get().applic_country);
    }

    tdf.lstPaxData.push_back(pdf);
    ++count;
  } // for iPax
  tdf.count_overall = count;
}

//---------------------------------------------------------------------------------------

Paxlst::PaxlstInfo& getPaxlstInfo(const TApisRouteData& route,
                                  const TAPISFormat& format,
                                  const TApisPaxData& pax,
                                  Paxlst::PaxlstInfo& FPM,
                                  Paxlst::PaxlstInfo& FCM)
{
  return (pax.status != psCrew?FPM:FCM);
}

bool CreateApisFiles(const TApisDataset& dataset, TApisTestMap* test_map = nullptr)
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
      for (APIS::SettingsList::const_iterator iApis = iRoute->lstSetsData.begin(); iApis != iRoute->lstSetsData.end(); ++iApis)
      {
#if APIS_TEST
        test_map->try_key.set(iRoute->arvInfo().point_id, iApis->second.format());
#endif
        boost::scoped_ptr<const TAPISFormat> pFormat(SpawnAPISFormat(iApis->second));
        // https://stackoverflow.com/questions/6718538/does-boostscoped-ptr-violate-the-guideline-of-logical-constness

        if ((iRoute->task_name==ON_CLOSE_CHECKIN && !pFormat->rule(r_create_ON_CLOSE_CHECKIN)) ||
            (iRoute->task_name==ON_CLOSE_BOARDING && !pFormat->rule(r_create_ON_CLOSE_BOARDING)) ||
            (iRoute->task_name==ON_FLIGHT_CANCEL && !pFormat->rule(r_create_ON_FLIGHT_CANCEL)) ||
            (iRoute->task_name==ON_TAKEOFF && pFormat->rule(r_skip_ON_TAKEOFF)))
          continue;

        Paxlst::PaxlstInfo FPM(pFormat->firstPaxlstType(iRoute->task_name),
                               pFormat->firstPaxlstTypeExtra(iRoute->task_name, iRoute->final_apis()));

        Paxlst::PaxlstInfo FCM(pFormat->secondPaxlstType(iRoute->task_name),
                               pFormat->secondPaxlstTypeExtra(iRoute->task_name, iRoute->final_apis()));

        vector< pair<string, string> > files;
        string text, type;
        map<string, string> file_params;

        TTxtDataFormatted tdf;
        ostringstream body, paxs_body, crew_body;
        ostringstream header, pax_header, crew_header;
        string file_name;

        switch (pFormat->format_type)
        {
          case t_format_edi:
          case t_format_iapi:
            CreateEdi(*iRoute, *pFormat, FPM, FCM, getPaxlstInfo);
            switch (pFormat->file_rule)
            {
              case r_file_rule_1:
                CreateEdiFile1(*iRoute, *pFormat, FPM, FCM, files);
                break;
              case r_file_rule_2:
                if (CreateEdiFile2(*iRoute, *pFormat, FPM, FCM, text, type, file_params)) result=true;
                break;
              default:
                break;
            }
            break;
          case t_format_txt:
            CreateTxt(*iRoute, *pFormat, tdf);
            pFormat->CreateTxtBodies(tdf, body, paxs_body, crew_body);
            if (iRoute->task_name.empty() ||
                !iRoute->use_us_customs_tasks ||
                iRoute->task_name==BEFORE_TAKEOFF_30)
            {
              file_name = pFormat->CreateFilename(*iRoute);
              pFormat->CreateHeaders(*iRoute, header, pax_header, crew_header, crew_body, tdf.count_overall);
              switch (pFormat->file_rule)
              {
                case r_file_rule_txt_AE_TH:
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
                case r_file_rule_txt_common:
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
        apis_test_key key(iRoute->arvInfo().point_id, iApis->second.format());
        apis_test_value value(files, text, type, file_params);
        test_map->insert(make_pair(key, value));
#else
        if (!files.empty())
        {
          bool apis_created = false;
          if (pFormat->settings.transportType() == TRANSPORT_TYPE_FILE)
          {
            LogTrace(TRACE5) << __func__ << " TRANSPORT_TYPE_FILE, files.size=" << files.size();
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
            apis_created = true;
          }
          else if (pFormat->settings.transportType() == TRANSPORT_TYPE_RABBIT_MQ)
          {
            LogTrace(TRACE5) << __func__ << " TRANSPORT_TYPE_RABBIT_MQ, files.size=" << files.size();
            MQRABBIT_TRANSPORT::MQRabbitParams mq(pFormat->settings.transportParams());
            jms::connection cl( mq.addr );
            jms::text_queue queue = cl.create_text_queue( mq.queue );
            for (auto iFile : files)
            {
              jms::text_message in1;
              in1.text = iFile.second;
              queue.enqueue(in1);
            }
            cl.commit();
            apis_created = true;
          }

          if (apis_created)
          {
            LEvntPrms params;
            params << PrmSmpl<string>("fmt", pFormat->settings.format())
                   << PrmElem<string>("country_dep", etCountry, iRoute->country_dep())
                   << PrmElem<string>("airp_dep", etAirp, iRoute->depInfo.airp)
                   << PrmElem<string>("country_arv", etCountry, iRoute->country_arv())
                   << PrmElem<string>("airp_arv", etAirp, iRoute->arvInfo().airp);
            TReqInfo::Instance()->LocaleToLog("EVT.APIS_CREATED", params, evtFlt, iRoute->depInfo.point_id);
            result = true;
          }
        } // !files.empty
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

//---------------------------------------------------------------------------------------

bool create_apis_file(int point_id, const string& task_name)
{
  TApisDataset dataset;
  if (!dataset.FromDB(point_id, task_name))
    return false;
  return CreateApisFiles(dataset);
}

//---------------------------------------------------------------------------------------

bool TAPPSVersion26::CheckDocoIssueCountry(const string& issue_place)
{
  if (issue_place.empty()) return true;
  TElemFmt elem_fmt;
  issuePlaceToPaxDocCountryId(issue_place, elem_fmt);
  return elem_fmt != efmtUnknown;
}

//---------------------------------------------------------------------------------------

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
  catch (const Exception& e)
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

//---------------------------------------------------------------------------------------

//сравнивать командой diff -q apis_test_old/ apis_test/
int apis_test_single(int argc, char **argv)
{
  TDateTime firstDate, lastDate;

  if (!getDateRangeFromArgs(argc, argv, firstDate, lastDate)) return 1;

  if(edifact::init_edifact() < 0)
    throw EXCEPTIONS::Exception("'init_edifact' error!");

  list<int> pointIds;
  TQuery PointIdQry(&OraSession);
  PointIdQry.SQLText=
    "SELECT point_id "
    "FROM points "
    "WHERE airline IS NOT NULL AND pr_del=0 AND "
    "      scd_out>=:first_date AND scd_out<:last_date";
  PointIdQry.CreateVariable("first_date", otDate, firstDate);
  PointIdQry.CreateVariable("last_date", otDate, lastDate);
  PointIdQry.Execute();
  for(; !PointIdQry.Eof; PointIdQry.Next())
    pointIds.push_back(PointIdQry.FieldAsInteger("point_id"));

  const std::initializer_list<string> taskNames={BEFORE_TAKEOFF_30, BEFORE_TAKEOFF_60, ON_TAKEOFF, ON_CLOSE_CHECKIN, ON_CLOSE_BOARDING};

  printf("%zu flights found\n", pointIds.size());
  printf("%zu iterations expected\n", pointIds.size()*taskNames.size());

  int iteration = 0;
  for (int point_id : pointIds)
  {
    for (const string& task_name : taskNames)
    {
      nosir_wait(iteration++, false, 5, 0);
      LogTrace(TRACE5) << "TESTING APIS: point = " << point_id << " task = \"" << task_name << "\"";
#if APIS_TEST
      // заполним map
      TApisTestMap map_test;
      TApisDataset dataset;
      try
      {
        dataset.FromDB(point_id, task_name, &map_test);
        CreateApisFiles(dataset, &map_test);
      }
      catch (std::exception& E)
      {
        map_test.exception = true;
        map_test.str_exception = E.what();
      }
#if APIS_TEST_IGNORE_EMPTY
      if (map_test.isEmpty()) continue;
#endif
      // запишем файл
      ofstream file_test;
      try
      {
        ostringstream filename;
        filename << "./apis_test/" << "point_" << point_id << "_task_" << task_name << ".txt";
        file_test.open(filename.str().c_str(), ios_base::trunc|ios_base::out);
        if (!file_test.is_open())
          throw Exception("Can't open file '%s'",filename.str().c_str());
        file_test << map_test.ToString() << endl;
        file_test.close();
      }
      catch (const Exception& e)
      {
        LogTrace(TRACE5) << "ERROR write file: " << e.what();
        try { file_test.close(); } catch (...) { }
      }
#else
      LogTrace(TRACE5) << "APIS_TEST must be true";
#endif
    }
  }
  return 1;
}

#if 0
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
  list<string> lst_task_name = {BEFORE_TAKEOFF_30, BEFORE_TAKEOFF_60, ON_TAKEOFF, ON_CLOSE_CHECKIN, ON_CLOSE_BOARDING};

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
#endif
    } // for (string task_name : lst_task_name)
  } // for (int point_id : lst_point_id)
  return 1;
}
#endif

const std::set<std::string>& getIAPIFormats()
{
  static boost::optional<set<string>> formats;

  if (!formats)
  {
    formats=boost::in_place();
    for(const auto& f : APIS::allFormats())
    {
      std::unique_ptr<const TAPISFormat> pFormat(SpawnAPISFormat(f, false));
      if (dynamic_cast<const TIAPIFormat*>(pFormat.get()))
        formats.get().insert(f);
    }
  }

  return formats.get();
}

const std::set<std::string>& getCrewFormats()
{
  static boost::optional<set<string>> formats;

  if (!formats)
  {
    formats=boost::in_place();
    for(const auto& f : APIS::allFormats())
    {
      std::unique_ptr<const TAPISFormat> pFormat(SpawnAPISFormat(f, false));
      if (pFormat!=nullptr &&
          pFormat->rule(r_notOmitCrew))
        formats.get().insert(f);
    }
  }

  return formats.get();
}

void create_apis_nosir_help(const char *name)
{
  printf("  %-15.15s ", name);
  puts("<points.point_id>  ");
}

int create_apis_nosir(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  int point_id=ASTRA::NoExists;
  try
  {
    //проверяем параметры
    if (argc<2) throw EConvertError("wrong parameters");
    point_id = ToInt(argv[1]);
    Qry.Clear();
    Qry.SQLText="SELECT point_id FROM points WHERE point_id=:point_id";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if (Qry.Eof) throw EConvertError("point_id not found");
  }
  catch(EConvertError &E)
  {
    printf("Error: %s\n", E.what());
    if (argc>0)
    {
      puts("Usage:");
      create_apis_nosir_help(argv[0]);
      puts("Example:");
      printf("  %s 1234567\n",argv[0]);
    };
    return 1;
  };

  init_locale();
  create_apis_file(point_id, (argc>=3)?argv[2]:"");

  puts("create_apis successfully completed");

  send_apis_tr();
  return 0;
}

void create_apis_task(const TTripTaskKey &task)
{
  LogTrace(TRACE5) << __FUNCTION__ << ": " << task;
  create_apis_file(task.point_id, task.params);
}

