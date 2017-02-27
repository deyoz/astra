create or replace PROCEDURE DowngradeDB(vgrp_id pax_grp.grp_id%TYPE)
IS
  CURSOR cur IS
    SELECT grp_id
    FROM tckin_pax_grp
    WHERE tckin_id IN (SELECT tckin_id FROM tckin_pax_grp WHERE grp_id=vgrp_id)
    UNION
    SELECT vgrp_id FROM dual;

BEGIN
  FOR curRow IN cur LOOP
    DELETE FROM grp_service_lists WHERE grp_id=curRow.grp_id;
    DELETE FROM pax_service_lists WHERE pax_id IN (SELECT pax_id FROM pax WHERE grp_id=curRow.grp_id);
    DELETE FROM pax_norms_text WHERE pax_id IN (SELECT pax_id FROM pax WHERE grp_id=curRow.grp_id);
    DELETE FROM paid_rfisc WHERE pax_id IN (SELECT pax_id FROM pax WHERE grp_id=curRow.grp_id);
    DELETE FROM pax_services WHERE pax_id IN (SELECT pax_id FROM pax WHERE grp_id=curRow.grp_id);
    DELETE FROM service_payment WHERE grp_id=curRow.grp_id;
    UPDATE bag2 SET list_id=NULL, bag_type_str=NULL, service_type=NULL, airline=NULL WHERE grp_id=curRow.grp_id;
    UPDATE paid_bag SET list_id=NULL, bag_type_str=NULL, airline=NULL WHERE grp_id=curRow.grp_id;
    UPDATE pax_grp SET excess_wt=NULL, excess_pc=NULL WHERE grp_id=curRow.grp_id;
    COMMIT;
  END LOOP;
END DowngradeDB;
/
