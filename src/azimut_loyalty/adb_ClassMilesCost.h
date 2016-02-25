

        #ifndef ADB_CLASSMILESCOST_H
        #define ADB_CLASSMILESCOST_H

       /**
        * adb_ClassMilesCost.h
        *
        * This file was auto-generated from WSDL
        * by the Apache Axis2/Java version: 1.7.0  Built on : Jan 18, 2016 (09:42:13 GMT)
        */

       /**
        *  adb_ClassMilesCost class
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
        

        typedef struct adb_ClassMilesCost adb_ClassMilesCost_t;

        
        

        /******************************* Create and Free functions *********************************/

        /**
         * Constructor for creating adb_ClassMilesCost_t
         * @param env pointer to environment struct
         * @return newly created adb_ClassMilesCost_t object
         */
        adb_ClassMilesCost_t* AXIS2_CALL
        adb_ClassMilesCost_create(
            const axutil_env_t *env );

        /**
         * Wrapper for the "free" function, will invoke the extension mapper instead
         * @param  _ClassMilesCost adb_ClassMilesCost_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_free (
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        /**
         * Free adb_ClassMilesCost_t object
         * @param  _ClassMilesCost adb_ClassMilesCost_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_free_obj (
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);



        /********************************** Getters and Setters **************************************/
        
        

        /**
         * Getter for arrival. 
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesCost_get_arrival(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        /**
         * Setter for arrival.
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param arg_arrival axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_arrival(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            const axis2_char_t*  arg_arrival);

        /**
         * Resetter for arrival
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_reset_arrival(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        
        

        /**
         * Getter for arrivalRegion. 
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesCost_get_arrivalRegion(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        /**
         * Setter for arrivalRegion.
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param arg_arrivalRegion axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_arrivalRegion(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            const axis2_char_t*  arg_arrivalRegion);

        /**
         * Resetter for arrivalRegion
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_reset_arrivalRegion(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        
        

        /**
         * Getter for classDesc. 
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesCost_get_classDesc(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        /**
         * Setter for classDesc.
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param arg_classDesc axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_classDesc(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            const axis2_char_t*  arg_classDesc);

        /**
         * Resetter for classDesc
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_reset_classDesc(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        
        

        /**
         * Getter for className. 
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesCost_get_className(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        /**
         * Setter for className.
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param arg_className axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_className(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            const axis2_char_t*  arg_className);

        /**
         * Resetter for className
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_reset_className(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        
        

        /**
         * Getter for count. 
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_ClassMilesCost_get_count(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        /**
         * Setter for count.
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param arg_count int
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_count(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            const int  arg_count);

        /**
         * Resetter for count
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_reset_count(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        
        

        /**
         * Getter for departure. 
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesCost_get_departure(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        /**
         * Setter for departure.
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param arg_departure axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_departure(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            const axis2_char_t*  arg_departure);

        /**
         * Resetter for departure
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_reset_departure(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        
        

        /**
         * Getter for departureRegion. 
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesCost_get_departureRegion(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        /**
         * Setter for departureRegion.
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param arg_departureRegion axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_departureRegion(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            const axis2_char_t*  arg_departureRegion);

        /**
         * Resetter for departureRegion
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_reset_departureRegion(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        
        

        /**
         * Getter for milesOW. 
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesCost_get_milesOW(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        /**
         * Setter for milesOW.
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param arg_milesOW double
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_milesOW(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            const double  arg_milesOW);

        /**
         * Resetter for milesOW
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_reset_milesOW(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        
        

        /**
         * Getter for milesRT. 
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesCost_get_milesRT(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        /**
         * Setter for milesRT.
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param arg_milesRT double
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_milesRT(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            const double  arg_milesRT);

        /**
         * Resetter for milesRT
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_reset_milesRT(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

        


        /******************************* Checking and Setting NIL values *********************************/
        

        /**
         * NOTE: set_nil is only available for nillable properties
         */

        

        /**
         * Check whether arrival is nill
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesCost_is_arrival_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);


        
        /**
         * Set arrival to nill (currently the same as reset)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_arrival_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);
        

        /**
         * Check whether arrivalRegion is nill
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesCost_is_arrivalRegion_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);


        
        /**
         * Set arrivalRegion to nill (currently the same as reset)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_arrivalRegion_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);
        

        /**
         * Check whether classDesc is nill
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesCost_is_classDesc_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);


        
        /**
         * Set classDesc to nill (currently the same as reset)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_classDesc_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);
        

        /**
         * Check whether className is nill
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesCost_is_className_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);


        
        /**
         * Set className to nill (currently the same as reset)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_className_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);
        

        /**
         * Check whether count is nill
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesCost_is_count_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);


        
        /**
         * Set count to nill (currently the same as reset)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_count_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);
        

        /**
         * Check whether departure is nill
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesCost_is_departure_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);


        
        /**
         * Set departure to nill (currently the same as reset)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_departure_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);
        

        /**
         * Check whether departureRegion is nill
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesCost_is_departureRegion_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);


        
        /**
         * Set departureRegion to nill (currently the same as reset)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_departureRegion_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);
        

        /**
         * Check whether milesOW is nill
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesCost_is_milesOW_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);


        
        /**
         * Set milesOW to nill (currently the same as reset)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_milesOW_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);
        

        /**
         * Check whether milesRT is nill
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesCost_is_milesRT_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);


        
        /**
         * Set milesRT to nill (currently the same as reset)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_set_milesRT_nil(
                adb_ClassMilesCost_t* _ClassMilesCost,
                const axutil_env_t *env);
        

        /**************************** Serialize and Deserialize functions ***************************/
        /*********** These functions are for use only inside the generated code *********************/

        
        /**
         * Wrapper for the deserialization function, will invoke the extension mapper instead
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs, 
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_deserialize(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);

        /**
         * Deserialize an XML to adb objects
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs,
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ClassMilesCost_deserialize_obj(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);
                            
            

       /**
         * Declare namespace in the most parent node 
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param parent_element parent element
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index pointer to an int which contain the next namespace index
         */
       void AXIS2_CALL
       adb_ClassMilesCost_declare_parent_namespaces(
                    adb_ClassMilesCost_t* _ClassMilesCost,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index);

        

        /**
         * Wrapper for the serialization function, will invoke the extension mapper instead
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param ClassMilesCost_om_node node to serialize from
         * @param ClassMilesCost_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_ClassMilesCost_serialize(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            axiom_node_t* ClassMilesCost_om_node, axiom_element_t *ClassMilesCost_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Serialize to an XML from the adb objects
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @param ClassMilesCost_om_node node to serialize from
         * @param ClassMilesCost_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_ClassMilesCost_serialize_obj(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env,
            axiom_node_t* ClassMilesCost_om_node, axiom_element_t *ClassMilesCost_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Check whether the adb_ClassMilesCost is a particle class (E.g. group, inner sequence)
         * @return whether this is a particle class.
         */
        axis2_bool_t AXIS2_CALL
        adb_ClassMilesCost_is_particle();

        /******************************* Alternatives for Create and Free functions *********************************/

        

        /**
         * Constructor for creating adb_ClassMilesCost_t
         * @param env pointer to environment struct
         * @param _arrival axis2_char_t*
         * @param _arrivalRegion axis2_char_t*
         * @param _classDesc axis2_char_t*
         * @param _className axis2_char_t*
         * @param _count int
         * @param _departure axis2_char_t*
         * @param _departureRegion axis2_char_t*
         * @param _milesOW double
         * @param _milesRT double
         * @return newly created adb_ClassMilesCost_t object
         */
        adb_ClassMilesCost_t* AXIS2_CALL
        adb_ClassMilesCost_create_with_values(
            const axutil_env_t *env,
                axis2_char_t* _arrival,
                axis2_char_t* _arrivalRegion,
                axis2_char_t* _classDesc,
                axis2_char_t* _className,
                int _count,
                axis2_char_t* _departure,
                axis2_char_t* _departureRegion,
                double _milesOW,
                double _milesRT);

        


                /**
                 * Free adb_ClassMilesCost_t object and return the property value.
                 * You can use this to free the adb object as returning the property value. If there are
                 * many properties, it will only return the first property. Other properties will get freed with the adb object.
                 * @param  _ClassMilesCost adb_ClassMilesCost_t object to free
                 * @param env pointer to environment struct
                 * @return the property value holded by the ADB object, if there are many properties only returns the first.
                 */
                axis2_char_t* AXIS2_CALL
                adb_ClassMilesCost_free_popping_value(
                        adb_ClassMilesCost_t* _ClassMilesCost,
                        const axutil_env_t *env);
            

        /******************************* get the value by the property number  *********************************/
        /************NOTE: This method is introduced to resolve a problem in unwrapping mode *******************/

        
        

        /**
         * Getter for arrival by property number (1)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesCost_get_property1(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

    
        

        /**
         * Getter for arrivalRegion by property number (2)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesCost_get_property2(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

    
        

        /**
         * Getter for classDesc by property number (3)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesCost_get_property3(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

    
        

        /**
         * Getter for className by property number (4)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesCost_get_property4(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

    
        

        /**
         * Getter for count by property number (5)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_ClassMilesCost_get_property5(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

    
        

        /**
         * Getter for departure by property number (6)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesCost_get_property6(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

    
        

        /**
         * Getter for departureRegion by property number (7)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ClassMilesCost_get_property7(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

    
        

        /**
         * Getter for milesOW by property number (8)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesCost_get_property8(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

    
        

        /**
         * Getter for milesRT by property number (9)
         * @param  _ClassMilesCost adb_ClassMilesCost_t object
         * @param env pointer to environment struct
         * @return double
         */
        double AXIS2_CALL
        adb_ClassMilesCost_get_property9(
            adb_ClassMilesCost_t* _ClassMilesCost,
            const axutil_env_t *env);

    
     #ifdef __cplusplus
     }
     #endif

     #endif /* ADB_CLASSMILESCOST_H */
    

