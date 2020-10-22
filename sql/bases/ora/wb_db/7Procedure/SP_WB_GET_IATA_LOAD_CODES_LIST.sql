create or replace PROCEDURE SP_WB_GET_IATA_LOAD_CODES_LIST(cXML_in IN clob,
                                                             cXML_out OUT CLOB)
AS
P_LANG varchar2(50):='';
R_COUNT number;
cXML_data clob:='';
  BEGIN

    P_LANG:=cXML_in;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    ---------------------------DENSITY------------------------------------------
    select count(id) into R_COUNT
    from WB_REF_IATA_LOAD_CODES;

    if R_COUNT>0 then
      begin
        cXML_data:='';

        select XMLAGG(XMLELEMENT("list", xmlattributes(to_char(qq.id) "id",
                                                            qq.CODE_NAME "CODE_NAME"))).getClobVal() into cXML_data
        from (select q.id,
                       q.CODE_NAME
              from (select id,
                             CODE_NAME
                    from WB_REF_IATA_LOAD_CODES) q
              order by q.CODE_NAME) qq;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;
    ---------------------------DENSITY------------------------------------------
    ----------------------------------------------------------------------------

    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;

  END SP_WB_GET_IATA_LOAD_CODES_LIST;
/
