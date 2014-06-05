create or replace PACKAGE utils
AS

PROCEDURE sync_countries;
PROCEDURE sync_cities(pr_summer IN NUMBER);
PROCEDURE sync_airps;
PROCEDURE sync_airlines;
PROCEDURE sync_crafts;
PROCEDURE sync_sirena_codes(pr_summer IN NUMBER);
PROCEDURE fill_tz_regions;

END utils;
/
