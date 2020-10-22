create or replace PROCEDURE SP_WB_REF_AC_DOC_GET_DATE_INFO(cXML_in IN clob,
                                                             cXML_out OUT CLOB)
AS
P_ADV_ID number:=-1;
P_REMARK clob:='';
P_DATE_FROM varchar2(100);
cXML_data clob:='';
r_count number:=0;
  BEGIN
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ADV_ID[1]')) into P_ADV_ID from dual;

    select count(id) into r_count
    from WB_REF_AIRCO_AUTO_DOCS_DET
    where ADV_ID=P_ADV_ID;

    cXML_out:='<?xml version="1.0" ?><root>';

    if (r_count)>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("list", xmlattributes(e.DOC_ID "doc_id",
                                                         e.copy_count "copy_count",
                                                           e.u_name "U_NAME",
                                                             e.u_ip "U_IP",
                                                               e.u_host_name "U_HOST_NAME",
                                                                 e.date_write "DATE_WRITE"))).getClobVal() into cXML_data
        from (select distinct to_char(doc_id) doc_id,
                              to_char(copy_count) copy_count,
                              U_NAME,
                              U_IP,
                              U_HOST_NAME,
                              to_char(DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') DATE_WRITE
              from WB_REF_AIRCO_AUTO_DOCS_DET
              where ADV_ID=P_ADV_ID) e;

        if cXML_data is not null then
          begin
           cXML_out:=cXML_out||cXML_data||'<remark rem="';

           select nvl(WB_CLEAR_XML(remark), 'NULL') into P_REMARK
           from WB_REF_AIRCO_AUTO_DOCS_ADV
           where id=P_ADV_ID;

           cXML_out:=cXML_out||P_REMARK||'"></remark><date_from dt_from="';

           select to_char(DATE_FROM, 'dd.mm.yyyy') into P_DATE_FROM
           from WB_REF_AIRCO_AUTO_DOCS_ADV
           where id=P_ADV_ID;

           cXML_out:=cXML_out||P_DATE_FROM||'"></date_from>';
          end;
        end if;
      end;
    end if;


    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;
  END SP_WB_REF_AC_DOC_GET_DATE_INFO;
/
