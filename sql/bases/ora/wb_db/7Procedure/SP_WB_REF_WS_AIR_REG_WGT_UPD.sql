create or replace procedure SP_WB_REF_WS_AIR_REG_WGT_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;

P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_ID_BORT_T number:=-1;
P_S_L_ADV_ID number:=-1;
P_WCC_ID number:=-1;

P_IS_ACARS number:=-1;
P_IS_DEFAULT number:=-1;
P_IS_APPROVED number:=-1;

P_DATE_FROM date;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);

P_DOW number:=null;
P_DOI number:=null;
P_ARM number:=null;
P_PROC_MAC number:=null;

P_DOW_INT_PART varchar2(50):='NULL';
P_DOW_DEC_PART varchar2(50):='NULL';

P_DOI_INT_PART varchar2(50):='NULL';
P_DOI_DEC_PART varchar2(50):='NULL';

P_ARM_INT_PART varchar2(50):='NULL';
P_ARM_DEC_PART varchar2(50):='NULL';

P_PROC_MAC_INT_PART varchar2(50):='NULL';
P_PROC_MAC_DEC_PART varchar2(50):='NULL';

P_REMARK varchar2(2000):='';
P_IS_REMARK_EMPTY number:=0;

P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT int:=0;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT_T[1]')) into P_ID_BORT_T from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_S_L_ADV_ID[1]')) into P_S_L_ADV_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_WCC_ID[1]')) into P_WCC_ID from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_ACARS[1]')) into P_IS_ACARS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_DEFAULT[1]')) into P_IS_DEFAULT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_APPROVED[1]')) into P_IS_APPROVED from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DOW_INT_PART[1]') into P_DOW_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DOW_DEC_PART[1]') into P_DOW_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DOI_INT_PART[1]') into P_DOI_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DOI_DEC_PART[1]') into P_DOI_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ARM_INT_PART[1]') into P_ARM_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ARM_DEC_PART[1]') into P_ARM_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_PROC_MAC_INT_PART[1]') into P_PROC_MAC_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_PROC_MAC_DEC_PART[1]') into P_PROC_MAC_DEC_PART from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_REMARK_EMPTY[1]')) into P_IS_REMARK_EMPTY from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_REMARK[1]') into P_REMARK from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');

    if P_DOW_INT_PART<>'NULL' then P_DOW:=to_number(P_DOW_INT_PART||'.'||P_DOW_DEC_PART); end if;
    if P_DOI_INT_PART<>'NULL' then P_DOI:=to_number(P_DOI_INT_PART||'.'||P_DOI_DEC_PART); end if;
    if P_ARM_INT_PART<>'NULL' then P_ARM:=to_number(P_ARM_INT_PART||'.'||P_ARM_DEC_PART); end if;
    if P_PROC_MAC_INT_PART<>'NULL' then P_PROC_MAC:=to_number(P_PROC_MAC_INT_PART||'.'||P_PROC_MAC_DEC_PART); end if;


    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_REG_WGT
    where ID=P_ID;

    if V_R_COUNT=0 then
      begin
        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then V_STR_MSG:='This record is deleted!'; end if;
            if P_LANG='RUS' then V_STR_MSG:='�� ������ 㤠����!'; end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_AIRCO_WS_BORTS
        where ID=P_ID_BORT_T;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The record for the selected "Bort" has been deleted!';
              end;
            else
              begin
                V_STR_MSG:='������ ��� ��࠭���� ���⮢��� ����� 㤠����!';
              end;
            end if;
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
   if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_SEAT_LAY_ADV
        where ID=P_S_L_ADV_ID;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The record for the selected "Seat Plan" has been deleted!';
              end;
            else
              begin
                V_STR_MSG:='������ ��� ��࠭���� "Seat Plan" 㤠����!';
              end;
            end if;
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_WCC
        where ID=P_WCC_ID;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The record for the selected "Weight Config Code" has been deleted!';
              end;
            else
              begin
                V_STR_MSG:='������ ��� ��࠭���� "Weight Config Code" 㤠����!';
              end;
           end if;
         end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_WS_AIR_REG_WGT
        where ID_BORT=P_ID_BORT_T and
              DATE_FROM=P_DATE_FROM and
              ID<>P_ID;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The record for the selected "Bort" with this value of "Effective Date" already exists!';
              end;
            else
              begin
                V_STR_MSG:='������ ��� ��࠭���� ���⮢��� ����� � ⠪�� ���祭��� "Effective Date" 㦥 �������!';
              end;
           end if;
         end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        if P_IS_REMARK_EMPTY=1 then
          begin

            P_REMARK:='EMPTY_STRING';
          end;
        end if;

        if P_IS_REMARK_EMPTY=0 then
          begin
            select count(id) into V_R_COUNT
            from WB_REF_RESERVED_PHRASE
            where PHRASE=P_REMARK;

            if V_R_COUNT>0 then
              begin
                if P_LANG='ENG' then
                  begin
                     V_STR_MSG:='Value '||P_REMARK||' is a phrase reserved!';
                  end;
                else
                  begin
                    V_STR_MSG:='���祭�� '||P_REMARK||' ���� ��१�ࢨ஢����� �ࠧ��!';
                  end;
                end if;
              end;
            end if;

          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if (V_STR_MSG is null) then
      begin
         UPDATE WB_REF_WS_AIR_REG_WGT
         set DATE_FROM=P_DATE_FROM,
	           S_L_ADV_ID=P_S_L_ADV_ID,
	           WCC_ID=P_WCC_ID,
	           IS_ACARS=P_IS_ACARS,
	           IS_DEFAULT=P_IS_DEFAULT,
	           IS_APPROVED=P_IS_APPROVED,
	           DOW=P_DOW,
	           DOI=P_DOI,
	           ARM=P_ARM,
	           PROC_MAC=P_PROC_MAC,
	           REMARK=P_REMARK,
             U_NAME=P_U_NAME,
		         U_IP=P_U_IP,
		         U_HOST_NAME=P_U_HOST_NAME,
		         DATE_WRITE=sysdate()
          where ID=P_ID;

        if P_IS_DEFAULT=1 then
          begin
            UPDATE WB_REF_WS_AIR_REG_WGT
            set IS_DEFAULT=0,
                U_NAME=P_U_NAME,
		            U_IP=P_U_IP,
		            U_HOST_NAME=P_U_HOST_NAME,
		            DATE_WRITE=sysdate()
            where ID_BORT=P_ID_BORT_T and
                  ID<>P_ID AND
                  IS_DEFAULT=1;
          end;
        end if;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_REG_WGT_UPD;
/
