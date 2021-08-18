create or replace package LIBRA
as

procedure WRITE_AHM_LOG_MSG(AIRLINE_IN in varchar2,
                            CATEGORY_IN in varchar2,
                            BORT_NUM_IN in varchar2,
                            USER_DESCR_IN in varchar2,
                            USER_DESK_IN in varchar2,
                            MSG_IN in varchar2);

procedure WRITE_AHM_LOG_MSG2(AIRLINE_IN in varchar2,
                             CATEGORY_IN in varchar2,
                             BORT_NUM_IN in varchar2,
                             USER_DESCR_IN in varchar2,
                             USER_DESK_IN in varchar2,
                             MSG_RU_IN in varchar2,
                             MSG_EN_IN in varchar2);

procedure WRITE_BALANCE_LOG_MSG(POINT_ID_IN in number,
                                USER_DESCR_IN in varchar2,
                                USER_DESK_IN in varchar2,
                                MSG_IN in varchar2);

procedure WRITE_BALANCE_LOG_MSG2(POINT_ID_IN in number,
                                 USER_DESCR_IN in varchar2,
                                 USER_DESK_IN in varchar2,
                                 MSG_RU_IN in varchar2,
                                 MSG_EN_IN in varchar2);


end LIBRA;
/
