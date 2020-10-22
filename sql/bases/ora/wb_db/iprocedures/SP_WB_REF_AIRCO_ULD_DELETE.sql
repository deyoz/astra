create or replace procedure SP_WB_REF_AIRCO_ULD_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT number:=0;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID (ID,
                                  num)
    select distinct f.id,
                      f.id
    from (select to_number(extractValue(value(t),'list/P_ID[1]')) as id
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    insert into WB_TEMP_XML_ID_EX (id,
                                     ACTION_NAME,
                                       F_FLT_1,
                                         F_FLT_2)
    select distinct cc.id,
                      'SP_WB_REF_AIRCO_ULD_DELETE',
                         cc.id_ac,
                           cc.ULD_IATA_ID
    from WB_REF_AIRCO_ULD c join WB_TEMP_XML_ID t
    on c.id=t.id join WB_REF_AIRCO_ULD cc
       on cc.id_ac=c.id_ac;

    select count(distinct(sbt.id)) into V_R_COUNT
    from WB_REF_WS_AIR_SEC_BAY_TT sbt join WB_TEMP_XML_ID_EX t
    on sbt.id_ac=t.F_FLT_1 and
       not exists(select tt.id
                  from WB_TEMP_XML_ID_EX tt
                  where tt.F_FLT_1=sbt.id_ac and
                        tt.F_FLT_2=sbt.ULD_IATA_ID and
                        tt.id not in (select ttt.id from WB_TEMP_XML_ID ttt));

   if V_R_COUNT>0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='On the field "IATA ULD Type" referenced in blocks "Airline Fleet"->"Holds"->"Sections/Bays"!';
          end;
        else
          begin
            V_STR_MSG:='На поле "IATA ULD Type" имеются ссылки в блоках "Airline Fleet"->"Holds"->"Sections/Bays"!';
          end;
        end if;
      end;
    end if;

   if V_STR_MSG is null then
     begin
       insert into WB_REF_AIRCO_ULD_HIST(ID_,
	                                          U_NAME_,
	                                            U_IP_,
	                                              U_HOST_NAME_,
    	                                            DATE_WRITE_,
                                                    OPERATION_,
	                                                    ACTION_,
	                                                      ID,
	                                                        ID_AC,	
	                                                          ULD_TYPE_ID,
	                                                            ULD_ATA_ID,
	                                                              ULD_IATA_ID,
	                                                                BEG_SER_NUM,
	                                                                  END_SER_NUM,
	                                                                    OWNER_CODE,
	                                                                      TARE_WEIGHT,
	                                                                        MAX_WEIGHT,
	                                                                          MAX_VOLUME,
	                                                                            WIDTH,
	                                                                              LENGTH,
	                                                                                HEIGHT,
	                                                                                  BY_DEFAULT,
	                                                                                    U_NAME,
		                                                                                    U_IP,
		                                                                                      U_HOST_NAME,
		                                                                                        DATE_WRITE)
        select SEC_WB_REF_AIRCO_ULD_HIST.nextval,
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE(),
                         'delete',
                           'delete',
                              i.id,
                                i.ID_AC,
	                                i.ULD_TYPE_ID,
	                                  i.ULD_ATA_ID,
	                                    i.ULD_IATA_ID,
	                                      i.BEG_SER_NUM,
	                                        i.END_SER_NUM,
	                                          i.OWNER_CODE,
    	                                        i.TARE_WEIGHT,
	                                              i.MAX_WEIGHT,
	                                                i.MAX_VOLUME,
	                                                  i.WIDTH,
	                                                    i.LENGTH,
	                                                      i.HEIGHT,
	                                                        i.BY_DEFAULT,
	                                                          i.U_NAME,
	                                                            i.U_IP,
	                                                              i.U_HOST_NAME,
	                                                                i.DATE_WRITE
        from WB_TEMP_XML_ID t join WB_REF_AIRCO_ULD i
        on i.id=t.id;

        delete from WB_REF_AIRCO_ULD
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_AIRCO_ULD.id);

          V_STR_MSG:='EMPTY_STRING';
        end;
      end if;

      cXML_out:=cXML_out||'<list str_msg="'||V_STR_MSG||'"/>'||'</root>';
    commit;
  end SP_WB_REF_AIRCO_ULD_DELETE;
/
