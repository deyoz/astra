create or replace PACKAGE BODY sopp
AS

FUNCTION get_birks(vpoint_id 	IN points.point_id%TYPE,
                   vlang	IN lang_types.code%TYPE) RETURN VARCHAR2
IS
res	VARCHAR2(1000);
cur ckin.birks_cursor_ref;
BEGIN
  OPEN cur FOR
    /* Только бирки тех багажных пулов, все пассажиры которых не прошли посадку */
    SELECT tag_type,no_len,
           tag_colors.code AS color,
           DECODE(vlang,'RU',tag_colors.code,NVL(tag_colors.code_lat,tag_colors.code)) AS color_view,
           TRUNC(no/1000) AS first,
           MOD(no,1000) AS last,
           no
    FROM pax_grp,bag2,bag_tags,tag_types,tag_colors
    WHERE pax_grp.grp_id=bag2.grp_id AND
          bag2.grp_id=bag_tags.grp_id AND
          bag2.num=bag_tags.bag_num AND
          bag_tags.tag_type=tag_types.code AND
          bag_tags.color=tag_colors.code(+) AND
          pax_grp.point_dep=vpoint_id AND pax_grp.status NOT IN ('E') AND
          ckin.bag_pool_refused(bag2.grp_id, bag2.bag_pool_num, pax_grp.class, pax_grp.bag_refuse)=0 AND
          ckin.bag_pool_boarded(bag2.grp_id, bag2.bag_pool_num, pax_grp.class, pax_grp.bag_refuse)=0
    UNION
    SELECT tag_type,no_len,
           tag_colors.code AS color,
           DECODE(vlang,'RU',tag_colors.code,NVL(tag_colors.code_lat,tag_colors.code)) AS color_view,
           TRUNC(no/1000) AS first,
           MOD(no,1000) AS last,
           no
    FROM pax_grp,bag_tags,tag_types,tag_colors
    WHERE pax_grp.grp_id=bag_tags.grp_id AND
          bag_tags.bag_num IS NULL AND
          bag_tags.tag_type=tag_types.code AND
          bag_tags.color=tag_colors.code(+) AND
          pax_grp.point_dep=vpoint_id AND pax_grp.status NOT IN ('E') AND
          ckin.bag_pool_refused(bag_tags.grp_id, 1, pax_grp.class, pax_grp.bag_refuse)=0 AND
          ckin.bag_pool_boarded(bag_tags.grp_id, 1, pax_grp.class, pax_grp.bag_refuse)=0
    ORDER BY tag_type,color,no;
  res:=ckin.build_birks_str(cur);
  CLOSE cur;
  RETURN res;
END get_birks;

END sopp;
/
