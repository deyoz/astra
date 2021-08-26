CREATE TABLE ET_ADDR_SET (
    AIRIMP_ADDR VARCHAR(8),
    AIRIMP_OWN_ADDR VARCHAR(8),
    AIRLINE VARCHAR(3) NOT NULL,
    EDIFACT_PROFILE VARCHAR(30),
    EDI_ADDR VARCHAR(20) NOT NULL,
    EDI_ADDR_EXT VARCHAR(20),
    EDI_OWN_ADDR VARCHAR(20) NOT NULL,
    EDI_OWN_ADDR_EXT VARCHAR(20),
    FLT_NO INTEGER,
    ID INTEGER NOT NULL
);

