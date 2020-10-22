create or replace PROCEDURE SP_WB_REF_WS_AIR_MEAS_INFO(cXML_in IN clob,
                                                         cXML_out OUT CLOB)
AS
P_ID number:=-1;
P_LANG varchar2(50):='';
cXML_data clob:='';
R_COUNT number:=0;
  BEGIN
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;

    select count(id) into R_COUNT
    from WB_REF_WS_AIR_MEASUREMENT
    where id=P_ID;

    if R_COUNT>0 then
      begin
        cXML_out:='<?xml version="1.0" ?><root>';

        select '<list DATE_FROM="'||to_char(i.DATE_FROM, 'dd.mm.yyyy')||'" '||

                      'MEAS_WEIGHT="'||
                      case when P_LANG='RUS'
                           then WB_CLEAR_XML(mw.NAME_RUS_SMALL||'...['||mw.NAME_RUS_FULL||']')
                           else WB_CLEAR_XML(mw.NAME_ENG_SMALL||'...['||mw.NAME_ENG_FULL||']')
                      end||'" '||

                      'MEAS_LENGTH="'||
                      case when P_LANG='RUS'
                           then WB_CLEAR_XML(ml.NAME_RUS_SMALL||'...['||ml.NAME_RUS_FULL||']')
                           else WB_CLEAR_XML(ml.NAME_ENG_SMALL||'...['||ml.NAME_ENG_FULL||']')
                      end||'" '||

                     'MEAS_LIQUID_VOLUME="'||
                      case when P_LANG='RUS'
                           then WB_CLEAR_XML(mlv.NAME_RUS_SMALL||'...['||mlv.NAME_RUS_FULL||']')
                           else WB_CLEAR_XML(mlv.NAME_ENG_SMALL||'...['||mlv.NAME_ENG_FULL||']')
                      end||'" '||

                     'MEAS_VOLUME="'||
                      case when P_LANG='RUS'
                           then WB_CLEAR_XML(mv.NAME_RUS_SMALL||'...['||mv.NAME_RUS_FULL||']')
                           else WB_CLEAR_XML(mv.NAME_ENG_SMALL||'...['||mv.NAME_ENG_FULL||']')
                      end||'" '||

                     'MEAS_FUEL_DENSITY="'||
                      case when P_LANG='RUS'
                           then WB_CLEAR_XML(md.NAME_RUS_SMALL||'...['||md.NAME_RUS_FULL||']')
                           else WB_CLEAR_XML(md.NAME_ENG_SMALL||'...['||md.NAME_ENG_FULL||']')
                      end||'" '||

                      'MEAS_MOMENTS="'||
                      case when P_LANG='RUS'
                           then WB_CLEAR_XML(mm.NAME_RUS_SMALL||'...['||mm.NAME_RUS_FULL||']')
                           else WB_CLEAR_XML(mm.NAME_ENG_SMALL||'...['||mm.NAME_ENG_FULL||']')
                      end||'" '||

                     'REMARK="'||WB_CLEAR_XML(nvl(i.REMARK, 'NULL'))||'" '||
                     'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                     'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                     'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                     'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
           into cXML_data
           from WB_REF_WS_AIR_MEASUREMENT i join WB_REF_MEASUREMENT_WEIGHT mw
           on i.id=P_ID and
              i.WEIGHT_ID=mw.id join WB_REF_MEASUREMENT_LENGTH ml
              on i.length_id=ml.id join WB_REF_MEASUREMENT_VOLUME mlv
                 on mlv.id=i.VOLUME_ID_LIQUID join WB_REF_MEASUREMENT_VOLUME mv
                    on mv.id=i.VOLUME_ID join WB_REF_MEASUREMENT_DENSITY md
                       on md.id=i.DENSITY_ID_FUEL join WB_REF_MEASUREMENT_MOMENTS mm
                          on mm.id=i.MOMENTS_ID;

        cXML_out:=cXML_out||cXML_data||'</root>';
      end;
    end if;

  END SP_WB_REF_WS_AIR_MEAS_INFO;
/
