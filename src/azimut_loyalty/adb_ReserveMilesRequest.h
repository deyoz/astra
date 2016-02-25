

        #ifndef ADB_RESERVEMILESREQUEST_H
        #define ADB_RESERVEMILESREQUEST_H

       /**
        * adb_ReserveMilesRequest.h
        *
        * This file was auto-generated from WSDL
        * by the Apache Axis2/Java version: 1.7.0  Built on : Jan 18, 2016 (09:42:13 GMT)
        */

       /**
        *  adb_ReserveMilesRequest class
        */

        
            #include <axutil_date_time.h>
          

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
        

        typedef struct adb_ReserveMilesRequest adb_ReserveMilesRequest_t;

        
        

        /******************************* Create and Free functions *********************************/

        /**
         * Constructor for creating adb_ReserveMilesRequest_t
         * @param env pointer to environment struct
         * @return newly created adb_ReserveMilesRequest_t object
         */
        adb_ReserveMilesRequest_t* AXIS2_CALL
        adb_ReserveMilesRequest_create(
            const axutil_env_t *env );

        /**
         * Wrapper for the "free" function, will invoke the extension mapper instead
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_free (
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        /**
         * Free adb_ReserveMilesRequest_t object
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object to free
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_free_obj (
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);



        /********************************** Getters and Setters **************************************/
        
        

        /**
         * Getter for arrival. 
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_arrival(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        /**
         * Setter for arrival.
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_arrival axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_arrival(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_arrival);

        /**
         * Resetter for arrival
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_reset_arrival(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for departure. 
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_departure(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        /**
         * Setter for departure.
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_departure axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_departure(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_departure);

        /**
         * Resetter for departure
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_reset_departure(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for ffpCardNumber. 
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_ffpCardNumber(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        /**
         * Setter for ffpCardNumber.
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_ffpCardNumber axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_ffpCardNumber(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_ffpCardNumber);

        /**
         * Resetter for ffpCardNumber
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_reset_ffpCardNumber(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for flightClass. 
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_flightClass(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        /**
         * Setter for flightClass.
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_flightClass axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_flightClass(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_flightClass);

        /**
         * Resetter for flightClass
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_reset_flightClass(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for flightDate. 
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axutil_date_time_t*
         */
        axutil_date_time_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_flightDate(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        /**
         * Setter for flightDate.
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_flightDate axutil_date_time_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_flightDate(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            axutil_date_time_t*  arg_flightDate);

        /**
         * Resetter for flightDate
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_reset_flightDate(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for flightNumber. 
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_flightNumber(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        /**
         * Setter for flightNumber.
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_flightNumber axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_flightNumber(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_flightNumber);

        /**
         * Resetter for flightNumber
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_reset_flightNumber(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for isRT. 
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_bool_t
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesRequest_get_isRT(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        /**
         * Setter for isRT.
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_isRT axis2_bool_t
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_isRT(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            axis2_bool_t  arg_isRT);

        /**
         * Resetter for isRT
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_reset_isRT(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for miles. 
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_ReserveMilesRequest_get_miles(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        /**
         * Setter for miles.
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_miles int
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_miles(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            const int  arg_miles);

        /**
         * Resetter for miles
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_reset_miles(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        
        

        /**
         * Getter for pnr. 
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_pnr(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        /**
         * Setter for pnr.
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param arg_pnr axis2_char_t*
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_pnr(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            const axis2_char_t*  arg_pnr);

        /**
         * Resetter for pnr
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_reset_pnr(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

        


        /******************************* Checking and Setting NIL values *********************************/
        

        /**
         * NOTE: set_nil is only available for nillable properties
         */

        

        /**
         * Check whether arrival is nill
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesRequest_is_arrival_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);


        
        /**
         * Set arrival to nill (currently the same as reset)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_arrival_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether departure is nill
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesRequest_is_departure_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);


        
        /**
         * Set departure to nill (currently the same as reset)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_departure_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether ffpCardNumber is nill
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesRequest_is_ffpCardNumber_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);


        
        /**
         * Set ffpCardNumber to nill (currently the same as reset)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_ffpCardNumber_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether flightClass is nill
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesRequest_is_flightClass_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);


        
        /**
         * Set flightClass to nill (currently the same as reset)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_flightClass_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether flightDate is nill
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesRequest_is_flightDate_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);


        
        /**
         * Set flightDate to nill (currently the same as reset)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_flightDate_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether flightNumber is nill
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesRequest_is_flightNumber_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);


        
        /**
         * Set flightNumber to nill (currently the same as reset)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_flightNumber_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether isRT is nill
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesRequest_is_isRT_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);


        
        /**
         * Set isRT to nill (currently the same as reset)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_isRT_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether miles is nill
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesRequest_is_miles_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);


        
        /**
         * Set miles to nill (currently the same as reset)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_miles_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);
        

        /**
         * Check whether pnr is nill
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_TRUE if the element is nil or AXIS2_FALSE otherwise
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesRequest_is_pnr_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);


        
        /**
         * Set pnr to nill (currently the same as reset)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_set_pnr_nil(
                adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                const axutil_env_t *env);
        

        /**************************** Serialize and Deserialize functions ***************************/
        /*********** These functions are for use only inside the generated code *********************/

        
        /**
         * Wrapper for the deserialization function, will invoke the extension mapper instead
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs, 
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_deserialize(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);

        /**
         * Deserialize an XML to adb objects
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param dp_parent double pointer to the parent node to deserialize
         * @param dp_is_early_node_valid double pointer to a flag (is_early_node_valid?)
         * @param dont_care_minoccurs Dont set errors on validating minoccurs,
         *              (Parent will order this in a case of choice)
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axis2_status_t AXIS2_CALL
        adb_ReserveMilesRequest_deserialize_obj(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            axiom_node_t** dp_parent,
            axis2_bool_t *dp_is_early_node_valid,
            axis2_bool_t dont_care_minoccurs);
                            
            

       /**
         * Declare namespace in the most parent node 
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param parent_element parent element
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index pointer to an int which contain the next namespace index
         */
       void AXIS2_CALL
       adb_ReserveMilesRequest_declare_parent_namespaces(
                    adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                    const axutil_env_t *env, axiom_element_t *parent_element,
                    axutil_hash_t *namespaces, int *next_ns_index);

        

        /**
         * Wrapper for the serialization function, will invoke the extension mapper instead
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param ReserveMilesRequest_om_node node to serialize from
         * @param ReserveMilesRequest_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_ReserveMilesRequest_serialize(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            axiom_node_t* ReserveMilesRequest_om_node, axiom_element_t *ReserveMilesRequest_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Serialize to an XML from the adb objects
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @param ReserveMilesRequest_om_node node to serialize from
         * @param ReserveMilesRequest_om_element parent element to serialize from
         * @param tag_closed whether the parent tag is closed or not
         * @param namespaces hash of namespace uri to prefix
         * @param next_ns_index an int which contain the next namespace index
         * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE
         */
        axiom_node_t* AXIS2_CALL
        adb_ReserveMilesRequest_serialize_obj(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env,
            axiom_node_t* ReserveMilesRequest_om_node, axiom_element_t *ReserveMilesRequest_om_element, int tag_closed, axutil_hash_t *namespaces, int *next_ns_index);

        /**
         * Check whether the adb_ReserveMilesRequest is a particle class (E.g. group, inner sequence)
         * @return whether this is a particle class.
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesRequest_is_particle();

        /******************************* Alternatives for Create and Free functions *********************************/

        

        /**
         * Constructor for creating adb_ReserveMilesRequest_t
         * @param env pointer to environment struct
         * @param _arrival axis2_char_t*
         * @param _departure axis2_char_t*
         * @param _ffpCardNumber axis2_char_t*
         * @param _flightClass axis2_char_t*
         * @param _flightDate axutil_date_time_t*
         * @param _flightNumber axis2_char_t*
         * @param _isRT axis2_bool_t
         * @param _miles int
         * @param _pnr axis2_char_t*
         * @return newly created adb_ReserveMilesRequest_t object
         */
        adb_ReserveMilesRequest_t* AXIS2_CALL
        adb_ReserveMilesRequest_create_with_values(
            const axutil_env_t *env,
                axis2_char_t* _arrival,
                axis2_char_t* _departure,
                axis2_char_t* _ffpCardNumber,
                axis2_char_t* _flightClass,
                axutil_date_time_t* _flightDate,
                axis2_char_t* _flightNumber,
                axis2_bool_t _isRT,
                int _miles,
                axis2_char_t* _pnr);

        


                /**
                 * Free adb_ReserveMilesRequest_t object and return the property value.
                 * You can use this to free the adb object as returning the property value. If there are
                 * many properties, it will only return the first property. Other properties will get freed with the adb object.
                 * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object to free
                 * @param env pointer to environment struct
                 * @return the property value holded by the ADB object, if there are many properties only returns the first.
                 */
                axis2_char_t* AXIS2_CALL
                adb_ReserveMilesRequest_free_popping_value(
                        adb_ReserveMilesRequest_t* _ReserveMilesRequest,
                        const axutil_env_t *env);
            

        /******************************* get the value by the property number  *********************************/
        /************NOTE: This method is introduced to resolve a problem in unwrapping mode *******************/

        
        

        /**
         * Getter for arrival by property number (1)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_property1(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for departure by property number (2)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_property2(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for ffpCardNumber by property number (3)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_property3(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for flightClass by property number (4)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_property4(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for flightDate by property number (5)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axutil_date_time_t*
         */
        axutil_date_time_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_property5(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for flightNumber by property number (6)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_property6(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for isRT by property number (7)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_bool_t
         */
        axis2_bool_t AXIS2_CALL
        adb_ReserveMilesRequest_get_property7(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for miles by property number (8)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return int
         */
        int AXIS2_CALL
        adb_ReserveMilesRequest_get_property8(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

    
        

        /**
         * Getter for pnr by property number (9)
         * @param  _ReserveMilesRequest adb_ReserveMilesRequest_t object
         * @param env pointer to environment struct
         * @return axis2_char_t*
         */
        axis2_char_t* AXIS2_CALL
        adb_ReserveMilesRequest_get_property9(
            adb_ReserveMilesRequest_t* _ReserveMilesRequest,
            const axutil_env_t *env);

    
     #ifdef __cplusplus
     }
     #endif

     #endif /* ADB_RESERVEMILESREQUEST_H */
    

