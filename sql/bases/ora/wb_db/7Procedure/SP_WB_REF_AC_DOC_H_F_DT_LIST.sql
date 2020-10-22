create or replace PROCEDURE SP_WB_REF_AC_DOC_H_F_DT_LIST(cXML_in IN clob,
                                                             cXML_out OUT CLOB)
AS
P_AIRCO_ID number:=-1;
P_LANG varchar2(100):='';
cXML_data clob:='';
r_count number:=0;
  BEGIN
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]')) into P_AIRCO_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;

    select count(id) into r_count
    from WB_REF_AIRCO_DOCS_HEAD_FOOT
    where ID_AC=P_AIRCO_ID;

    cXML_out:='<?xml version="1.0" ?><root>';

    if (r_count)>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("list", xmlattributes(e.ID "id",
                                                         e.date_from_ "date_from",
                                                           e.doc_name "doc_NAME"))).getClobVal() into cXML_data
        from (select q.id,
                     to_char(q.DATE_from, 'dd.mm.yyyy') date_from_,
                     q.doc_name
              from (select d.id,
                           d.DATE_FROM,
                           case when P_LANG='RUS'
                                then dl.NAME_RUS
                                else dl.NAME_ENG
                           end doc_name
                    from WB_REF_AIRCO_DOCS_HEAD_FOOT d join WB_REF_DOC_TYPE_LIST dl
                    on d.id_ac=P_AIRCO_ID and
                       d.doc_id=dl.id) q
              order by q.doc_name,
                       q.DATE_FROM) e;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;


    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;
  END SP_WB_REF_AC_DOC_H_F_DT_LIST;
/
