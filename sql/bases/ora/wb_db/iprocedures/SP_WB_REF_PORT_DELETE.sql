create or replace procedure SP_WB_REF_PORT_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
rec_id number:=-1;
id_hist number:=-1;
lang varchar2(50):='';
p_U_NAME varchar2(50):='';
p_U_IP varchar2(50):='';
p_U_COMP_NAME varchar2(50):='';
p_U_HOST_NAME varchar2(50):='';
V_STR_MSG varchar2(1000):=null;
V_R_COUNT int:=0;
  begin
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/lang[1]') into lang from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/id[1]') into rec_id from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_NAME[1]') into p_U_NAME from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_IP[1]') into p_U_IP from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_COMP_NAME[1]') into p_U_COMP_NAME from dual;
   select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_HOST_NAME[1]') into p_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';
   -----------------------------------------------------------------------------
   --------------------------èêéÇÖêäà çÄ àëèéãúáéÇÄçàÖ--------------------------
   select count(id) into V_R_COUNT
   from WB_REF_AIRCO_PORT_FLIGHT
   where ID_PORT=rec_id;

   if V_R_COUNT>0 then
     begin
       if lang='ENG' then
         begin
           V_STR_MSG:='There is a link in block "Airline Information"->"General Airline Information"->"Airports"!';
         end;
       else
         begin
           V_STR_MSG:='à¨•Ó‚·Ô ··Î´™® ¢ °´Æ™• "Airline Information"->"General Airline Information"->"Airports"!';
         end;
       end if;

     end;
   end if;
   -----------------------------------------------------------------------------
   -----------------------------------------------------------------------------
   if V_STR_MSG is NULL then
     begin
       select count(id) into V_R_COUNT
       from WB_SHED
       where ID_AP_1=rec_id or
             ID_AP_2=rec_id;


       if V_R_COUNT>0 then
         begin
           if lang='ENG' then
             begin
               V_STR_MSG:='There is a link in block "Schedule"!';
             end;
           else
             begin
               V_STR_MSG:='à¨•Ó‚·Ô ··Î´™® ¢ °´Æ™• "ê†·Ø®·†≠®•"!';
             end;
           end if;
         end;
       end if;
     end;
   end if;
   --------------------------èêéÇÖêäà çÄ àëèéãúáéÇÄçàÖ--------------------------
   -----------------------------------------------------------------------------

    if (V_STR_MSG is null) then
      begin
        id_hist:=SEC_WB_REF_AIRPORTS_HISTORY.nextval();

        insert into WB_REF_AIRPORTS_HISTORY (ID_,
	                                          U_NAME_,
	                                            U_IP_,
	                                              U_HOST_NAME_,
	                                                DATE_WRITE_,
	                                                  ACTION,
	                                                    ID,
	                                                      ID_CITY_OLD,
                                                          AP_OLD,
                                                            IATA_OLD,
                                                              IKAO_OLD,
	                                                              NAME_RUS_SMALL_OLD,
	                                                                NAME_RUS_FULL_OLD,
	                                                                  NAME_ENG_SMALL_OLD,
	                                                                    NAME_ENG_FULL_OLD,
	                                                                      REMARK_OLD,
	                                                                        U_NAME_OLD,
	                                                                          U_IP_OLD,
	                                                                            U_HOST_NAME_OLD,
	                                                                              DATE_WRITE_OLD,
	                                                                                ID_CITY_NEW,
                                                                                    AP_NEW,
                                                                                      IATA_NEW,
                                                                                        IKAO_NEW,
	                                                                                        NAME_RUS_SMALL_NEW,
	                                                                                          NAME_RUS_FULL_NEW,
	                                                                                            NAME_ENG_SMALL_NEW,
	                                                                                              NAME_ENG_FULL_NEW,
	                                                                                                REMARK_NEW,
	                                                                                                  U_NAME_NEW,
	                                                                                                    U_IP_NEW,
	                                                                                                      U_HOST_NAME_NEW,
	                                                                                                        DATE_WRITE_NEW)
        select id_hist,
                 p_U_NAME,
	                 p_U_IP,
	                   p_U_HOST_NAME,
                       SYSDATE,
                         'delete',
                            rec_id,
                              id_city,
                                AP,
                                  IATA,
                                    IKAO,
	                                    NAME_RUS_SMALL,
	                                      NAME_RUS_FULL,
	                                        NAME_ENG_SMALL,
	                                          NAME_ENG_FULL,
	                                            REMARK,
	                                              U_NAME,
	                                                U_IP,
	                                                  U_HOST_NAME,
                                                      SYSDATE,
                                                        id_city,
                                                          AP,
                                                            IATA,
                                                              IKAO,
	                                                              NAME_RUS_SMALL,
	                                                                NAME_RUS_FULL,
	                                                                  NAME_ENG_SMALL,
	                                                                    NAME_ENG_FULL,
	                                                                      REMARK,
	                                                                        U_NAME,
	                                                                          U_IP,
	                                                                            U_HOST_NAME,
                                                                                SYSDATE
        from WB_REF_AIRPORTS
        where id=rec_id;

        delete from WB_REF_AIRPORTS
        where id=rec_id;
      end;
      end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_PORT_DELETE;
/
