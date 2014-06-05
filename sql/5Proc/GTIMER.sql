create or replace PACKAGE gtimer
AS
PROCEDURE puttrip_stages( vpoint_id IN points.point_id%TYPE );
FUNCTION get_stage_time( vpoint_id IN trip_stages.point_id%TYPE,
                         vstage_id IN trip_stages.stage_id%TYPE ) RETURN DATE;
FUNCTION IsClientStage( vpoint_id IN points.point_id%TYPE,
                        vstage_id IN trip_stages.stage_id%TYPE,
                        vpr_permit OUT trip_ckin_client.pr_permit%TYPE ) RETURN NUMBER;
FUNCTION ExecStage( vpoint_id IN points.point_id%TYPE,
                    vstage_id IN trip_stages.stage_id%TYPE,
                    vact OUT trip_stages.act%TYPE ) RETURN NUMBER;
PROCEDURE sync_trip_final_stages( vpoint_id IN trip_stages.point_id%TYPE );
END gtimer;
/
