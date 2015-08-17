create or replace PROCEDURE SearchHist(UserIdValue users2.user_id%TYPE)
  BEGIN
  SET @resultQuery = NULL;
  SELECT
    GROUP_CONCAT(
      DISTINCT
      CONCAT('select login, type, pr_denial, hist_users2.hist_time, time, disp_airp, disp_airline, disp_craft, disp_suffix, hist_user_sets.hist_time, history_events.hist_order, open_time, close_time
      from hist_users2, hist_user_sets, history_events where
      hist_users2.hist_order = ', hist_users2.hist_order, 'AND
      hist_users2.user_id = hist_user_sets.user_id AND
      hist_user_sets.hist_order = history_events.hist_order AND
      (open_time < ', close_time, ' OR open_time is NULL) AND
      (close_time > ', open_time, ' OR close_time is NULL)
      order by hist_users2.hist_order');
      SEPARATOR '\r\nUNION\r\n'
    )
  INTO
    @resultQuery
  FROM
    hist_users2, history_events
  WHERE
    user_id = UserIdValue AND hist_users2.hist_order = history_events.hist_order AND table_id = 26 order by hist_users2.hist_order;

  SELECT @resultQuery;
  PREPARE stmt FROM @resultQuery ;
  EXECUTE stmt ;

  DEALLOCATE PREPARE stmt;
END
/
