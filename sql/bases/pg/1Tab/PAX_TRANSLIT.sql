CREATE TABLE PAX_TRANSLIT (
    GRP_ID INTEGER NOT NULL,
    NAME VARCHAR(64),
    PAX_ID INTEGER NOT NULL,
    POINT_ID INTEGER NOT NULL,
    SURNAME VARCHAR(64) NOT NULL,
    TRANSLIT_FORMAT SMALLINT NOT NULL
);