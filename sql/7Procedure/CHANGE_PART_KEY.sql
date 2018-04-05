create or replace PROCEDURE change_part_key(vpart_key arx_points.part_key%TYPE,
                                            vmove_id  arx_points.move_id%TYPE)
AS
CURSOR curPoints IS
  SELECT point_id FROM arx_points
  WHERE part_key=vpart_key AND move_id=vmove_id;
CURSOR curGrp(vpoint_id arx_points.point_id%TYPE) IS
  SELECT grp_id FROM arx_pax_grp
  WHERE part_key=vpart_key AND point_dep=vpoint_id;
CURSOR curPax(vgrp_id arx_pax_grp.grp_id%TYPE) IS
  SELECT pax_id FROM arx_pax
  WHERE part_key=vpart_key AND grp_id=vgrp_id;
new_part_key arx_points.part_key%TYPE;
BEGIN
  SELECT
       GREATEST(NVL(MAX(scd_out),TO_DATE('01.01.0001','DD.MM.YYYY')),
                NVL(MAX(scd_in), TO_DATE('01.01.0001','DD.MM.YYYY')),
                NVL(MAX(est_out),TO_DATE('01.01.0001','DD.MM.YYYY')),
                NVL(MAX(est_in), TO_DATE('01.01.0001','DD.MM.YYYY')),
                NVL(MAX(act_out),TO_DATE('01.01.0001','DD.MM.YYYY')),
                NVL(MAX(act_in), TO_DATE('01.01.0001','DD.MM.YYYY')))
  INTO new_part_key
  FROM arx_points
  WHERE part_key=vpart_key AND move_id=vmove_id;
  IF new_part_key=TO_DATE('01.01.0001','DD.MM.YYYY') THEN
    DBMS_OUTPUT.PUT_LINE('TROUBLE!!!');
    RETURN;
  END IF;
  FOR curRow IN curPoints LOOP
    FOR curGrpRow IN curGrp(curRow.point_id) LOOP
      FOR curPaxRow IN curPax(curGrpRow.grp_id) LOOP
        UPDATE arx_pax_norms SET part_key=new_part_key WHERE part_key=vpart_key AND pax_id=curPaxRow.pax_id;
        UPDATE arx_pax_rem SET part_key=new_part_key WHERE part_key=vpart_key AND pax_id=curPaxRow.pax_id;
        UPDATE arx_transfer_subcls SET part_key=new_part_key WHERE part_key=vpart_key AND pax_id=curPaxRow.pax_id;
      END LOOP;
      UPDATE arx_bag2 SET part_key=new_part_key WHERE part_key=vpart_key AND grp_id=curGrpRow.grp_id;
      UPDATE arx_bag_prepay SET part_key=new_part_key WHERE part_key=vpart_key AND grp_id=curGrpRow.grp_id;
      UPDATE arx_bag_tags SET part_key=new_part_key WHERE part_key=vpart_key AND grp_id=curGrpRow.grp_id;
      UPDATE arx_paid_bag SET part_key=new_part_key WHERE part_key=vpart_key AND grp_id=curGrpRow.grp_id;
      UPDATE arx_pax SET part_key=new_part_key WHERE part_key=vpart_key AND grp_id=curGrpRow.grp_id;
      UPDATE arx_transfer SET part_key=new_part_key WHERE part_key=vpart_key AND grp_id=curGrpRow.grp_id;
      UPDATE arx_tckin_segments SET part_key=new_part_key WHERE part_key=vpart_key AND grp_id=curGrpRow.grp_id;
      UPDATE arx_market_flt SET part_key=new_part_key WHERE part_key=vpart_key AND grp_id=curGrpRow.grp_id;
      UPDATE arx_value_bag SET part_key=new_part_key WHERE part_key=vpart_key AND grp_id=curGrpRow.grp_id;
      UPDATE arx_grp_norms SET part_key=new_part_key WHERE part_key=vpart_key AND grp_id=curGrpRow.grp_id;
    END LOOP;
    UPDATE arx_etickets SET part_key=new_part_key WHERE part_key=vpart_key AND point_id=curRow.point_id;
    UPDATE arx_events SET part_key=new_part_key
    WHERE part_key=vpart_key AND
            type IN (system.evtFlt,
                     system.evtGraph,
                     system.evtPax,
                     system.evtPay,
                     system.evtTlg) AND id1=curRow.point_id;
    UPDATE arx_pax_grp SET part_key=new_part_key WHERE part_key=vpart_key AND point_dep=curRow.point_id;
    UPDATE arx_stat SET part_key=new_part_key WHERE part_key=vpart_key AND point_id=curRow.point_id;
    UPDATE arx_trfer_stat SET part_key=new_part_key WHERE part_key=vpart_key AND point_id=curRow.point_id;
    UPDATE arx_tlg_out SET part_key=new_part_key WHERE part_key=vpart_key AND point_id=curRow.point_id;
    UPDATE arx_trip_classes SET part_key=new_part_key WHERE part_key=vpart_key AND point_id=curRow.point_id;
    UPDATE arx_trip_delays SET part_key=new_part_key WHERE part_key=vpart_key AND point_id=curRow.point_id;
    UPDATE arx_trip_load SET part_key=new_part_key WHERE part_key=vpart_key AND point_dep=curRow.point_id;
    UPDATE arx_trip_sets SET part_key=new_part_key WHERE part_key=vpart_key AND point_id=curRow.point_id;
    UPDATE arx_trip_stages SET part_key=new_part_key WHERE part_key=vpart_key AND point_id=curRow.point_id;
    UPDATE arx_crs_displace2 SET part_key=new_part_key WHERE part_key=vpart_key AND point_id_spp=curRow.point_id;
    UPDATE arx_bag_pay_types SET part_key=new_part_key WHERE part_key=vpart_key AND receipt_id IN
      (SELECT receipt_id FROM arx_bag_receipts WHERE part_key=vpart_key AND point_id=curRow.point_id);
    UPDATE arx_bag_receipts SET part_key=new_part_key WHERE part_key=vpart_key AND point_id=curRow.point_id;
  END LOOP;
  UPDATE arx_events SET part_key=new_part_key WHERE part_key=vpart_key AND type IN (system.evtDisp) AND id1=vmove_id;
  UPDATE arx_move_ref SET part_key=new_part_key WHERE part_key=vpart_key AND move_id=vmove_id;
  UPDATE arx_points SET part_key=new_part_key WHERE part_key=vpart_key AND move_id=vmove_id;
END;


/
