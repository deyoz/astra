create or replace PACKAGE kassa
AS

PROCEDURE check_period(pr_new           BOOLEAN,
                       vfirst_date      DATE,
                       vlast_date       DATE,
                       vnow             DATE,
                       first        OUT DATE,
                       last         OUT DATE,
                       pr_opd       OUT BOOLEAN);

PROCEDURE modify_bag_norm(
       vid              bag_norms.id%TYPE,
       vlast_date       bag_norms.last_date%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             bag_norms.tid%TYPE DEFAULT NULL);

PROCEDURE delete_bag_norm(
       vid              bag_norms.id%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             bag_norms.tid%TYPE DEFAULT NULL);

PROCEDURE copy_basic_bag_norm(vairline         bag_norms.airline%TYPE,
                              vsetting_user    history_events.open_user%TYPE,
                              vstation         history_events.open_desk%TYPE);

PROCEDURE add_bag_norm(
       vid       IN OUT bag_norms.id%TYPE,
       vairline         bag_norms.airline%TYPE,
       vpr_trfer        bag_norms.pr_trfer%TYPE,
       vcity_dep        bag_norms.city_dep%TYPE,
       vcity_arv        bag_norms.city_arv%TYPE,
       vpax_cat         bag_norms.pax_cat%TYPE,
       vsubclass        bag_norms.subclass%TYPE,
       vclass           bag_norms.class%TYPE,
       vflt_no          bag_norms.flt_no%TYPE,
       vcraft           bag_norms.craft%TYPE,
       vtrip_type       bag_norms.trip_type%TYPE,
       vfirst_date      bag_norms.first_date%TYPE,
       vlast_date       bag_norms.last_date%TYPE,
       vbag_type        bag_norms.bag_type%TYPE,
       vamount          bag_norms.amount%TYPE,
       vweight          bag_norms.weight%TYPE,
       vper_unit        bag_norms.per_unit%TYPE,
       vnorm_type       bag_norms.norm_type%TYPE,
       vextra           bag_norms.extra%TYPE,
       vtid             bag_norms.tid%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE);

PROCEDURE modify_bag_rate(
       vid              bag_rates.id%TYPE,
       vlast_date       bag_rates.last_date%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             bag_rates.tid%TYPE DEFAULT NULL);
PROCEDURE delete_bag_rate(
       vid              bag_rates.id%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             bag_rates.tid%TYPE DEFAULT NULL);

PROCEDURE copy_basic_bag_rate(vairline         bag_rates.airline%TYPE,
                              vsetting_user    history_events.open_user%TYPE,
                              vstation         history_events.open_desk%TYPE);

PROCEDURE add_bag_rate(
       vid       IN OUT bag_rates.id%TYPE,
       vairline         bag_rates.airline%TYPE,
       vpr_trfer        bag_rates.pr_trfer%TYPE,
       vcity_dep        bag_rates.city_dep%TYPE,
       vcity_arv        bag_rates.city_arv%TYPE,
       vpax_cat         bag_rates.pax_cat%TYPE,
       vsubclass        bag_rates.subclass%TYPE,
       vclass           bag_rates.class%TYPE,
       vflt_no          bag_rates.flt_no%TYPE,
       vcraft           bag_rates.craft%TYPE,
       vtrip_type       bag_rates.trip_type%TYPE,
       vfirst_date      bag_rates.first_date%TYPE,
       vlast_date       bag_rates.last_date%TYPE,
       vbag_type        bag_rates.bag_type%TYPE,
       vrate            bag_rates.rate%TYPE,
       vrate_cur        bag_rates.rate_cur%TYPE,
       vmin_weight      bag_rates.min_weight%TYPE,
       vextra           bag_rates.extra%TYPE,
       vtid             bag_rates.tid%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE);

PROCEDURE modify_value_bag_tax(
       vid              value_bag_taxes.id%TYPE,
       vlast_date       value_bag_taxes.last_date%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             value_bag_taxes.tid%TYPE DEFAULT NULL);

PROCEDURE delete_value_bag_tax(
       vid              value_bag_taxes.id%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             value_bag_taxes.tid%TYPE DEFAULT NULL);

PROCEDURE copy_basic_value_bag_tax(vairline         value_bag_taxes.airline%TYPE,
                                   vsetting_user    history_events.open_user%TYPE,
                                   vstation         history_events.open_desk%TYPE);

PROCEDURE add_value_bag_tax(
       vid       IN OUT value_bag_taxes.id%TYPE,
       vairline         value_bag_taxes.airline%TYPE,
       vpr_trfer        value_bag_taxes.pr_trfer%TYPE,
       vcity_dep        value_bag_taxes.city_dep%TYPE,
       vcity_arv        value_bag_taxes.city_arv%TYPE,
       vfirst_date      value_bag_taxes.first_date%TYPE,
       vlast_date       value_bag_taxes.last_date%TYPE,
       vtax             value_bag_taxes.tax%TYPE,
       vmin_value       value_bag_taxes.min_value%TYPE,
       vmin_value_cur   value_bag_taxes.min_value_cur%TYPE,
       vextra           value_bag_taxes.extra%TYPE,
       vtid             value_bag_taxes.tid%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE);

PROCEDURE modify_exchange_rate(
       vid              exchange_rates.id%TYPE,
       vlast_date       exchange_rates.last_date%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             exchange_rates.tid%TYPE DEFAULT NULL);

PROCEDURE delete_exchange_rate(
       vid              exchange_rates.id%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE,
       vtid             exchange_rates.tid%TYPE DEFAULT NULL);

PROCEDURE copy_basic_exchange_rate(vairline         exchange_rates.airline%TYPE,
                                   vsetting_user    history_events.open_user%TYPE,
                                   vstation         history_events.open_desk%TYPE);

PROCEDURE add_exchange_rate(
       vid       IN OUT exchange_rates.id%TYPE,
       vairline         exchange_rates.airline%TYPE,
       vrate1           exchange_rates.rate1%TYPE,
       vcur1            exchange_rates.cur1%TYPE,
       vrate2           exchange_rates.rate2%TYPE,
       vcur2            exchange_rates.cur2%TYPE,
       vfirst_date      exchange_rates.first_date%TYPE,
       vlast_date       exchange_rates.last_date%TYPE,
       vextra           exchange_rates.extra%TYPE,
       vtid             exchange_rates.tid%TYPE,
       vsetting_user    history_events.open_user%TYPE,
       vstation         history_events.open_desk%TYPE);

END kassa;
/
