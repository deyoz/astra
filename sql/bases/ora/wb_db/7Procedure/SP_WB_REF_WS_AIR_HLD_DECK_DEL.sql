create or replace procedure SP_WB_REF_WS_AIR_HLD_DECK_DEL
(cXML_in in clob,
   cXML_out out clob)
as
P_LANG varchar2(50):='ENG';
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

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID (ID,
                                  num)
    select distinct f.id,
                      -1
    from (select to_number(extractValue(value(t),'list/P_ID[1]')) as id
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    select count(h.id) into V_R_COUNT
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_HLD_DECK i
    on i.id=t.id join WB_REF_WS_AIR_HLD_HLD_T h
       on h.ID_AC=i.id_ac and
          h.ID_WS=i.id_WS and
          h.ID_BORT=i.ID_BORT and
          h.deck_id=i.deck_id;

    if V_R_COUNT>0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='In the value field "Deck" is referenced in the "Holds" block. The operation is prohibited!';
          end;
        else
          begin
            V_STR_MSG:='На значения поля "Deck" имеются ссылки в блоке "Holds". Операция запрещена!';
          end;
        end if;
      end;
    end if;

    if V_STR_MSG is null then
      begin
        insert into WB_REF_WS_AIR_HLD_DECK_HST(ID_,
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
                                                                     DECK_ID,
	                                                                     MAX_WEIGHT,
	                                                                       LA_FROM,
	                                                                         LA_TO,
	                                                                           BA_FWD,
	                                                                             BA_AFT,
	                                                                               U_NAME,
		                                                                               U_IP,
		                                                                                 U_HOST_NAME,
		                                                                                   DATE_WRITE,
                                                                                         MAX_VOLUME)
        select SEC_WB_REF_WS_AIR_HLD_DECK_HST.nextval,
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
	                                   i.DECK_ID,
	                                     i.MAX_WEIGHT,
	                                       i.LA_FROM,
	                                         i.LA_TO,
	                                           i.BA_FWD,
    	                                         i.BA_AFT,
	                                               i.U_NAME,
		                                 	             i.U_IP,
		                                  	             i.U_HOST_NAME,
		                                   	               i.DATE_WRITE,
                                                         i.MAX_VOLUME
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_HLD_DECK i
        on i.id=t.id;

        delete from WB_REF_WS_AIR_HLD_DECK
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_WS_AIR_HLD_DECK.id);


          V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

      cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';
    commit;
  end SP_WB_REF_WS_AIR_HLD_DECK_DEL;
/
