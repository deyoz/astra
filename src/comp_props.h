#ifndef COMP_PROPS_H
#define COMP_PROPS_H

#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_locale.h"

namespace SALONS2
{
  static const std::string TAIL = "tail";
  static const std::string NOSE = "nose";
  static const std::string SECTION = "section";
  static const std::string WING = "wing";
  static const std::string EVT_DELETE_TAIL_SECTIONS = "EVT.DELETE_TAIL_SECTIONS";
  static const std::string EVT_ASSIGNED_TAIL_SECTIONS = "EVT.ASSIGNED_TAIL_SECTIONS";
  static const std::string EVT_DELETE_NOSE_SECTIONS = "EVT.DELETE_NOSE_SECTIONS";
  static const std::string EVT_ASSIGNED_NOSE_SECTIONS = "EVT.ASSIGNED_NOSE_SECTIONS";
  static const std::string EVT_DELETE_LUGGAGE_SECTIONS = "EVT.DELETE_LUGGAGE_SECTIONS";
  static const std::string EVT_ASSIGNED_LUGGAGE_SECTIONS = "EVT.ASSIGNE_LUGGAGE_SECTIONS";
  static const std::string EVT_DELETE_WING_SECTIONS = "EVT.DELETE_WING_SECTIONS";
  static const std::string EVT_ASSIGNED_WING_SECTIONS = "EVT.ASSIGNED_WING_SECTIONS";
  static const std::string WRAP_IS_SECT_DEL = "is_sect_del";
  static const std::string MSG_INVALID_TAIL_SECTION_NAME = "MSG.INVALID_TAILSECTIONS_NAME";
  static const std::string MSG_INVALID_NOSE_SECTION_NAME = "MSG.INVALID_NOSESECTIONS_NAME";
  static const std::string MSG_INVALID_WING_SECTION_NAME = "MSG.INVALID_WINGSECTIONS_NAME";
  static const std::string MSG_INVALID_LUGGAGE_SECTION_NAME = "MSG.INVALID_COMPSECTIONS_NAME";

class SimpleProp {
  private:
    std::string name;
    int first_row;
    int last_row;
  public:
    SimpleProp( const std::string &name, int first_row, int last_row ) {
      this->name = name;
      this->first_row = first_row;
      this->last_row = last_row;
    }
    SimpleProp( ) {
      clear();
    }
    void operator = (const SimpleProp &simpleProp) {
      this->name = simpleProp.getName();
      this->first_row = simpleProp.getFirstRow();
      this->last_row = simpleProp.getLastRow();
    }
    bool operator == ( const SimpleProp &value ) const {
      return ( name == value.name &&
               first_row == value.first_row &&
               last_row == value.last_row );
    }

    const std::string str() const {
      return name + "=(" + IntToString(first_row) + "," + IntToString(last_row) + ")";
    }
    std::string getName() const {
      return name;
    }
    int getFirstRow() const {
        return first_row;
    }
    int getLastRow() const {
        return last_row;
    }
    void clear() {
      first_row = ASTRA::NoExists;
      last_row = ASTRA::NoExists;
      name.clear();
    }
};

struct LexemaType {
  std::string lDelete;
  std::string lAssigned;
  std::string lException;
  LexemaType( const std::string &vDelete,
              const std::string &vAssigned,
              const std::string &vException ):
              lDelete(vDelete),
              lAssigned(vAssigned),
              lException(vException) {}
};

struct CodeNames {
  std::string name;
  std::string name_lat;
  std::string color;
  CodeNames( const std::string &name, const std::string &name_lat, const std::string &color ) {
    this->name = name;
    this->name_lat = name_lat;
    this->color = color;
  }
};

class adjustmentIndexRow:public std::vector<int>{};

class componPropCodes {
  private:
    std::map<std::string,CodeNames> codes;
    std::map<std::string,LexemaType> lexemas;
    void validateCode( const std::string &code, const auto &container  ) {
      if ( container.find( code ) == container.end() ) {
        throw EXCEPTIONS::Exception( "Invalid compon property type %s", code.c_str() );
      }
    }
  public:
    componPropCodes() {
      codes = { make_pair(TAIL,CodeNames("•Ά®αβ","Tail","$00400080")),
                make_pair(NOSE,CodeNames("®α","Nose","$00FF8000")),
                make_pair(SECTION,CodeNames("‘¥ªζ¨ο","Section","clGray")),
                make_pair(WING,CodeNames("ΰλ«®","Wing","clOlive")) };
      lexemas = {
          make_pair(TAIL,LexemaType(EVT_DELETE_TAIL_SECTIONS,EVT_ASSIGNED_TAIL_SECTIONS,MSG_INVALID_TAIL_SECTION_NAME)),
          make_pair(NOSE,LexemaType(EVT_DELETE_NOSE_SECTIONS,EVT_ASSIGNED_NOSE_SECTIONS,MSG_INVALID_NOSE_SECTION_NAME)),
          make_pair(SECTION,LexemaType(EVT_DELETE_LUGGAGE_SECTIONS,EVT_ASSIGNED_LUGGAGE_SECTIONS,MSG_INVALID_LUGGAGE_SECTION_NAME)),
          make_pair(WING,LexemaType(EVT_DELETE_WING_SECTIONS,EVT_ASSIGNED_WING_SECTIONS,MSG_INVALID_WING_SECTION_NAME))
        };
    }
    static componPropCodes *Instance() {
      static componPropCodes *instance = 0;
      if ( !instance ) {
        instance = new componPropCodes();
      }
      return instance;
    }
    void validateCode( const std::string &code ) {
      validateCode( code, codes );
    }
    const std::string &getWrapIsSectDelLexema( const std::string &code ) {
      return WRAP_IS_SECT_DEL;
    }
    const std::string &getDeleteLexema( const std::string &code ) {
      validateCode( code, codes );
      validateCode( code, lexemas );
      return lexemas.find( code )->second.lDelete;
    }
    const std::string &getAssignedLexema( const std::string &code ) {
      validateCode( code, codes );
      validateCode( code, lexemas );
      return lexemas.find( code )->second.lAssigned;
    }
    const std::string &getInvalidSectionNameLexema( const std::string &code ) {
      validateCode( code, codes );
      validateCode( code, lexemas );
      return lexemas.find( code )->second.lException;
    }
    std::vector<std::string> getCodes() {
      std::vector<std::string> res;
      for ( std::map<std::string,CodeNames>::const_iterator i=codes.begin(); i!=codes.end(); i++ ) {
        res.push_back( i->first );
      }
      return res;
    }
    std::string getName( const std::string &lang, const std::string code ) {
       validateCode( code );
       for ( std::map<std::string,CodeNames>::const_iterator i=codes.begin(); i!=codes.end(); i++ ) {
         if ( code == i->first ) {
           return (lang == AstraLocale::LANG_RU)?i->second.name:(i->second.name_lat.empty())?i->second.name:i->second.name_lat;
         }
       }
       return "";
    }
    std::string getColor( const std::string code ) {
       validateCode( code );
       for ( std::map<std::string,CodeNames>::const_iterator i=codes.begin(); i!=codes.end(); i++ ) {
         if ( code == i->first ) {
           return i->second.color;
         }
       }
       return "";
    }
    void buildSections( int comp_id, const std::string &lang, xmlNodePtr dataNode, const adjustmentIndexRow &rows, bool buildEmptySection = true );
};

/*template <typename T> class CompProps {
  private:
    std::string code;
    T props;
  public:
    CompProps( const std::string &code, const T &props ) {
      componPropCodes::validateCode( code );
      this->code = code;
      this->props = props;
    }
    void read( int comp_id ) {
      props.read( comp_id );
    }
    void write( int comp_id ) {
      props.write( comp_id );
    }
    void build( xmlNodePtr sectionNode ) {
      props.build( sectionNode );
    }
    void parse( xmlNodePtr sectionNode ) {
      props.parse( sectionNode );
    }
};*/

struct less_than_SimpleProp
{
    inline bool operator() (const SimpleProp& prop1, const SimpleProp& prop2)
    {
        return (prop1.getFirstRow() < prop2.getFirstRow());
    }
};

class simpleProps:public std::vector<SimpleProp> {
  private:
    std::string code;
    simpleProps& adjustmentIndexRowFunc( const adjustmentIndexRow &rows );
  public:
    simpleProps( const std::string &code ) {
      this->code = code;
    }
    simpleProps& build( const adjustmentIndexRow &rows, xmlNodePtr sectionNode );
    simpleProps& read( int comp_id );
    simpleProps& write( int comp_id );
    simpleProps& parse( xmlNodePtr sectionNode );
    simpleProps& parse_check_write( xmlNodePtr sectionNode, int comp_id );
    simpleProps& parse_write( xmlNodePtr sectionNode, int comp_id ) {
      if ( sectionNode ) {
        parse( sectionNode ).write( comp_id );
      }
      return *this;
    }
};

void checkBuildSections( int point_id, int comp_id, xmlNodePtr dataNode, const adjustmentIndexRow &rows, bool buildEmptySection = true );

} //end namespace

#endif // COMP_PROPS_H
