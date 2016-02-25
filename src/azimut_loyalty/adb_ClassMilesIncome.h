

        #ifndef ADB_CLASSMILESINCOME_H
        #define ADB_CLASSMILESINCOME_H

       /**
        * adb_ClassMilesIncome.h
        *
        * This file was auto-generated from WSDL
        * by the Apache Axis2/Java version: 1.7.0  Built on : Jan 18, 2016 (09:42:13 GMT)
        */

       /**
        *  adb_ClassMilesIncome class
        */

        

        #include <stdio.h>
        #include <axiom.h>
        #include <axis2_util.h>
        #include <axiom_soap.h>
        #include <axis2_client.h>

        #include "axis2_extension_mapper.h"

        #ifdef __cplusplus
        extern "C"
        {
        #endif

        #define ADB_DEFAULT_DIGIT_LIMIT 1024
        #define ADB_DEFAULT_NAMESPACE_PREFIX_LIMIT 64
        

        typedef struct adb_ClassMilesIncome adb_ClassMilesIncome_t;

        
        

        /******************************* Create and Free functions *********************************/

        /**
         * Constructor for creating adb_ClassMilesIncome_t
         * @param env pointer to environment struct
         * @return newly created adb_ClassMilesIncome_t object
         */
        adb_ClassMilesIncome_t* AXIS2_CALL
        adb_ClassMilesIncome_create(
            const axutil_env_t *env );

        /**
         * Wrapper for the "free" function, will invoke the extension mapper instead
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_free (
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        /**
         * Free adb_ClassMilesIncome_t object
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_free_obj (
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);



        /********************************** Getters and Setters **************************************/
        
        

        /**
         * Getter for arrival. 
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesIncome_get_arrival(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        /**
         * Setter for arrival.
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param arg_arrival axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_arrival(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            const axis2_char_t*  arg_arrival);

        /**
         * Resetter for arrival
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_reset_arrival(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        
        

        /**
         * Getter for base. 
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesIncome_get_base(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        /**
         * Setter for base.
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param arg_base double
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_base(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            const double  arg_base);

        /**
         * Resetter for base
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_reset_base(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        
        

        /**
         * Getter for className. 
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesIncome_get_className(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        /**
         * Setter for className.
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param arg_className axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_className(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            const axis2_char_t*  arg_className);

        /**
         * Resetter for className
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_reset_className(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        
        

        /**
         * Getter for count. 
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_ClassMilesIncome_get_count(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        /**
         * Setter for count.
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param arg_count int
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_count(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            const int  arg_count);

        /**
         * Resetter for count
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_reset_count(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        
        

        /**
         * Getter for departure. 
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesIncome_get_departure(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        /**
         * Setter for departure.
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param arg_departure axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_departure(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            const axis2_char_t*  arg_departure);

        /**
         * Resetter for departure
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_reset_departure(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        
        

        /**
         * Getter for factor. 
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesIncome_get_factor(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        /**
         * Setter for factor.
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param arg_factor double
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_factor(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            const double  arg_factor);

        /**
         * Resetter for factor
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_reset_factor(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        
        

        /**
         * Getter for miles. 
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesIncome_get_miles(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        /**
         * Setter for miles.
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param arg_miles double
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_miles(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            const double  arg_miles);

        /**
         * Resetter for miles
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_reset_miles(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        
        

        /**
         * Getter for statCoef. 
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesIncome_get_statCoef(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        /**
         * Setter for statCoef.
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param arg_statCoef double
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_statCoef(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            const double  arg_statCoef);

        /**
         * Resetter for statCoef
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_reset_statCoef(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        
        

        /**
         * Getter for typeCoef. 
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesIncome_get_typeCoef(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        /**
         * Setter for typeCoef.
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param arg_typeCoef double
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_typeCoef(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            const double  arg_typeCoef);

        /**
         * Resetter for typeCoef
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_reset_typeCoef(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

        


        /******************************* Checking and Setting NIL values *********************************/
        

        /**
         * NOTE: set_nil is only available for nillable properties
         */

        

        /**
         * Check whether arrival is nill
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesIncome_is_arrival_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);


        
        /**
         * Set arrival to nill (currently the same as reset)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_arrival_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);
        

        /**
         * Check whether base is nill
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesIncome_is_base_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);


        
        /**
         * Set base to nill (currently the same as reset)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_base_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);
        

        /**
         * Check whether className is nill
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesIncome_is_className_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);


        
        /**
         * Set className to nill (currently the same as reset)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_className_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);
        

        /**
         * Check whether count is nill
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesIncome_is_count_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);


        
        /**
         * Set count to nill (currently the same as reset)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_count_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);
        

        /**
         * Check whether departure is nill
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesIncome_is_departure_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);


        
        /**
         * Set departure to nill (currently the same as reset)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_departure_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);
        

        /**
         * Check whether factor is nill
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesIncome_is_factor_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);


        
        /**
         * Set factor to nill (currently the same as reset)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_factor_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);
        

        /**
         * Check whether miles is nill
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesIncome_is_miles_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);


        
        /**
         * Set miles to nill (currently the same as reset)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_miles_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);
        

        /**
         * Check whether statCoef is nill
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesIncome_is_statCoef_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);


        
        /**
         * Set statCoef to nill (currently the same as reset)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_statCoef_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);
        

        /**
         * Check whether typeCoef is nill
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesIncome_is_typeCoef_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);


        
        /**
         * Set typeCoef to nill (currently the same as reset)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_set_typeCoef_nil(
                adb_ClassMilesIncome_t* _ClassMilesIncome,
                const axutil_env_t *env);
        

        /**************************** Serialize and Deserialize functions ***************************/
        /*********** These functions are for use only inside the generated code *********************/

        
        /**
         * Wrapper for the deserialization function, will invoke the extension mapper instead
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs, 
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_deserialize(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);

        /**
         * Deserialize an XML to adb objects
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs,
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesIncome_deserialize_obj(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);
                            
            

       /**
         * Declare namespace in the most parent node 
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param parent_element parent element
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index pointer to an int which contain the next namespace index
         */
       void AXIS2_CALL
       adb_ClassMilesIncome_declare_parent_namespaces(
                    adb_ClassMilesIncome_t* _ClassMilesIncome,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index);

        

        /**
         * Wrapper for the serialization function, will invoke the extension mapper instead
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param ClassMilesIncome_om_node node to serialize from
         * @param ClassMilesIncome_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_ClassMilesIncome_serialize(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            axiom_node_t* ClassMilesIncome_om_node, axiom_element_t *ClassMilesIncome_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Serialize to an XML from the adb objects
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @param ClassMilesIncome_om_node node to serialize from
         * @param ClassMilesIncome_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_ClassMilesIncome_serialize_obj(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env,
            axiom_node_t* ClassMilesIncome_om_node, axiom_element_t *ClassMilesIncome_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Check whether the adb_ClassMilesIncome is a particle class (E.g. group, inner sequence)
         * @return whether this is a particle class.
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesIncome_is_particle();

        /******************************* Alternatives for Create and Free functions *********************************/

        

        /**
         * Constructor for creating adb_ClassMilesIncome_t
         * @param env pointer to environment struct
         * @param _arrival axis2_char_t*
         * @param _base double
         * @param _className axis2_char_t*
         * @param _count int
         * @param _departure axis2_char_t*
         * @param _factor double
         * @param _miles double
         * @param _statCoef double
         * @param _typeCoef double
         * @return newly created adb_ClassMilesIncome_t object
         */
        adb_ClassMilesIncome_t* AXIS2_CALL
        adb_ClassMilesIncome_create_with_values(
            const axutil_env_t *env,
                axis2_char_t* _arrival,
                double _base,
                axis2_char_t* _className,
                int _count,
                axis2_char_t* _departure,
                double _factor,
                double _miles,
                double _statCoef,
                double _typeCoef);

        


                /**
                 * Free adb_ClassMilesIncome_t object and return the property value.
                 * You can use this to free the adb object as returning the property value. If there are
                 * many properties, it will only return the first property. Other properties will get freed with the adb object.
                 * @param  _ClassMilesIncome adb_ClassMilesIncome_t object to free
                 * @param env pointer to environment struct
                 * @return the property value holded by the ADB object, if there are many properties only returns the first.
                 */
                axis2_char_t* AXIS2_CALL
                adb_ClassMilesIncome_free_popping_value(
                        adb_ClassMilesIncome_t* _ClassMilesIncome,
                        const axutil_env_t *env);
            

        /******************************* get the value by the property number  *********************************/
        /************NOTE: This method is introduced to resolve a problem in unwrapping mode *******************/

        
        

        /**
         * Getter for arrival by property number (1)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesIncome_get_property1(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

    
        

        /**
         * Getter for base by property number (2)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesIncome_get_property2(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

    
        

        /**
         * Getter for className by property number (3)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesIncome_get_property3(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

    
        

        /**
         * Getter for count by property number (4)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_ClassMilesIncome_get_property4(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

    
        

        /**
         * Getter for departure by property number (5)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesIncome_get_property5(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

    
        

        /**
         * Getter for factor by property number (6)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesIncome_get_property6(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

    
        

        /**
         * Getter for miles by property number (7)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesIncome_get_property7(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

    
        

        /**
         * Getter for statCoef by property number (8)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesIncome_get_property8(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

    
        

        /**
         * Getter for typeCoef by property number (9)
         * @param  _ClassMilesIncome adb_ClassMilesIncome_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesIncome_get_property9(
            adb_ClassMilesIncome_t* _ClassMilesIncome,
            const axutil_env_t *env);

    
     #ifdef __cplusplus
     }
     #endif

     #endif /* ADB_CLASSMILESINCOME_H */
    

