create or replace trigger TRG_WB_REF_AC_CR_WEIGHTS_HIST
AFTER
INSERT OR UPDATE
ON WB_REF_AIRCO_CREW_WEIGHTS
FOR EACH ROW
BEGIN
  if inserting then
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
             :new.U_NAME,
	             :new.U_IP,
	               :new.U_HOST_NAME,
                   SYSDATE,
                     'insert',
                       :new.id,
                         :new.ID_AC,
                           :new.DESCRIPTION,
	                           :new.DATE_FROM,
	                             :new.FC_STANDART,
                             	   :new.FC_MALE,
	                                 :new.FC_FEMALE,
	                                   :new.FC_HAND_BAG,
	                                     :new.FC_HAND_BAG_INCLUDE,
	                                       :new.CC_STANDART,
	                                         :new.CC_MALE,
	                                           :new.CC_FEMALE,
	                                             :new.CC_HAND_BAG,
	                                               :new.CC_HAND_BAG_INCLUDE,
	                                                 :new.FC_BAGGAGE_WEIGHT,
	                                                   :new.CC_BAGGAGE_WEIGHT,
	                                                     :new.BY_DEFAULT,
	                                                       :new.U_NAME,
                                             	             :new.U_IP,
                                             	               :new.U_HOST_NAME,
	                                                             :new.DATE_WRITE,
                                                                 :new.ID_AC,
                                                                   :new.DESCRIPTION,
	                                                                   :new.DATE_FROM,
	                                                                     :new.FC_STANDART,
	                                                                       :new.FC_MALE,
	                                                                         :new.FC_FEMALE,
	                                                                           :new.FC_HAND_BAG,
	                                                                             :new.FC_HAND_BAG_INCLUDE,
	                                                                               :new.CC_STANDART,
	                                                                                 :new.CC_MALE,
	                                                                                   :new.CC_FEMALE,
	                                                                                     :new.CC_HAND_BAG,
	                                                                                       :new.CC_HAND_BAG_INCLUDE,
	                                                                                         :new.FC_BAGGAGE_WEIGHT,
	                                                                                           :new.CC_BAGGAGE_WEIGHT,
	                                                                                             :new.BY_DEFAULT,
	                                                                                               :new.U_NAME,
	                                                                                                 :new.U_IP,
                                             	                                                       :new.U_HOST_NAME,
	                                                                                                     :new.DATE_WRITE
    from dual;
  end if;

  if updating then
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
             :new.U_NAME,
	             :new.U_IP,
	               :new.U_HOST_NAME,
                   SYSDATE,
                     'insert',
                       :old.id,
                         :old.ID_AC,
                           :old.DESCRIPTION,
	                           :old.DATE_FROM,
	                             :old.FC_STANDART,
                             	   :old.FC_MALE,
	                                 :old.FC_FEMALE,
	                                   :old.FC_HAND_BAG,
	                                     :old.FC_HAND_BAG_INCLUDE,
	                                       :old.CC_STANDART,
	                                         :old.CC_MALE,
	                                           :old.CC_FEMALE,
	                                             :old.CC_HAND_BAG,
	                                               :old.CC_HAND_BAG_INCLUDE,
	                                                 :old.FC_BAGGAGE_WEIGHT,
	                                                   :old.CC_BAGGAGE_WEIGHT,
	                                                     :old.BY_DEFAULT,
	                                                       :old.U_NAME,
                                             	             :old.U_IP,
                                             	               :old.U_HOST_NAME,
	                                                             :old.DATE_WRITE,
                                                                 :new.ID_AC,
                                                                   :new.DESCRIPTION,
	                                                                   :new.DATE_FROM,
	                                                                     :new.FC_STANDART,
	                                                                       :new.FC_MALE,
	                                                                         :new.FC_FEMALE,
	                                                                           :new.FC_HAND_BAG,
	                                                                             :new.FC_HAND_BAG_INCLUDE,
	                                                                               :new.CC_STANDART,
	                                                                                 :new.CC_MALE,
	                                                                                   :new.CC_FEMALE,
	                                                                                     :new.CC_HAND_BAG,
	                                                                                       :new.CC_HAND_BAG_INCLUDE,
	                                                                                         :new.FC_BAGGAGE_WEIGHT,
	                                                                                           :new.CC_BAGGAGE_WEIGHT,
	                                                                                             :new.BY_DEFAULT,
	                                                                                               :new.U_NAME,
	                                                                                                 :new.U_IP,
                                             	                                                       :new.U_HOST_NAME,
	                                                                                                     :new.DATE_WRITE
    from dual;
  end if;
END;
/
