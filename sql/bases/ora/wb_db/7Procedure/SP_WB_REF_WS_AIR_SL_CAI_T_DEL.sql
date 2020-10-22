create or replace procedure SP_WB_REF_WS_AIR_SL_CAI_T_DEL
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
P_ACTION_NAME varchar2(200):='SP_WB_REF_WS_AIR_SL_CAI_T_DEL';
P_ACTION_DATE date:=sysdate();
str_msg clob:=null;
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




    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    if V_STR_MSG is null then
      begin
        insert into WB_REF_WS_AIR_SL_CAI_T_HST(ID_,
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
	                                                                       CABIN_SECTION,
	                                                                         BA_CENTROID,
	                                                                           BA_FWD,
	                                                                             BA_AFT,
	                                                                               INDEX_PER_WT_UNIT,
		                                                                               U_NAME,
		                                                                                 U_IP,
		                                                                                   U_HOST_NAME,
		                                                                                     DATE_WRITE)
        select SEC_WB_REF_WS_AIR_SL_CAI_T_HST.nextval,
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
	                                       i.CABIN_SECTION,
	                                         i.BA_CENTROID,
	                                           i.BA_FWD,
	                                             i.BA_AFT,
	                                               i.INDEX_PER_WT_UNIT,
                                                   i.U_NAME,
		                                            	   i.U_IP,
		                                                   i.U_HOST_NAME,
		                                 	                   i.DATE_WRITE
        from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SL_CAI_T i
        on i.id=t.id and
           t.ACTION_NAME=P_ACTION_NAME and
           t.ACTION_DATE=P_ACTION_DATE;

        delete from WB_REF_WS_AIR_SL_CAI_T
        where exists(select 1
                     from WB_TEMP_XML_ID_EX t
                     where WB_REF_WS_AIR_SL_CAI_T.id=t.id and
                           t.ACTION_NAME=P_ACTION_NAME and
                           t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
        insert into WB_REF_WS_AIR_SL_CAI_TT_HST(ID_,
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
	                                                                        T_ID,
	                                                                          CLASS_CODE,
	                                                                            NUM_OF_SEATS,
		                                                                            U_NAME,
		                                                                              U_IP,
		                                                                                U_HOST_NAME,
		                                                                                  DATE_WRITE)
        select SEC_WB_REF_WS_AIR_SL_CAI_TT_HS.nextval,
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
	                                       i.T_ID,
	                                         i.CLASS_CODE,
	                                           i.NUM_OF_SEATS,
                                               i.U_NAME,
		                                             i.U_IP,
    		                                           i.U_HOST_NAME,
		                                     	           i.DATE_WRITE
        from WB_TEMP_XML_ID_EX t join WB_REF_WS_AIR_SL_CAI_TT i
        on i.t_id=t.id and
           t.ACTION_NAME=P_ACTION_NAME and
           t.ACTION_DATE=P_ACTION_DATE;

        delete from WB_REF_WS_AIR_SL_CAI_TT
        where exists(select 1
                     from WB_TEMP_XML_ID_EX t
                     where WB_REF_WS_AIR_SL_CAI_TT.t_id=t.id and
                           t.ACTION_NAME=P_ACTION_NAME and
                           t.ACTION_DATE=P_ACTION_DATE);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

        str_msg:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(str_msg)||'"/>'||'</root>';
    commit;
  end SP_WB_REF_WS_AIR_SL_CAI_T_DEL;
/
