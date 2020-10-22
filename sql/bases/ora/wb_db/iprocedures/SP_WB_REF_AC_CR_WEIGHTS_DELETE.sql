create or replace procedure SP_WB_REF_AC_CR_WEIGHTS_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
  begin
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

    insert into WB_REF_AIRCO_CREW_WEIGHTS_HIST (ID_,
	                                                U_NAME_,
	                                                  U_IP_,
	                                                    U_HOST_NAME_,
	                                                      DATE_WRITE_,
	                                                        ACTION_,
	                                                          ID,
	                                                            ID_AC_OLD,
                                                                DESCRIPTION_OLD,
	                                                                DATE_FROM_OLD,
	                                                                  FC_STANDART_OLD,
	                                                                    FC_MALE_OLD,
                                                                     	  FC_FEMALE_OLD,
                                                                	     	  FC_HAND_BAG_OLD,
                                                                	     	    FC_HAND_BAG_INCLUDE_OLD,
                                                                	   	        CC_STANDART_OLD,
                                                                	   	          CC_MALE_OLD,
                                                                   	  	          CC_FEMALE_OLD,
	                                                                 	                CC_HAND_BAG_OLD,
	                                                                 	                  CC_HAND_BAG_INCLUDE_OLD,
	                                                                	                    FC_BAGGAGE_WEIGHT_OLD,
	                                                                	                      CC_BAGGAGE_WEIGHT_OLD,
	                                                                	                        BY_DEFAULT_OLD,
	                                                                	                          U_NAME_OLD,
	                                                               	                              U_IP_OLD,
	                                                               	                                U_HOST_NAME_OLD,
	                                                              	                                  DATE_WRITE_OLD,
	                                                              	                                    ID_AC_NEW,
	                                                              	                                      DESCRIPTION_NEW,
	                                                              	                                        DATE_FROM_NEW,
	                                                              	                                          FC_STANDART_NEW,
	                                                              	                                            FC_MALE_NEW,
	                                                              	                                              FC_FEMALE_NEW,
	                                                              	                                                FC_HAND_BAG_NEW,
	                                                              	                                                  FC_HAND_BAG_INCLUDE_NEW,
	                                                              	                                                    CC_STANDART_NEW,
	                                                               	                                                      CC_MALE_NEW,
	                                                                	                                                      CC_FEMALE_NEW,
	                                                               	                                                          CC_HAND_BAG_NEW,
	                                                               	                                                            CC_HAND_BAG_INCLUDE_NEW,
	                                                                	                                                            FC_BAGGAGE_WEIGHT_NEW,
	                                                                	                                                              CC_BAGGAGE_WEIGHT_NEW,
	                                                                	                                                                BY_DEFAULT_NEW,
	                                                               	                                                                    U_NAME_NEW,
	                                                               	                                                                      U_IP_NEW,
	                                                                	                                                                      U_HOST_NAME_NEW,
	                                                                	                                                                        DATE_WRITE_NEW)
    select SEC_WB_REF_AC_CR_WEIGHTS_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
                           i.DESCRIPTION,
	                           i.DATE_FROM,
	                             i.FC_STANDART,
                             	   i.FC_MALE,
	                                 i.FC_FEMALE,
	                                   i.FC_HAND_BAG,
	                                     i.FC_HAND_BAG_INCLUDE,
	                                       i.CC_STANDART,
	                                         i.CC_MALE,
	                                           i.CC_FEMALE,
	                                             i.CC_HAND_BAG,
	                                               i.CC_HAND_BAG_INCLUDE,
	                                                 i.FC_BAGGAGE_WEIGHT,
	                                                   i.CC_BAGGAGE_WEIGHT,
	                                                     i.BY_DEFAULT,
	                                                       i.U_NAME,
                                             	             i.U_IP,
                                             	               i.U_HOST_NAME,
	                                                             i.DATE_WRITE,
                                                                 i.ID_AC,
                                                                   i.DESCRIPTION,
	                                                                   i.DATE_FROM,
	                                                                     i.FC_STANDART,
	                                                                       i.FC_MALE,
	                                                                         i.FC_FEMALE,
	                                                                           i.FC_HAND_BAG,
	                                                                             i.FC_HAND_BAG_INCLUDE,
	                                                                               i.CC_STANDART,
	                                                                                 i.CC_MALE,
	                                                                                   i.CC_FEMALE,
	                                                                                     i.CC_HAND_BAG,
	                                                                                       i.CC_HAND_BAG_INCLUDE,
	                                                                                         i.FC_BAGGAGE_WEIGHT,
	                                                                                           i.CC_BAGGAGE_WEIGHT,
	                                                                                             i.BY_DEFAULT,
	                                                                                               i.U_NAME,
	                                                                                                 i.U_IP,
                                             	                                                       i.U_HOST_NAME,
	                                                                                                     i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_CREW_WEIGHTS i
    on i.id=t.id;

    delete from WB_REF_AIRCO_CREW_WEIGHTS
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_CREW_WEIGHTS.id);


      str_msg:='EMPTY_STRING';

      cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';
    commit;
  end SP_WB_REF_AC_CR_WEIGHTS_DELETE;
/
