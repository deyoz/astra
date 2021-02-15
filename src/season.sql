 select routes.move_id as move_id, delta_in as delta, sched_days.pr_del as pr_del, first_day, last_day, region
 FROM sched_days, routes
 WHERE routes.move_id = sched_days.move_id
 AND DATE_trunc('day', first_day) + interval '1 day' * (delta_in + delta) <= now()
 AND date_trunc('day', last_day) + interval '1 day' * (delta_in + delta) >= now()
 AND POSITION(TO_CHAR(now() - interval '1 day' * (delta_in - delta), 'ID') in days) != 0;
