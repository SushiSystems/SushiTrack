/**
 * @file      SushiTrackTypes.h
 * @brief     Status codes, logging levels, and callback definitions for the SushiTrack library.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#ifdef __cplusplus
extern "C" 
{
#endif

    /**
     * @brief Enumeration of return status codes for tracking operations.
     */
    typedef enum 
    {
        SUSHITRACK_SUCCESS = 0,                 ///< Operation completed successfully.
        SUSHITRACK_INVALID_ARGUMENT = -1,       ///< Parameter validation failure.
        SUSHITRACK_INITIALIZATION_FAILURE = -2, ///< Internal component setup failure.
        SUSHITRACK_OUT_OF_MEMORY = -3,          ///< Dynamic memory allocation failure.
        SUSHITRACK_RUNTIME_ERROR = -4,          ///< Unhandled execution exception.
        SUSHITRACK_BUFFER_OVERFLOW = -5         ///< Target buffer capacity exceeded.
    
    } SushiTrack_Status;

    /**
     * @brief Enumeration of system logging levels.
     */
    typedef enum 
    {
        SUSHITRACK_LOG_DEBUG = 0, ///< Detailed diagnostic information.
        SUSHITRACK_LOG_INFO  = 1, ///< Informational system messages.
        SUSHITRACK_LOG_WARN  = 2, ///< Non-terminating operational warnings.
        SUSHITRACK_LOG_ERROR = 3  ///< Terminating operational errors.
    
    } SushiTrack_LogLevel;

    /**
     * @brief Function pointer type for custom logging implementations.
     */
    typedef void (*SushiTrack_LogCallback)(SushiTrack_LogLevel level, const char* message);

#ifdef __cplusplus
}
#endif
