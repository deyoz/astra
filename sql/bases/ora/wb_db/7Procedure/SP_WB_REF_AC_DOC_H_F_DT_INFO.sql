create or replace PROCEDURE SP_WB_REF_AC_DOC_H_F_DT_INFO(cXML_in IN clob,
                                                           cXML_out OUT CLOB)
AS
P_ID number:=-1;
P_LANG varchar2(100):='';
cXML_data clob:='';
r_count number:=0;
  BEGIN
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;

    select count(id) into r_count
    from WB_REF_AIRCO_DOCS_HEAD_FOOT
    where ID=P_ID;

    cXML_out:='<?xml version="1.0" ?><root>';

    if (r_count)>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("list", xmlattributes(e.date_from "date_from",
                                                         e.doc_id "doc_id",
                                                           e.doc_name "doc_name",
                                                             e.u_name "U_NAME",
                                                               e.u_ip "U_IP",
                                                                 e.u_host_name "U_HOST_NAME",
                                                                   e.date_write "DATE_WRITE"),
                                                                     xmlelement("header", (select header
                                                                                           from WB_REF_AIRCO_DOCS_HEAD_FOOT
                                                                                           where id=P_ID)),
                                                                       xmlelement("footer", (select footer
                                                                                             from WB_REF_AIRCO_DOCS_HEAD_FOOT
                                                                                             where id=P_ID)))).getClobVal() into cXML_data
        from (select to_char(d.DATE_from, 'dd.mm.yyyy') date_from,
                     d.doc_id,
                     case when P_LANG='RUS'
                          then dl.NAME_RUS
                          else dl.NAME_ENG
                     end doc_name,
                     d.U_NAME,
                     d.U_IP,
                     d.U_HOST_NAME,
                     to_char(d.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') DATE_WRITE
              from WB_REF_AIRCO_DOCS_HEAD_FOOT d join WB_REF_DOC_TYPE_LIST dl
              on d.id=P_ID and
                 d.doc_id=dl.id) e;

        if cXML_data is not null
          then cXML_out:=cXML_out||cXML_data;
        end if;
      end;
    end if;

    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;
  END SP_WB_REF_AC_DOC_H_F_DT_INFO;
/
