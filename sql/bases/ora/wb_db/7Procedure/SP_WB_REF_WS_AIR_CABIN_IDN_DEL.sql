create or replace procedure SP_WB_REF_WS_AIR_CABIN_IDN_DEL
(cXML_in in clob,
   cXML_out out clob)
as
P_LANG varchar2(200):='ENG';
V_RCOUNT number:=0;
V_STR_MSG clob:=null;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
P_ACTION_NAME varchar2(200):='SP_WB_REF_WS_AIR_CABIN_IDN_DEL';
P_ACTION_DATE date:=sysdate();
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;


    P_ACTION_NAME:=P_ACTION_NAME||P_U_NAME||P_U_IP;

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID_EX (ID,
                                     ACTION_NAME,
                                       ACTION_DATE)
    select distinct f.id,
                      P_ACTION_NAME,
                        P_ACTION_DATE
    from (select to_number(extractValue(value(t),'list/P_ID[1]')) as id
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    select count(sl.id) into V_RCOUNT
    from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SEAT_LAY_ADV sl
    on sl.cabin_id=t.id and
       t.ACTION_NAME=P_ACTION_NAME and
       t.ACTION_DATE=P_ACTION_DATE;

    if V_RCOUNT>0 then
      begin
        if P_LANG='ENG' then V_STR_MSG:='There are links in the "Seating Layout"!'; end if;
        if P_LANG='RUS' then V_STR_MSG:='Имеются ссылки в блоке "Seating Layout"!'; end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    if V_STR_MSG is null then
      begin
        insert into WB_REF_WS_AIR_CABIN_IDN_HST(ID_,
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
		                                                                  TABLE_NAME,
		                                                                    U_NAME,
		                                                                      U_IP,
		                                                                        U_HOST_NAME,
		                                                                          DATE_WRITE)
        select SEC_WB_REF_WS_AIR_CABIN_IDN_HS.nextval,
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE(),
                         'delete',
                           'delete',
                             i.ID,
	                             i.ID_AC,
	                               i.ID_WS,
		                               i.ID_BORT,
                                     i.TABLE_NAME,
		                                   i.U_NAME,
		                                     i.U_IP,
		                                       i.U_HOST_NAME,
		                                         i.DATE_WRITE
        from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_CABIN_IDN i
        on i.id=t.id and
           t.ACTION_NAME=P_ACTION_NAME and
           t.ACTION_DATE=P_ACTION_DATE;

        delete from WB_REF_WS_AIR_CABIN_IDN
        where exists(select 1
                     from WB_TEMP_XML_ID_EX t
                     where t.id=WB_REF_WS_AIR_CABIN_IDN.id and
                           t.ACTION_NAME=P_ACTION_NAME and
                           t.ACTION_DATE=P_ACTION_DATE);

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
        insert into WB_REF_WS_AIR_CABIN_ADV_HST(ID_,
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
		                                                                  IDN,
	                                                                      CD_BALANCE_ARM,
	                                                                        CD_INDEX_UNIT,
	                                                                          FDL_BALANCE_ARM,
	                                                                            FDL_INDEX_UNIT,
	                                                                              CCL_BALANCE_ARM,
	                                                                                CCL_INDEX_UNIT,
		                                                                                U_NAME,
		                                                                                  U_IP,
		                                                                                    U_HOST_NAME,
		                                                                                      DATE_WRITE,
                                                                                            DATE_FROM)
        select SEC_WB_REF_WS_AIR_CABIN_ADV_HS.nextval,
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
		                                 i.IDN,
	                                     i.CD_BALANCE_ARM,
	                                       i.CD_INDEX_UNIT,
	                                         i.FDL_BALANCE_ARM,
	                                           i.FDL_INDEX_UNIT,
	                                             i.CCL_BALANCE_ARM,
	                                               i.CCL_INDEX_UNIT,
		                         	                     i.U_NAME,
		                               	                 i.U_IP,
		                                	                 i.U_HOST_NAME,
		                                 	                   i.DATE_WRITE,
                                                       i.DATE_FROM
        from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_CABIN_ADV i
        on i.idn=t.id and
           t.ACTION_NAME=P_ACTION_NAME and
           t.ACTION_DATE=P_ACTION_DATE;

        delete from WB_REF_WS_AIR_CABIN_ADV
        where exists(select 1
                     from WB_TEMP_XML_ID_EX t
                     where t.id=WB_REF_WS_AIR_CABIN_ADV.idn and
                           t.ACTION_NAME=P_ACTION_NAME and
                           t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
        insert into WB_REF_WS_AIR_CABIN_CD_HST(ID_,
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
		                                                                 IDN,
                                                                       ADV_ID,
                                                                         DECK_ID,
	                                                                         SECTION,
	                                                                           ROWS_FROM,
	                                                                             ROWS_TO,
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
       select SEC_WB_REF_WS_AIR_CABIN_CD_HST.nextval,
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
		                                 i.IDN,
	                                     i.ADV_ID,
                                         i.DECK_ID,
    	                                     i.SECTION,
	                                           i.ROWS_FROM,
	                                             i.ROWS_TO,
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
         from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_CABIN_CD i
         on i.IDN=t.ID and
            t.ACTION_NAME=P_ACTION_NAME and
            t.ACTION_DATE=P_ACTION_DATE;

        delete from WB_REF_WS_AIR_CABIN_CD
        where exists(select 1
                     from WB_TEMP_XML_ID_EX t
                     where t.id=WB_REF_WS_AIR_CABIN_CD.idn and
                           t.ACTION_NAME=P_ACTION_NAME and
                           t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
        insert into WB_REF_WS_AIR_CABIN_FD_HST(ID_,
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
		                                                                 IDN,
                                                                       ADV_ID,
                                                                         FCL_ID,
	                                                                         MAX_NUM_SEATS,
	                                                                           LA_CENTROID,
	                                                                             BA_CENTROID,
	                                                                               INDEX_PER_WT_UNIT,
		                                                                               U_NAME,
		                                                                                 U_IP,
		                                                                                   U_HOST_NAME,
		                                                                                     DATE_WRITE)
        select SEC_WB_REF_WS_AIR_CABIN_FD_HST.nextval,
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
		                                 i.IDN,
	                                     i.ADV_ID,
                                         i.FCL_ID,
	                                         i.MAX_NUM_SEATS,
	                                           i.LA_CENTROID,
	                                             i.BA_CENTROID,
	                                               i.INDEX_PER_WT_UNIT,
		                         	                     i.U_NAME,
		                               	                 i.U_IP,
		                                	                 i.U_HOST_NAME,
		                                 	                   i.DATE_WRITE
         from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_CABIN_FD i
         on i.IDN=t.ID and
            t.ACTION_NAME=P_ACTION_NAME and
            t.ACTION_DATE=P_ACTION_DATE;

        delete from WB_REF_WS_AIR_CABIN_FD
        where exists(select 1
                     from WB_TEMP_XML_ID_EX t
                     where t.id=WB_REF_WS_AIR_CABIN_FD.idn and
                           t.ACTION_NAME=P_ACTION_NAME and
                           t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
        insert into WB_REF_WS_AIR_CABIN_CCL_HST(ID_,
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
		                                                                  IDN,
                                                                        ADV_ID,
                                                                          DECK_ID,
                                                                            LOCATION,
	                                                                            MAX_NUM_SEATS,
	                                                                              LA_CENTROID,
	                                                                                BA_CENTROID,
	                                                                                  INDEX_PER_WT_UNIT,
		                                                                                  U_NAME,
		                                                                                    U_IP,
		                                                                                      U_HOST_NAME,
		                                                                                        DATE_WRITE)
        select SEC_WB_REF_WS_AIR_CBN_CCL_HST.nextval,
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
		                                 i.IDN,
	                                     i.ADV_ID,
                                         i.DECK_ID,
                                           i.LOCATION,
	                                           i.MAX_NUM_SEATS,
	                                            i.LA_CENTROID,
	                                               i.BA_CENTROID,
	                                                 i.INDEX_PER_WT_UNIT,
		                         	                       i.U_NAME,
		                               	                   i.U_IP,
		                                	                   i.U_HOST_NAME,
		                                 	                     i.DATE_WRITE
         from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_CABIN_CCL i
         on i.IDN=t.ID and
            t.ACTION_NAME=P_ACTION_NAME and
            t.ACTION_DATE=P_ACTION_DATE;

        delete from WB_REF_WS_AIR_CABIN_CCL
        where exists(select 1
                     from WB_TEMP_XML_ID_EX t
                     where t.id=WB_REF_WS_AIR_CABIN_CCL.idn and
                           t.ACTION_NAME=P_ACTION_NAME and
                           t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';
    commit;
  end SP_WB_REF_WS_AIR_CABIN_IDN_DEL;
/
