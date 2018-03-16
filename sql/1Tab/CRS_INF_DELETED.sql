create table CRS_INF_DELETED 
(
       inf_id               NUMBER(9) NOT NULL,
       pax_id               NUMBER(9) NOT NULL
);

alter table CRS_INF_DELETED
add constraint CRS_INF_DELETED__PK primary key (INF_ID);
