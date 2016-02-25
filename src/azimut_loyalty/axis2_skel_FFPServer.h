

    /**
     * axis2_skel_FFPServer.h
     *
     * This file was auto-generated from WSDL for "FFPServer|http://ffpServer.aeronavigator.ru" service
     * by the Apache Axis2/C version: 1.7.0  Built on : Jan 18, 2016 (09:41:27 GMT)
     * axis2_skel_FFPServer Axis2/C skeleton for the axisService- Header file
     */

    #ifndef AXIS2_SKEL_FFPSERVER_H
    #define AXIS2_SKEL_FFPSERVER_H

	#include <axis2_svc_skeleton.h>
	#include <axutil_log_default.h>
	#include <axutil_error_default.h>
    #include <axutil_error.h>
	#include <axiom_text.h>
	#include <axiom_node.h>
	#include <axiom_element.h>
    #include <stdio.h>


   
     #include "adb_login.h"
    
     #include "adb_loginResponseE3.h"
    
     #include "adb_payMiles.h"
    
     #include "adb_payMilesResponseE1.h"
    
     #include "adb_getPrice.h"
    
     #include "adb_getPriceResponse.h"
    
     #include "adb_getBonus.h"
    
     #include "adb_getBonusResponse.h"
    
     #include "adb_getReserveStatus.h"
    
     #include "adb_getReserveStatusResponse.h"
    
     #include "adb_cancelReserve.h"
    
     #include "adb_cancelReserveResponseE2.h"
    
     #include "adb_getAvailableMiles.h"
    
     #include "adb_getAvailableMilesResponse.h"
    
     #include "adb_reserveMiles.h"
    
     #include "adb_reserveMilesResponseE0.h"
    

	#ifdef __cplusplus
	extern "C" {
	#endif

     

		 
        /**
         * auto generated function declaration
         * for "login|http://ffpServer.aeronavigator.ru" operation.
         * @param env environment ( mandatory)* @param MessageContext the outmessage context
         * @param _login of the adb_login_t*
         *
         * @return adb_loginResponseE3_t*
         */
        adb_loginResponseE3_t* axis2_skel_FFPServer_login(const axutil_env_t *env,axis2_msg_ctx_t *msg_ctx,
                                              adb_login_t* _login);


     

		 
        /**
         * auto generated function declaration
         * for "payMiles|http://ffpServer.aeronavigator.ru" operation.
         * @param env environment ( mandatory)* @param MessageContext the outmessage context
         * @param _payMiles of the adb_payMiles_t*
         *
         * @return adb_payMilesResponseE1_t*
         */
        adb_payMilesResponseE1_t* axis2_skel_FFPServer_payMiles(const axutil_env_t *env,axis2_msg_ctx_t *msg_ctx,
                                              adb_payMiles_t* _payMiles);


     

		 
        /**
         * auto generated function declaration
         * for "getPrice|http://ffpServer.aeronavigator.ru" operation.
         * @param env environment ( mandatory)* @param MessageContext the outmessage context
         * @param _getPrice of the adb_getPrice_t*
         *
         * @return adb_getPriceResponse_t*
         */
        adb_getPriceResponse_t* axis2_skel_FFPServer_getPrice(const axutil_env_t *env,axis2_msg_ctx_t *msg_ctx,
                                              adb_getPrice_t* _getPrice);


     

		 
        /**
         * auto generated function declaration
         * for "getBonus|http://ffpServer.aeronavigator.ru" operation.
         * @param env environment ( mandatory)* @param MessageContext the outmessage context
         * @param _getBonus of the adb_getBonus_t*
         *
         * @return adb_getBonusResponse_t*
         */
        adb_getBonusResponse_t* axis2_skel_FFPServer_getBonus(const axutil_env_t *env,axis2_msg_ctx_t *msg_ctx,
                                              adb_getBonus_t* _getBonus);


     

		 
        /**
         * auto generated function declaration
         * for "getReserveStatus|http://ffpServer.aeronavigator.ru" operation.
         * @param env environment ( mandatory)* @param MessageContext the outmessage context
         * @param _getReserveStatus of the adb_getReserveStatus_t*
         *
         * @return adb_getReserveStatusResponse_t*
         */
        adb_getReserveStatusResponse_t* axis2_skel_FFPServer_getReserveStatus(const axutil_env_t *env,axis2_msg_ctx_t *msg_ctx,
                                              adb_getReserveStatus_t* _getReserveStatus);


     

		 
        /**
         * auto generated function declaration
         * for "cancelReserve|http://ffpServer.aeronavigator.ru" operation.
         * @param env environment ( mandatory)* @param MessageContext the outmessage context
         * @param _cancelReserve of the adb_cancelReserve_t*
         *
         * @return adb_cancelReserveResponseE2_t*
         */
        adb_cancelReserveResponseE2_t* axis2_skel_FFPServer_cancelReserve(const axutil_env_t *env,axis2_msg_ctx_t *msg_ctx,
                                              adb_cancelReserve_t* _cancelReserve);


     

		 
        /**
         * auto generated function declaration
         * for "getAvailableMiles|http://ffpServer.aeronavigator.ru" operation.
         * @param env environment ( mandatory)* @param MessageContext the outmessage context
         * @param _getAvailableMiles of the adb_getAvailableMiles_t*
         *
         * @return adb_getAvailableMilesResponse_t*
         */
        adb_getAvailableMilesResponse_t* axis2_skel_FFPServer_getAvailableMiles(const axutil_env_t *env,axis2_msg_ctx_t *msg_ctx,
                                              adb_getAvailableMiles_t* _getAvailableMiles);


     

		 
        /**
         * auto generated function declaration
         * for "reserveMiles|http://ffpServer.aeronavigator.ru" operation.
         * @param env environment ( mandatory)* @param MessageContext the outmessage context
         * @param _reserveMiles of the adb_reserveMiles_t*
         *
         * @return adb_reserveMilesResponseE0_t*
         */
        adb_reserveMilesResponseE0_t* axis2_skel_FFPServer_reserveMiles(const axutil_env_t *env,axis2_msg_ctx_t *msg_ctx,
                                              adb_reserveMiles_t* _reserveMiles);


     

    /** we have to reserve some error codes for adb and for custom messages */
    #define AXIS2_SKEL_FFPSERVER_ERROR_CODES_START (AXIS2_ERROR_LAST + 2500)

    typedef enum 
    {
        AXIS2_SKEL_FFPSERVER_ERROR_NONE = AXIS2_SKEL_FFPSERVER_ERROR_CODES_START,
        
        AXIS2_SKEL_FFPSERVER_ERROR_LAST
    } axis2_skel_FFPServer_error_codes;

	#ifdef __cplusplus
	}
	#endif

    #endif
    

