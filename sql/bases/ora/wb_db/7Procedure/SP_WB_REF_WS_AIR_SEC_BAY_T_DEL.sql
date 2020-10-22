create or replace procedure SP_WB_REF_WS_AIR_SEC_BAY_T_DEL
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

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID (ID,
                                  num)
    select f.P_ID,
             f.P_ID
    from (select to_number(extractValue(value(t),'list/P_ID[1]')) as P_ID
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    ----------------------------------------------------------------------------
    ----------------------------ÇéáåéÜçõÖ èêéÇÖêäà------------------------------
    select count(o.id) into V_R_COUNT
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SEC_BAY_T sbt
    on t.id=sbt.id join WB_REF_WS_AIR_ULD_OVER o
       on o.id_ac=sbt.id_ac and
          o.id_ws=sbt.id_ws and
          o.id_bort=sbt.id_bort and
          o.position=sbt.SEC_BAY_NAME;

    if V_R_COUNT>0 then
      begin
        if P_LANG='ENG' then V_STR_MSG:='There are links in the "Position" block "ULD Overlay"!'; end if;
        if P_LANG='RUS' then V_STR_MSG:='à¨•Ó‚·Ô ··Î´™® ¢ ØÆ´• "Position" °´Æ™† "ULD Overlay"!'; end if;
      end;
    end if;

    if V_STR_MSG is null then
      begin
        select count(o.id) into V_R_COUNT
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SEC_BAY_T sbt
        on t.id=sbt.id join WB_REF_WS_AIR_ULD_OVER o
           on o.id_ac=sbt.id_ac and
              o.id_ws=sbt.id_ws and
              o.id_bort=sbt.id_bort and
              o.overlay=sbt.SEC_BAY_NAME;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then V_STR_MSG:='There are links in the "Overlay" block "ULD Overlay"!'; end if;
            if P_LANG='RUS' then V_STR_MSG:='à¨•Ó‚·Ô ··Î´™® ¢ ØÆ´• "Overlay" °´Æ™† "ULD Overlay"!'; end if;
          end;
        end if;
      end;
    end if;
    ----------------------------ÇéáåéÜçõÖ èêéÇÖêäà------------------------------
    ----------------------------------------------------------------------------

    if V_STR_MSG is null then
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
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SEC_BAY_TT i
        on i.t_id=t.id;

        delete from WB_REF_WS_AIR_SEC_BAY_TT
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where WB_REF_WS_AIR_SEC_BAY_TT.t_id=t.id);
        ------------------------------------------------------------------------
        insert into WB_REF_WS_AIR_SEC_BAY_T_HST(ID_,
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
                                                                      CMP_NAME,
	                                                                      SEC_BAY_NAME,
	                                                                        SEC_BAY_TYPE_ID,
	                                                                          MAX_WEIGHT,
	                                                                            MAX_VOLUME,
	                                                                              LA_CENTROID,
	                                                                                LA_FROM,
	                                                                                  LA_TO,
	                                                                                    BA_CENTROID,
	                                                                                      BA_FWD,
	                                                                                        BA_AFT,
	                                                                                          INDEX_PER_WT_UNIT,
	                                                                                            DOOR_POSITION,
	                                                                                              U_NAME,
		                                                                                              U_IP,
		                                                                                                U_HOST_NAME,
		                                                                                                  DATE_WRITE,
                                                                                                        COLOR)
        select SEC_WB_REF_WS_AIR_SEC_BAY_T_HS.nextval,
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
                                     i.CMP_NAME,
	                                     i.SEC_BAY_NAME,
	                                       i.SEC_BAY_TYPE_ID,
	                                         i.MAX_WEIGHT,
	                                           i.MAX_VOLUME,
	                                             i.LA_CENTROID,
	                                               i.LA_FROM,
	                                                 i.LA_TO,
	                                                   i.BA_CENTROID,
	                                                     i.BA_FWD,
	                                                       i.BA_AFT,
	                                                         i.INDEX_PER_WT_UNIT,
	                                                           i.DOOR_POSITION,
	                                                             i.U_NAME,
		                               	                             i.U_IP,
		                                	                             i.U_HOST_NAME,
		                                 	                               i.DATE_WRITE,
                                                                       i.COLOR
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SEC_BAY_T i
        on i.id=t.id;

        delete from WB_REF_WS_AIR_SEC_BAY_T
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where WB_REF_WS_AIR_SEC_BAY_T.id=t.id);

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_SEC_BAY_T_DEL;
/
