create or replace procedure SP_WB_REF_WS_AIR_SEC_B_TT_UPD
(cXML_in in clob,
   cXML_out out clob)
as

P_ID number:=-1;

P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID[1]')) into P_ID from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

     insert into WB_TEMP_XML_ID (ID,
                                   num)
     select f.ULD_IATA_ID,
              f.ULD_IATA_ID
     from (select to_number(extractValue(value(t),'list/ULD_IATA_ID[1]')) as ULD_IATA_ID
           from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_SEC_BAY_T
    where id=P_ID;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then V_STR_MSG:='The selected record "Sections/Bays" block deleted!'; end if;
        if P_LANG='RUS' then V_STR_MSG:='Выбранная запись блока "Sections/Bays" удалена!'; end if;
      end;
    end if;
    ----------------------------------------------------------------------------
   if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_TEMP_XML_ID t
        where not exists(select 1
                         from WB_REF_WS_AIR_SEC_BAY_T bt join WB_REF_AIRCO_ULD u
                         on bt.id=P_ID and
                            bt.id_ac=u.id_ac and
                            u.ULD_IATA_ID=t.id join WB_REF_ULD_IATA iata
                            on iata.id=u.ULD_IATA_ID);

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then V_STR_MSG:='Some records for "IATA ULD Type" removed from block "Airline Standarts-> ULD Specifications"'; end if;
            if P_LANG='RUS' then V_STR_MSG:='Некоторые записи для "IATA ULD Type" удалены из блока "Airline Standarts->ULD Specifications"!'; end if;
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------


    if (V_STR_MSG is null) then
      begin
        insert into WB_REF_WS_AIR_SEC_BAY_TT_HST(ID_,
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
                                                                       T_ID,
	                                                                       ULD_IATA_ID,
	                                                                         U_NAME,
		                                                                         U_IP,
		                                                                           U_HOST_NAME,
		                                                                             DATE_WRITE)
        select SEC_WB_REF_WS_AIR_SEC_BAY_TT_H.nextval,
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
                                     i.T_ID,
	                                     i.ULD_IATA_ID,
                                         i.U_NAME,
		                                     	 i.U_IP,
		                                      	 i.U_HOST_NAME,
		                                     	     i.DATE_WRITE
        from WB_REF_WS_AIR_SEC_BAY_TT i
        where i.T_ID=P_ID;

        delete from WB_REF_WS_AIR_SEC_BAY_TT where T_ID=P_ID;

        select d.IS_ULD_IATA into V_R_COUNT
        from WB_REF_WS_AIR_SEC_BAY_T h join WB_REF_SEC_BAY_TYPE d
        on h.id=P_ID and
           h.SEC_BAY_TYPE_ID=d.id;

        if V_R_COUNT=1 then
          begin
            insert into WB_REF_WS_AIR_SEC_BAY_TT (ID,
	                                                  ID_AC,
	                                                    ID_WS,
	                                                      ID_BORT,
	                                                        T_ID,
	                                                          ULD_IATA_ID,
	                                                            U_NAME,
	                                                              U_IP,
	                                                                U_HOST_NAME,
	                                                                  DATE_WRITE)
            select SEC_WB_REF_WS_AIR_SEC_BAY_TT.nextval,
                     s.ID_AC,
	                     s.ID_WS,
	                       s.ID_BORT,
                           s.id,
                             t.id,
                               P_U_NAME,
	                               P_U_IP,
	                                 P_U_HOST_NAME,
                                     sysdate()
            from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SEC_BAY_T s
            on s.id=P_ID;
          end;
        end if;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_SEC_B_TT_UPD;
/
