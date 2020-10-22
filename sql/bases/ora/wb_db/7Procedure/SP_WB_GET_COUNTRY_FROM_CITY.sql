create or replace PROCEDURE SP_WB_GET_country_from_CITY(cXML_in IN clob,
                                                          cXML_out OUT CLOB)
AS
lang varchar2(50):='';
id_rec number:=-1;
cXML_data clob:='';
rec_count number:=0;
  BEGIN
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/lang[1]') into lang from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/id_rec[1]')) into id_rec from dual;

    select count(cc.id) into rec_count
    from WB_REF_CITIES c join WB_REF_COUNTRY Cc
    on cc.id=c.id_country and
       c.id=id_rec;

    cXML_out:='<?xml version="1.0" ?><root>';

    if rec_count>0 then
      begin
        select '<list COUNTRY_NAME="'||case when lang='RUS'
                                            then WB_CLEAR_XML(nvl(cc.NAME_RUS_FULL, 'NULL'))
                                            else WB_CLEAR_XML(nvl(cc.NAME_ENG_FULL, 'NULL'))
                                       end||'"/>' into cXML_data
        from WB_REF_CITIES c join WB_REF_COUNTRY Cc
        on cc.id=c.id_country and
           c.id=id_rec;
      end;
    end if;

    cXML_out:=cXML_out||cXML_data||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

  END SP_WB_GET_country_from_CITY;
/
