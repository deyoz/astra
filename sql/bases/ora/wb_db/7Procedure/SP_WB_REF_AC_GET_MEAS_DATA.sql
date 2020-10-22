create or replace PROCEDURE SP_WB_REF_AC_GET_MEAS_DATA(cXML_in IN clob,
                                                         cXML_out OUT CLOB)
AS
P_AIRCO_ID number:=-1;
P_LANG varchar2(50):='';
cXML_data clob:='';
R_COUNT number:=0;
  BEGIN
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]')) into P_AIRCO_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;

    select count(id) into R_COUNT
    from WB_REF_AIRCO_MEAS
    where ID_AC=P_AIRCO_ID;

    cXML_out:='<?xml version="1.0" ?><root>';

    if (R_COUNT)>0 then
      begin
        SELECT XMLAGG(XMLELEMENT("list", xmlattributes(qq.ID "id",
                                                         qq.date_from "date_from",
                                                           qq.D_MES_NAME "density",
                                                             qq.L_MES_NAME "length",
                                                               qq.V_MES_NAME "volume",
                                                                 qq.W_MES_NAME "weight",
                                                                   qq.u_name "U_NAME",
                                                                     qq.u_ip "U_IP",
                                                                       qq.u_host_name "U_HOST_NAME",
                                                                         qq.date_write "DATE_WRITE"))).getClobVal() into cXML_data

        from (select q.id,
                     q.date_from,
                     q.D_MES_NAME,
                     q.L_MES_NAME,
                     q.V_MES_NAME,
                     q.W_MES_NAME,
                     q.U_NAME,
                     q.U_IP,
                     q.U_HOST_NAME,
                     q.DATE_WRITE
              from (select to_char(a.id) id,
                           to_char(a.DATE_FROM, 'dd.mm.yyyy') DATE_FROM,
                           a.date_from date_from_,
                           case when P_LANG='RUS' then D.NAME_RUS_SMALL||'...['||D.NAME_RUS_FULL||']' else D.NAME_ENG_SMALL||'...['||D.NAME_ENG_FULL||']' end D_MES_NAME,
                           case when P_LANG='RUS' then L.NAME_RUS_SMALL||'...['||L.NAME_RUS_FULL||']' else L.NAME_ENG_SMALL||'...['||L.NAME_ENG_FULL||']' end L_MES_NAME,
                           case when P_LANG='RUS' then V.NAME_RUS_SMALL||'...['||V.NAME_RUS_FULL||']' else V.NAME_ENG_SMALL||'...['||V.NAME_ENG_FULL||']' end V_MES_NAME,
                           case when P_LANG='RUS' then W.NAME_RUS_SMALL||'...['||W.NAME_RUS_FULL||']' else W.NAME_ENG_SMALL||'...['||W.NAME_ENG_FULL||']' end W_MES_NAME,
                           a.U_NAME,
                           a.U_IP,
                           a.U_HOST_NAME,
                           to_char(a.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss') DATE_WRITE
                    from WB_REF_AIRCO_MEAS a join WB_REF_MEASUREMENT_DENSITY d
                    on a.ID_AC=P_AIRCO_ID and
                       d.id=a.DENSITY_ID join WB_REF_MEASUREMENT_LENGTH l
                       on l.id=a.LENGTH_ID join WB_REF_MEASUREMENT_VOLUME v
                          on v.id=a.VOLUME_ID join WB_REF_MEASUREMENT_WEIGHT W
                             on w.id=a.WEIGHT_ID) q
              order by q.date_from_) qq;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;

    cXML_out:=cXML_out||'</root>';

    if cXML_out='<?xml version="1.0" ?><root></root>'
      then cXML_out:='';
    end if;
  END SP_WB_REF_AC_GET_MEAS_DATA;
/
