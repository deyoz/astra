create or replace procedure SP_WB_REF_WS_AIR_HLD_T_DEL
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
    select count(c.id) into V_R_COUNT
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_HLD_CMP_T c
    on t.id=c.HOLD_ID;

    if V_R_COUNT>0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='On the value "Hold Name" are referenced in the "Compartments" block! The operation is prohibited!';
          end;
        else
          begin
            V_STR_MSG:='�� ���祭�� "Hold Name" ������� ��뫪� � ����� "Compartments"! ������ ����饭�!';
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_R_COUNT is null then
      begin
        select count(c.id) into V_R_COUNT
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_DOORS_T c
        on t.id=c.HOLD_ID;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='On the value "Hold Name" are referenced in the "Doors" block! The operation is prohibited!';
              end;
            else
              begin
                V_STR_MSG:='�� ���祭�� "Hold Name" ������� ��뫪� � ����� "Doors"! ������ ����饭�!';
              end;
            end if;
          end;
        end if;

      end;
    end if;

    if (V_STR_MSG is null) then
      begin
        insert into WB_REF_WS_AIR_HLD_HLD_T_HST(ID_,
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
	                                                                      HOLD_NAME,
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
    select SEC_WB_REF_WS_AIR_HLD_HLD_T_HS.nextval,
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
	                                 i.HOLD_NAME,
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
         from WB_TEMP_XML_ID t join WB_REF_WS_AIR_HLD_HLD_T i
         on i.id=t.id;

         delete from WB_REF_WS_AIR_HLD_HLD_T
         where exists(select 1
                      from WB_TEMP_XML_ID t
                      where WB_REF_WS_AIR_HLD_HLD_T.id=t.id);

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_HLD_T_DEL;
/
