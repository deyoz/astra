#include <stdlib.h>
#include "images.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "oralib.h"
#include "astra_elems.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "base_tables.h"
#include "stl_utils.h"
#include "salons.h"
#include "term_version.h"
#include "db_tquery.h"
#include "PgOraConfig.h"
#include "pg_session.h"
#include "serverlib/str_utils.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#define NICKNAME "DJEK"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

namespace BASIC_SALONS {

const std::string default_elem_type = "К";  //!!!здесь определяем

template<class IsPlacesT, class MaxTimeCreateT, class DefaultElemCodeT>
static void UpdatePg(
    const string& query,
    IsPlacesT& is_places,
    MaxTimeCreateT& max_time_create,
    DefaultElemCodeT& default_elem_code)
{
    string code;
    string filename;
    string name;
    string name_lat;
    int pr_seat = 0;
    int pr_del = 0;
    boost::posix_time::ptime time_create_;
    string image;
    PgCpp::BinaryDefHelper<string> imageDef{image};

    PgCpp::CursCtl curs = make_pg_curs(PgCpp::getPgReadOnly(), query);
    curs.def(code)
        .def(filename)
        .def(name)
        .def(name_lat)
        .def(pr_seat)
        .defNull(pr_del, 0)
        .def(time_create_)
        .def(imageDef)
        .exec();

    LogTrace(TRACE3) << __func__ << ":";

    while (not curs.fen()) {
        if (pr_del != 0) {
            continue;
        }

        const bool is_default_element = code == default_elem_type;
        if (is_default_element) {
            default_elem_code = default_elem_type;
        }

        TDateTime time_create = BoostToDateTime(time_create_);
        max_time_create = std::max(max_time_create, time_create);

        string image_;
        StringToHex(image, image_);

        LogTrace(TRACE3) << __func__ << ": image.substr(10) = " << image.substr(10);

        is_places.emplace(code, TCompElemType{
            code,
            name,
            name_lat,
            pr_seat,
            is_default_element,
            time_create,
            image_,
            filename
        });
    }
}

template<class IsPlacesT, class MaxTimeCreateT, class DefaultElemCodeT>
static void UpdateOra(
    const string& query,
    IsPlacesT& is_places,
    MaxTimeCreateT& max_time_create,
    DefaultElemCodeT& default_elem_code)
{
    TQuery Qry( &OraSession );
    Qry.SQLText = query;
    Qry.Execute();
    int len = 0;
    void *data = NULL;
    try {
        for ( ;!Qry.Eof; Qry.Next() ) {
            if ( Qry.FieldIsNULL( "pr_del" ) || Qry.FieldAsInteger( "pr_del" ) == 0 ) {
                string code = Qry.FieldAsString( "code" );
                string filename = Qry.FieldAsString( "filename" );
                string name = Qry.FieldAsString( "name" );
                string name_lat = Qry.FieldAsString( "name_lat" );
                TDateTime time_create = Qry.FieldAsDateTime( "time_create" );
                string image;
                if ( name_lat.empty() )
                    name_lat = name;
                if ( len != Qry.GetSizeLongField( "image" ) ) {
                    len = Qry.GetSizeLongField( "image" );
                    if ( data == NULL )
                        data = malloc( len );
                    else
                        data = realloc( data, len );
                }
                if ( data == NULL )
                    throw Exception( "Ошибка программы" );
                Qry.FieldAsLong( "image", data );
                StringToHex( string((char*)data, len), image );
                bool is_default_element = code == default_elem_type;
                if ( is_default_element )
                    default_elem_code = default_elem_type;
                TCompElemType comp_elem( code,
                                         name,
                                         name_lat,
                                         Qry.FieldAsInteger( "pr_seat" ),
                                         is_default_element,
                                         time_create,
                                         image,
                                         filename );
                if ( max_time_create < time_create )
                    max_time_create = time_create;
                is_places.insert( make_pair( code, comp_elem ) );
            }
        }
        if ( data != NULL )
            free( data );
    }
    catch(...) {
        if ( data != NULL )
            free( data );
        throw;
    }
}

void TCompElemTypes::Update()
{
    static const string QUERY =
        "SELECT code, filename, name, name_lat, pr_seat, pr_del, time_create, image "
        "  FROM comp_elem_types";

    is_places.clear();
    max_time_create = -1;
    default_elem_code.clear();

    if (PgOra::supportsPg("COMP_ELEM_TYPES")) {
        UpdatePg(QUERY, is_places, max_time_create, default_elem_code);
    }
    UpdateOra(QUERY, is_places, max_time_create, default_elem_code);
}

TCompElemTypes::TCompElemTypes()
{
  Update();
}

void TCompElemTypes::toFile(const string &file_name) const
{
    ofstream out(file_name.c_str());
    if (!out.good()) throw Exception("Cannot open for write file %s", file_name.c_str());
    for(const auto &i: is_places) {
        string StrDec;
        HexToString( i.second.getImage(), StrDec );
        out
            << i.first << "|"
            << i.second.getName() << "|"
            << i.second.isSeat() << "|"
            << i.second.getNameLat() << "|"
            << i.second.getFilename() << "|"
            << i.second.getImage() << endl;
    }
}

template<class IsPlacesT>
static void ToDbPg(const IsPlacesT& is_places)
{
    static const string PG_QUERY =
        "INSERT INTO comp_elem_types(code,name,name_lat,pr_seat,time_create,image,pr_del,filename)"
            "  VALUES(:code,:name,:name_lat,:pr_seat,:time_create,:image,0,:filename)"
            "    ON CONFLICT(code) DO"
            "      UPDATE SET name=:name,name_lat=:name_lat,pr_seat=:pr_seat,time_create=:time_create,image=:image, pr_del=0, filename = :filename;";

    for(const auto &i : is_places) {
        string image;
        HexToString(i.second.getImage(), image);

        auto cur = DbCpp::mainPgManagedSession(STDLOG).createPgCursor(STDLOG, PG_QUERY, true);

        cur.
            bind(":code", i.first).
            bind(":name", i.second.getName()).
            bind(":name_lat", i.second.getNameLat()).
            bind(":pr_seat", static_cast<int>(i.second.isSeat())).
            bind(":time_create", DateTimeToBoost(i.second.getTimeCreate())).
            bind(":image", PgCpp::BinaryBindHelper({image.data(), image.size()})).
            bind(":filename", i.second.getFilename()).
            exec();
    }
}

template<class IsPlacesT>
static void ToDbOra(const IsPlacesT& is_places)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "BEGIN "
        " UPDATE comp_elem_types "
        "   SET name=:name,name_lat=:name_lat,pr_seat=:pr_seat,time_create=:time_create,image=:image, pr_del=0, filename = :filename "
        "  WHERE code=:code; "
        " IF SQL%NOTFOUND THEN "
        "  INSERT INTO comp_elem_types(code,name,name_lat,pr_seat,time_create,image,pr_del,filename) "
        "   VALUES(:code,:name,:name_lat,:pr_seat,:time_create,:image,0,:filename); "
        " END IF; "
        "END;";
    Qry.DeclareVariable("code", otString);
    Qry.DeclareVariable("name", otString);
    Qry.DeclareVariable("name_lat", otString);
    Qry.DeclareVariable("pr_seat", otInteger);
    Qry.DeclareVariable("time_create", otDate);
    Qry.DeclareVariable("image", otLongRaw);
    Qry.DeclareVariable("filename", otString);
    for(const auto &i: is_places) {
        Qry.SetVariable("code", i.first);
        Qry.SetVariable("name", i.second.getName());
        Qry.SetVariable("name_lat", i.second.getNameLat());
        Qry.SetVariable("pr_seat", i.second.isSeat());
        Qry.SetVariable("time_create", i.second.getTimeCreate());
        string StrDec;
        HexToString( i.second.getImage(), StrDec );
        Qry.SetLongVariable( "image", const_cast<void*>(reinterpret_cast<const void*>(StrDec.c_str())), StrDec.length() );
        Qry.SetVariable("filename", i.second.getFilename());
        Qry.Execute();
    }
}

void TCompElemTypes::toDB()
{
    try {
        if (PgOra::supportsPg("COMP_ELEM_TYPES")) {
            ToDbPg(is_places);
        }
        ToDbOra(is_places);
    } catch (const std::exception& e) {
        cout << e.what() << endl;
        throw e;
    }
}

TCompElemTypes::TCompElemTypes(xmlNodePtr &reqNode)
{
    xmlNodePtr node = GetNode("data/images", reqNode);
    if ( node != NULL ) {
        TDateTime d = NodeAsDateTime( "@time_create", node );
        node = node->children;
        while ( node ) {
            string code = NodeAsString( "code", node );
            TCompElemType comp_elem(
                    code,
                    NodeAsString( "name", node ),
                    {},
                    NodeAsInteger( "pr_seat", node ) != 0,
                    code == default_elem_type,
                    d,
                    NodeAsString( "image", node ),
                    NodeAsString( "code", node ));
            is_places.insert( make_pair( code, comp_elem ) );
            node = node->next;
        }
    }
}

TCompElemTypes::TCompElemTypes(const std::string &file_name)
{
    ifstream in(file_name.c_str());
    if (!in.good()) throw Exception("Cannot open for read file %s", file_name.c_str());
    TDateTime d = NowUTC();
    for(string line; getline(in, line); ) {
        vector<string> tokens;
        boost::split(tokens, line, boost::is_any_of("|"));
        if(tokens.size() != 6)
            throw Exception("wrong file format");
        string code = tokens[0];
        string name = tokens[1];
        bool pr_seat = ToInt(tokens[2]) != 0;
        string name_lat = tokens[3];
        string filename = tokens[4];
        string filedata = tokens[5];

        TCompElemType comp_elem(
                code,
                name,
                name_lat,
                pr_seat,
                code == default_elem_type,
                d,
                filedata,
                filename);
        is_places.insert( make_pair( code, comp_elem ) );
    }
}

///////////////////////////////LAYERS///////////////////////////////////////////
TCompLayerTypes::TCompLayerTypes()
{
  Update();
}

template <class LayersT>
static void FillLayers(LayersT& layers)
{
    DB::TQuery Qry(PgOra::getROSession("COMP_LAYER_TYPES"));
    Qry.SQLText =
        "SELECT code, name, name_lat, del_if_comp_chg, color, figure, pr_occupy, priority"
        "  FROM comp_layer_types";
    Qry.Execute();
    for (; !Qry.Eof; Qry.Next()) {
        const string code = Qry.FieldAsString("code");
        const auto layer_type = DecodeCompLayerType(code.c_str());
        const BASIC_SALONS::TCompLayerPriority layer(code,
                                                     layer_type,
                                                     Qry.FieldAsString("name"),
                                                     Qry.FieldAsString("name_lat"),
                                                     Qry.FieldAsString("color"),
                                                     Qry.FieldAsString("figure"),
                                                     Qry.FieldAsInteger("pr_occupy"),
                                                     Qry.FieldAsInteger("priority"));
        layers.emplace(layer_type, layer);
        //ProgTrace( TRACE5, "TCompLayerTypes::Update(): add %s", code.c_str() );
    }
}

template <class AirlinePrioritiesT>
static void FillAirlinePriorities(AirlinePrioritiesT& airlinePriorities)
{
    DB::TQuery Qry(PgOra::getROSession("COMP_LAYER_PRIORITIES"));
    Qry.SQLText =
        "SELECT airline, layer_type, priority"
        "  FROM comp_layer_priorities";
    Qry.Execute();
    for (; !Qry.Eof; Qry.Next()) {
        const auto airline = Qry.FieldAsString("airline");
        const auto layerType = DecodeCompLayerType(Qry.FieldAsString("layer_type").c_str());
        const auto layerKey = TCompLayerTypes::LayerKey(airline, layerType);
        const auto priority = Qry.FieldAsInteger("priority");
        airlinePriorities.emplace(layerKey, priority);
    }
}

template <class PriorityRoutesT>
static void FillLayersPriorityRoutes(PriorityRoutesT& layersPriorityRoutes)
{
    DB::TQuery Qry(PgOra::getROSession("COMPARE_COMP_LAYERS"));
    Qry.SQLText =
        "SELECT prior_layer, next_layer, prior_time_less"
        "  FROM compare_comp_layers";
    Qry.Execute();
    for (; !Qry.Eof; Qry.Next()) {
        const int priorTimeLess = Qry.FieldIsNULL("prior_time_less")
            ? ASTRA::NoExists
            : Qry.FieldAsInteger("prior_time_less");
        const auto priorLayerType = DecodeCompLayerType(Qry.FieldAsString("prior_layer").c_str());
        const auto nextLayerType = DecodeCompLayerType(Qry.FieldAsString("next_layer").c_str());
        layersPriorityRoutes[priorLayerType][nextLayerType] = priorTimeLess;
    }
}

void TCompLayerTypes::Update()
{
    layers.clear();
    layers_priority_routes.clear();

    FillLayers(layers);
    FillAirlinePriorities(airline_priorities);
    FillLayersPriorityRoutes(layers_priority_routes);
}

} //end namespace BASIC_SALONS


using namespace BASIC_SALONS;
void ImagesInterface::GetImages( xmlNodePtr reqNode, xmlNodePtr resNode )
{
  ProgTrace( TRACE5, "ImagesInterface::GetImages" );

  xmlNodePtr dataNode = GetNode( "data", resNode );
  if ( dataNode == NULL )
    dataNode = NewTextChild( resNode, "data" );

  xmlNodePtr imagesNode = GetNode( "images", dataNode );
  if ( imagesNode == NULL )
    imagesNode = NewTextChild( dataNode, "images" );
  SetProp( imagesNode, "default_comp_elem", TCompElemTypes::Instance()->getDefaultElemType() );
  SetProp( imagesNode, "lastUpdDate", DateTimeToStr( TCompElemTypes::Instance()->getLastUpdateDate() ) );

  bool sendImages = ( fabs( TCompElemTypes::Instance()->getLastUpdateDate() - NodeAsDateTime( "@lastUpdDate", reqNode ) ) > 5.0/(24.0*60.0*60.0) );

  std::vector<std::string> server_elem_types;
  std::map<std::string,std::string> server_elem_files;
  TCompElemTypes::Instance()->getElemTypes( server_elem_types );
  for ( std::vector<std::string>::iterator icode=server_elem_types.begin();
        icode!=server_elem_types.end(); icode++ ) {
     TCompElemType comp_elem;
     TCompElemTypes::Instance()->getElem( *icode, comp_elem );
     server_elem_files.insert( make_pair(*icode, comp_elem.getFilename()) );
  }
   xmlNodePtr codeNode = GetNode( "codes", reqNode );
   if ( codeNode != NULL ) {
     if ( codeNode && !sendImages ) {
       codeNode = GetNode( "code", codeNode );
       std::vector<std::string> client_elem_codes;
       while ( codeNode && string((const char*)codeNode->name) == "code" ) {
         client_elem_codes.push_back( NodeAsString( codeNode ) );
         codeNode = codeNode->next;
       }
       // надо убедиться что на клиенте есть все элементы сервера
       for ( std::vector<std::string>::iterator icode=server_elem_types.begin();
             icode!=server_elem_types.end(); icode++ ) {
         if ( SALONS2::isConstructivePlace( *icode ) &&
              !TReqInfo::Instance()->desk.compatible( SALON_SECTION_VERSION ) ) {
           continue;
         }
         if ( find( client_elem_codes.begin(), client_elem_codes.end(), *icode ) == client_elem_codes.end() ) {
           ProgTrace( TRACE5, "code=%s", icode->c_str() );
           sendImages = true;
           break;
         }
       }
     }
   }
   codeNode = GetNode( "files", reqNode );
   if ( codeNode != NULL ) {
     if ( codeNode && !sendImages ) {
       codeNode = GetNode( "filename", codeNode );
       std::vector<std::string> client_elem_files;
       while ( codeNode && string((const char*)codeNode->name) == "filename" ) {
         client_elem_files.push_back( NodeAsString( codeNode ) );
         codeNode = codeNode->next;
       }
       // надо убедиться что на клиенте есть все элементы сервера
       for ( std::map<std::string,std::string>::iterator icode=server_elem_files.begin();
             icode!=server_elem_files.end(); icode++ ) {
         if ( SALONS2::isConstructivePlace( icode->first ) &&
              !TReqInfo::Instance()->desk.compatible( SALON_SECTION_VERSION ) ) {
           continue;
         }
         if ( find( client_elem_files.begin(), client_elem_files.end(), icode->second ) == client_elem_files.end() ) {
           ProgTrace( TRACE5, "code=%s", icode->first.c_str() );
           sendImages = true;
           break;
         }
       }
     }
   }
   if ( sendImages ) {
     SetProp( imagesNode, "sendimages", "true" );
   }
   /* пересылаем все данные */
   TCompElemType elem_type;
   for ( std::vector<std::string>::iterator icode=server_elem_types.begin();
         icode!=server_elem_types.end(); icode++ ) {
     if ( TCompElemTypes::Instance()->getElem( *icode, elem_type ) ) {
       if ( SALONS2::isConstructivePlace( elem_type.getCode() ) &&
            !TReqInfo::Instance()->desk.compatible( SALON_SECTION_VERSION ) ) {
         continue;
       }
       xmlNodePtr imageNode = NewTextChild( imagesNode, "image" );
       NewTextChild( imageNode, "code", elem_type.getCode() );
       NewTextChild( imageNode, "filename", elem_type.getFilename() );
       if ( TReqInfo::Instance()->desk.lang == AstraLocale::LANG_RU )
         NewTextChild( imageNode, "name", elem_type.getName() );
       else
         NewTextChild( imageNode, "name", elem_type.getNameLat() );
       NewTextChild( imageNode, "pr_seat", elem_type.isSeat() );
       if ( sendImages ) {
         NewTextChild( imageNode, "image", elem_type.getImage() );
       }
     }
   }
}

int comp_elem_types_from_db(int argc, char **argv)
{
    try {
        if (argc != 2) throw Exception("Must provide path");
        TCompElemTypes().toFile(argv[1]);
        cout << __func__ << ": done." << endl;
        return 0;
    } catch(Exception &E) {
        cout << __func__ << ": error: " << E.what() << endl;
        return 1;
    } catch(...) {
        cout << __func__ << ": unexpected error" << endl;
        return 1;
    }
}

int comp_elem_types_to_db(int argc, char **argv)
{
    try {
        if (argc != 2) throw Exception("Must provide path");
        TCompElemTypes(argv[1]).toDB();
        cout << __func__ << ": done." << endl;
        return 0;
    } catch(Exception &E) {
        cout << __func__ << ": error: " << E.what() << endl;
        return 1;
    } catch(...) {
        cout << __func__ << ": unexpected error" << endl;
        return 1;
    }
}

void ImagesInterface::SetImages(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    ProgTrace(TRACE2, "ImagesInterface::SetImages" );
    TCompElemTypes(reqNode).toDB();
    TCompElemTypes::Instance()->Update(); //перечитать информацию
    AstraLocale::showMessage( "MSG.DATA_SAVED" );
};

void ImagesInterface::GetImages(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  GetImages( reqNode, resNode );
};


void GetDrawSalonProp( xmlNodePtr reqNode, xmlNodePtr resNode )
{
  ImagesInterface::GetImages( reqNode, resNode );
  DB::TQuery Qry(PgOra::getROSession("COMP_LAYER_TYPES"));
  Qry.SQLText = "SELECT code,color,figure FROM comp_layer_types";
  Qry.Execute();
  xmlNodePtr imagesNode = GetNode( "data/images", resNode );
  xmlNodePtr layersNode = GetNode( "layers_color", imagesNode );
  if ( !layersNode )
    layersNode = NewTextChild( imagesNode, "layers_color" );
  while ( !Qry.Eof ) {
      ASTRA::TCompLayerType l = DecodeCompLayerType( Qry.FieldAsString( "code" ).c_str() );
    if ( !SALONS2::compatibleLayer( l ) ) {
      Qry.Next();
      continue;
    }
    xmlNodePtr n = NewTextChild( layersNode, "layer", Qry.FieldAsString( "code" ) );
    if ( !Qry.FieldIsNULL( "color" ) ) {
      SetProp( n, "color", Qry.FieldAsString( "color" ) );
    }
    if ( !Qry.FieldIsNULL( "figure" ) ) {
      SetProp( n, "figure", Qry.FieldAsString( "figure" ) );
    }
    Qry.Next();
  }
}

void getTariffColors( std::map<std::string,std::string> &colors )
{
  colors.clear();
  DB::TQuery Qry(PgOra::getROSession("COMP_TARIFF_COLORS"));
  Qry.SQLText = "SELECT color,figure FROM comp_tariff_colors";
  Qry.Execute();
  for ( ;!Qry.Eof; Qry.Next() ) {
    colors.insert( make_pair( Qry.FieldAsString( "color" ), Qry.FieldAsString( "figure" ) ) );
  }
}

void GetDrawWebTariff( xmlNodePtr reqNode, xmlNodePtr resNode )
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (!reqInfo->user.access.rights().permitted(431)) return;

    xmlNodePtr tariffsNode = GetNode( "data", resNode );
    if ( !tariffsNode ) {
        tariffsNode = NewTextChild( resNode, "data" );
    }
    tariffsNode = GetNode( "data/images", resNode );
    if ( !tariffsNode ) {
        tariffsNode = NewTextChild( tariffsNode, "images" );
    }
    tariffsNode = NewTextChild( tariffsNode, "web_tariff_property" );
    map<string,string> colors;
    getTariffColors( colors );
    for ( map<string,string>::iterator i=colors.begin(); i!=colors.end(); i++ ) {
      xmlNodePtr n = NewTextChild( tariffsNode, "tarif" );
      SetProp( n, "color", i->first );
      SetProp( n, "figure", i->second );
    }
}

void GetDataForDrawSalon( xmlNodePtr reqNode, xmlNodePtr resNode)
{
    GetDrawSalonProp( reqNode, resNode );
    GetDrawWebTariff( reqNode, resNode );
}

void ImagesInterface::GetDrawSalonData(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    GetDataForDrawSalon( reqNode, resNode );
}

void ImagesInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
};
