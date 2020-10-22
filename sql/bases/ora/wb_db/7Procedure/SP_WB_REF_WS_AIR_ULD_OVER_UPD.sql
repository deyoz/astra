create or replace procedure SP_WB_REF_WS_AIR_ULD_OVER_UPD
(cXML_in in clob,
   cXML_out out clob)
as

P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
P_POSITION varchar2(100):='';

P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_STR_OVERLAY clob:='EMPTY_STRING';
V_STR_CHANGE clob:='EMPTY_STRING';
V_R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_POSITION[1]') into P_POSITION from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

     insert into WB_TEMP_XML_ID (ID,
                                   STRING_VAL)
     select 1,
              f.P_OVERLAY
     from (select extractValue(value(t),'list/P_OVERLAY[1]') as P_OVERLAY
           from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_SEC_BAY_T
    where id_ac=P_ID_AC and
          id_ws=P_ID_WS and
          id_bort=P_ID_BORT and
          SEC_BAY_NAME=P_POSITION;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then V_STR_MSG:='The selected record "Position" is removed from the block "Sections/Bays"->"Sec/Bay Name"!'; end if;
        if P_LANG='RUS' then V_STR_MSG:='Выбранная запись "Position" удалена из блока "Sections/Bays"->"Sec/Bay Name"!'; end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(t.id) into V_R_COUNT
        from WB_TEMP_XML_ID t
        where not exists(select 1
                         from WB_REF_WS_AIR_SEC_BAY_T bt
                         where bt.id_ac=P_ID_AC and
                               bt.id_ws=P_ID_WS and
                               bt.id_bort=P_ID_BORT and
                               bt.SEC_BAY_NAME=t.STRING_VAL);

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then V_STR_MSG:='Some names of "Overload" removed from the block "Sections Bays"->"Sec/Bay Name"!'; end if;
            if P_LANG='RUS' then V_STR_MSG:='Некоторые наименования "Overload" удалены из блока "Sections/Bays"->"Sec/Bay Name"!'; end if;
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    if (V_STR_MSG is null) then
      begin
        insert into WB_REF_WS_AIR_ULD_OVER_HST(ID_,
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
                                                                   POSITION,
	                                                                   OVERLAY,
	                                                                     U_NAME,
		                                                                     U_IP,
		                                                                       U_HOST_NAME,
		                                                                         DATE_WRITE)
        select SEC_WB_REF_WS_AIR_ULD_OVER_HST.nextval,
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
                                     i.POSITION,
	                                     i.OVERLAY,
                                         i.U_NAME,
		                                   	   i.U_IP,
		                                  	     i.U_HOST_NAME,
		                                 	         i.DATE_WRITE
        from WB_REF_WS_AIR_ULD_OVER i
        where i.id_ac=P_ID_AC and
              i.id_ws=P_ID_WS and
              i.id_bort=P_ID_BORT and
              i.position=P_POSITION;

        delete from WB_REF_WS_AIR_ULD_OVER
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT and
              position=P_POSITION;

        insert into WB_REF_WS_AIR_ULD_OVER (ID,
	                                            ID_AC,
	                                              ID_WS,
	                                                ID_BORT,
	                                                  POSITION,
	                                                    OVERLAY,
	                                                      U_NAME,
	                                                        U_IP,
	                                                          U_HOST_NAME,
	                                                            DATE_WRITE)
        select SEC_WB_REF_WS_AIR_ULD_OVER.nextval,
                 p_ID_AC,
                   P_ID_WS,
                     P_ID_BORT,
                       P_POSITION,
                         STRING_VAL,
                           P_U_NAME,
	                           P_U_IP,
	                             P_U_HOST_NAME,
                                 sysdate()
        from WB_TEMP_XML_ID;

        select WB_WS_AIR_ULD_OVER_STR(P_ID_AC,
                                        P_ID_WS,
                                          P_ID_BORT,
                                            P_POSITION) into V_STR_OVERLAY
        from dual;

        select count(id) into V_R_COUNT
        from WB_TEMP_XML_ID;

        if V_R_COUNT>0 then
          begin
            select P_U_NAME||'...['||
                     to_char(sysdate(), 'dd.mm.yyyy hh24:mm:ss')||
                       ']' into V_STR_CHANGE
            from dual;
          end;
        end if;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_overlay="'||WB_CLEAR_XML(V_STR_OVERLAY)||'" '||
                              'str_change="'||WB_CLEAR_XML(V_STR_CHANGE)||'" '||
                              'str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_ULD_OVER_UPD;
/
