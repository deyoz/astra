CREATE TABLE ROUTES (
    AIRLINE VARCHAR(3),
    AIRLINE_FMT SMALLINT,
    AIRP VARCHAR(3),
    AIRP_FMT SMALLINT,
    C SMALLINT,
    CRAFT VARCHAR(3),
    CRAFT_FMT SMALLINT,
    DELTA_IN SMALLINT,
    DELTA_OUT SMALLINT,
    F SMALLINT,
    FLT_NO INTEGER,
    LITERA VARCHAR(3),
    MOVE_ID INTEGER NOT NULL,
    NUM SMALLINT NOT NULL,
    PR_DEL SMALLINT DEFAULT 0 NOT NULL,
    RBD_ORDER VARCHAR(64),
    SCD_IN TIMESTAMP(0),
    SCD_OUT TIMESTAMP(0),
    SUFFIX VARCHAR(1),
    SUFFIX_FMT SMALLINT,
    TRIP_TYPE VARCHAR(1),
    UNITRIP VARCHAR(255),
    Y SMALLINT
);
