create or replace procedure SP_WB_REF_WS_AIR_CMP_T_DEL
(cXML_in in clob,
   cXML_out out clob)
as
P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_LANG[1]') into P_LANG from dual;


    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    insert into WB_TEMP_XML_ID (ID,
                                  num)
    select distinct f.id,
                      f.id
    from (select to_number(extractValue(value(t),'list/P_ID[1]')) as id
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_TEMP_XML_ID_EX (id,
                                     ACTION_NAME,
                                       F_FLT_1,
                                         F_FLT_2,
                                           F_FLT_3,
                                             F_STR_1)
    select distinct cc.id,
                      'SP_WB_REF_WS_AIR_CMP_T_DEL',
                         cc.id_ac,
                           cc.id_ws,
                             cc.id_bort,
                              cc.CMP_NAME
    from WB_REF_WS_AIR_HLD_CMP_T c join WB_TEMP_XML_ID t
    on c.id=t.id join WB_REF_WS_AIR_HLD_CMP_T cc
       on cc.id_ac=c.id_ac and
          cc.id_ws=c.id_ws and
          cc.id_bort=c.id_bort;

    select count(distinct(sbt.id)) into V_R_COUNT
    from WB_REF_WS_AIR_SEC_BAY_T sbt join WB_TEMP_XML_ID_EX t
    on sbt.id_ac=t.F_FLT_1 and
       sbt.id_ws=t.F_FLT_2 and
       sbt.id_bort=t.F_FLT_3 and
       not exists(select tt.id
                  from WB_TEMP_XML_ID_EX tt
                  where tt.F_FLT_1=sbt.id_ac and
                        tt.F_FLT_2=sbt.id_ws and
                        tt.F_FLT_3=sbt.id_bort and
                        tt.F_STR_1=sbt.CMP_NAME and
                        tt.id not in (select ttt.id from WB_TEMP_XML_ID ttt));

    if V_R_COUNT>0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='On the field "Comp Name" referenced in the section "Sections/Bays"!';
          end;
        else
          begin
            V_STR_MSG:='На поле "Comp Name" имеются ссылки в блоке "Sections/Bays"!';
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    if (V_STR_MSG is null) then
      begin
        insert into WB_REF_WS_AIR_HLD_CMP_T_HST(ID_,
	                                                U_NAME_,
     	                                              U_IP_,
	                                                    U_HOST_NAME_,
	                                                      DATE_WRITE_,
                                                          OPERATION_,
      	                                                    ACTION_,
	                                                            ID,
	                                                              ID_AC,
	                                                                ID_WS,
		                                                                ID_BORT,
                                                                      HOLD_ID,
	                                                                      CMP_NAME,
	                                                                        MAX_WEIGHT,
	                                                                          MAX_VOLUME,
	                                                                            LA_CENTROID,
	                                                                              LA_FROM,
	                                                                                LA_TO,
	                                                                                  BA_CENTROID,
	                                                                                    BA_FWD,
	                                                                                      BA_AFT,
	                                                                                        INDEX_PER_WT_UNIT,
	                                                                                          U_NAME,
		                                                                                          U_IP,
		                                                                                            U_HOST_NAME,
		                                                                                              DATE_WRITE)
    select SEC_WB_REF_WS_AIR_HLD_CMP_T_HS.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       'delete',
                         i.id,
                           i.ID_AC,
	                           i.ID_WS,
		                           i.ID_BORT,
                                 i.HOLD_ID,
	                                 i.CMP_NAME,
	                                   i.MAX_WEIGHT,
	                                     i.MAX_VOLUME,
	                                       i.LA_CENTROID,
	                                         i.LA_FROM,
	                                           i.LA_TO,
	                                             i.BA_CENTROID,
	                                               i.BA_FWD,
	                                                 i.BA_AFT,
	                                                   i.INDEX_PER_WT_UNIT,
                                                       i.U_NAME,
		                                  	                 i.U_IP,
		                                	                     i.U_HOST_NAME,
		                                 	                       i.DATE_WRITE
         from WB_TEMP_XML_ID t join WB_REF_WS_AIR_HLD_CMP_T i
         on i.id=t.id;

         delete from WB_REF_WS_AIR_HLD_CMP_T
         where exists(select 1
                      from WB_TEMP_XML_ID t
                      where WB_REF_WS_AIR_HLD_CMP_T.id=t.id);

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_CMP_T_DEL;
/
