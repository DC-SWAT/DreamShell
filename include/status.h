/** 
 * \file    status.h
 * \brief   DreamShell status handling system
 * \date    2011-2014
 * \author  SWAT www.dc-swat.ru
 */

 /**
 * @file
 *
 * @brief Header file for statuses.
 */

#ifndef DS_STATUS_H
#define DS_STATUS_H


#ifndef _DS_CONSOLE_H
#include <kos/dbgio.h>
int ds_printf(const char *fmt, ...); 
#endif


/**
 * @defgroup DS_status Status Checks
 *
 * @{
 */
 

/**
 * @brief Status codes
 */
typedef enum {
	DS_SUCCESSFUL = 0, 
	DS_TASK_EXITTED = 1, 
	DS_MP_NOT_CONFIGURED = 2, 
	DS_INVALID_NAME = 3,
	DS_INVALID_ID = 4, 
	DS_TOO_MANY = 5, 
	DS_TIMEOUT = 6, 
	DS_OBJECT_WAS_DELETED = 7,
	DS_INVALID_SIZE = 8, 
	DS_INVALID_ADDRESS = 9, 
	DS_INVALID_NUMBER = 10, 
	DS_NOT_DEFINED = 11,
	DS_RESOURCE_IN_USE = 12, 
	DS_UNSATISFIED = 13, 
	DS_INCORRECT_STATE = 14, 
	DS_ALREADY_SUSPENDED = 15,
	DS_ILLEGAL_ON_SELF = 16, 
	DS_ILLEGAL_ON_REMOTE_OBJECT = 17, 
	DS_CALLED_FROM_ISR = 18, 
	DS_INVALID_PRIORITY = 19,
	DS_INVALID_CLOCK = 20, 
	DS_INVALID_NODE = 21, 
	DS_NOT_CONFIGURED = 22, 
	DS_NOT_OWNER_OF_RESOURCE = 23,
	DS_NOT_IMPLEMENTED = 24, 
	DS_INTERNAL_ERROR = 25, 
	DS_NO_MEMORY = 26, 
	DS_IO_ERROR = 27,
	DS_PROXY_BLOCKING = 28
} DStatus_t;



/**
 * @name Print Macros
 *
 * @{
 */

/**
 * @brief General purpose debug print macro.
 */
#ifdef DEBUG
  #ifndef DS_DEBUG_PRINT
    #ifdef DS_STATUS_USE_DS_PRINTF
      #define DS_DEBUG_PRINT( fmt, ...) \
        ds_printf( "%s: " fmt, __func__, ##__VA_ARGS__)
    #else /* DS_STATUS_USE_DS_PRINTF */
      #include <stdio.h>
      #define DS_DEBUG_PRINT( fmt, ...) \
        dbglog(DBG_DEBUG, "%s: " fmt, __func__, ##__VA_ARGS__)
    #endif /* DS_STATUS_USE_DS_PRINTF */
  #endif /* DS_DEBUG_PRINT */
#else /* DEBUG */
  #ifdef DS_DEBUG_PRINT
    #warning DS_DEBUG_PRINT was defined, but DEBUG was undefined
    #undef DS_DEBUG_PRINT
  #endif /* DS_DEBUG_PRINT */
  #define DS_DEBUG_PRINT( fmt, ...)
#endif /* DEBUG */

/**
 * @brief Macro to print debug messages for successful operations.
 */
#define DS_DEBUG_OK( msg) \
  DS_DEBUG_PRINT( "DS_OK: %s\n", msg)

/**
 * @brief General purpose system log print macro.
 */
#ifndef DS_SYSLOG_PRINT
  #ifdef DS_STATUS_USE_DS_PRINTF
    #define DS_SYSLOG_PRINT( fmt, ...) \
      ds_printf( fmt, ##__VA_ARGS__)
  #else /* DS_STATUS_USE_DS_PRINTF */
    #include <stdio.h>
    #define DS_SYSLOG_PRINT( fmt, ...) \
      dbglog(DBG_DEBUG, fmt, ##__VA_ARGS__)
  #endif /* DS_STATUS_USE_DS_PRINTF */
#endif /* DS_SYSLOG_PRINT */

/**
 * @brief General purpose system log macro.
 */
#define DS_SYSLOG( fmt, ...) \
  DS_SYSLOG_PRINT( "%s: " fmt, __func__, ##__VA_ARGS__)

/**
 * @brief General purpose system log macro for warnings.
 */
#define DS_SYSLOG_WARNING( fmt, ...) \
  DS_SYSLOG( "DS_WARNING: " fmt, ##__VA_ARGS__)

/**
 * @brief Macro to generate a system log warning message if the status code @a
 * sc is not equal to @ref DS_SUCCESSFUL.
 */
#define DS_SYSLOG_WARNING_SC( sc, msg) \
  if ((DStatus_t) (sc) != DS_SUCCESSFUL) { \
    DS_SYSLOG_WARNING( "SC = %i: %s\n", (int) sc, msg); \
  }

/**
 * @brief General purpose system log macro for errors.
 */
#define DS_SYSLOG_ERROR( fmt, ...) \
  DS_SYSLOG( "DS_ERROR: " fmt, ##__VA_ARGS__)

/**
 * @brief Macro for system log error messages with status code.
 */
#define DS_SYSLOG_ERROR_WITH_SC( sc, msg) \
  DS_SYSLOG_ERROR( "SC = %i: %s\n", (int) sc, msg);

/**
 * @brief Macro for system log error messages with return value.
 */
#define DS_SYSLOG_ERROR_WITH_RV( rv, msg) \
  DS_SYSLOG_ERROR( "RV = %i: %s\n", (int) rv, msg);

/**
 * @brief Macro to generate a system log error message if the status code @a
 * sc is not equal to @ref DS_SUCCESSFUL.
 */
#define DS_SYSLOG_ERROR_SC( sc, msg) \
  if ((DStatus_t) (sc) != DS_SUCCESSFUL) { \
    DS_SYSLOG_ERROR_WITH_SC( sc, msg); \
  }

/**
 * @brief Macro to generate a system log error message if the return value @a
 * rv is less than zero.
 */
#define DS_SYSLOG_ERROR_RV( rv, msg) \
  if ((int) (rv) < 0) { \
    DS_SYSLOG_ERROR_WITH_RV( rv, msg); \
  }

/** @} */

/**
 * @name Check Macros
 *
 * @{
 */

/**
 * @brief Prints message @a msg and returns with status code @a sc if the status
 * code @a sc is not equal to @ref DS_SUCCESSFUL.
 */
#define DS_CHECK_SC( sc, msg) \
  if ((DStatus_t) (sc) != DS_SUCCESSFUL) { \
    DS_SYSLOG_ERROR_WITH_SC( sc, msg); \
    return (DStatus_t) sc; \
  } else { \
    DS_DEBUG_OK( msg); \
  }

/**
 * @brief Prints message @a msg and returns with a return value of negative @a sc
 * if the status code @a sc is not equal to @ref DS_SUCCESSFUL.
 */
#define DS_CHECK_SC_RV( sc, msg) \
  if ((DStatus_t) (sc) != DS_SUCCESSFUL) { \
    DS_SYSLOG_ERROR_WITH_SC( sc, msg); \
    return -((int) (sc)); \
  } else { \
    DS_DEBUG_OK( msg); \
  }

/**
 * @brief Prints message @a msg and returns if the status code @a sc is not equal
 * to @ref DS_SUCCESSFUL.
 */
#define DS_CHECK_SC_VOID( sc, msg) \
  if ((DStatus_t) (sc) != DS_SUCCESSFUL) { \
    DS_SYSLOG_ERROR_WITH_SC( sc, msg); \
    return; \
  } else { \
    DS_DEBUG_OK( msg); \
  }

/**
 * @brief Prints message @a msg and returns with a return value @a rv if the
 * return value @a rv is less than zero.
 */
#define DS_CHECK_RV( rv, msg) \
  if ((int) (rv) < 0) { \
    DS_SYSLOG_ERROR_WITH_RV( rv, msg); \
    return (int) rv; \
  } else { \
    DS_DEBUG_OK( msg); \
  }

/**
 * @brief Prints message @a msg and returns with status code @ref DS_IO_ERROR
 * if the return value @a rv is less than zero.
 */
#define DS_CHECK_RV_SC( rv, msg) \
  if ((int) (rv) < 0) { \
    DS_SYSLOG_ERROR_WITH_RV( rv, msg); \
    return DS_IO_ERROR; \
  } else { \
    DS_DEBUG_OK( msg); \
  }

/**
 * @brief Prints message @a msg and returns if the return value @a rv is less
 * than zero.
 */
#define DS_CHECK_RV_VOID( rv, msg) \
  if ((int) (rv) < 0) { \
    DS_SYSLOG_ERROR_WITH_RV( rv, msg); \
    return; \
  } else { \
    DS_DEBUG_OK( msg); \
  }

/** @} */

/**
 * @name Cleanup Macros
 *
 * @{
 */

/**
 * @brief Prints message @a msg and jumps to @a label if the status code @a sc
 * is not equal to @ref DS_SUCCESSFUL.
 */
#define DS_CLEANUP_SC( sc, label, msg) \
  if ((DStatus_t) (sc) != DS_SUCCESSFUL) { \
    DS_SYSLOG_ERROR_WITH_SC( sc, msg); \
    goto label; \
  } else { \
    DS_DEBUG_OK( msg); \
  }

/**
 * @brief Prints message @a msg and jumps to @a label if the status code @a sc
 * is not equal to @ref DS_SUCCESSFUL.  The return value variable @a rv will
 * be set to a negative @a sc in this case.
 */
#define DS_CLEANUP_SC_RV( sc, rv, label, msg) \
  if ((DStatus_t) (sc) != DS_SUCCESSFUL) { \
    DS_SYSLOG_ERROR_WITH_SC( sc, msg); \
    rv = -((int) (sc)); \
    goto label; \
  } else { \
    DS_DEBUG_OK( msg); \
  }

/**
 * @brief Prints message @a msg and jumps to @a label if the return value @a rv
 * is less than zero.
 */
#define DS_CLEANUP_RV( rv, label, msg) \
  if ((int) (rv) < 0) { \
    DS_SYSLOG_ERROR_WITH_RV( rv, msg); \
    goto label; \
  } else { \
    DS_DEBUG_OK( msg); \
  }

/**
 * @brief Prints message @a msg and jumps to @a label if the return value @a rv
 * is less than zero.  The status code variable @a sc will be set to @ref
 * DS_IO_ERROR in this case.
 */
#define DS_CLEANUP_RV_SC( rv, sc, label, msg) \
  if ((int) (rv) < 0) { \
    DS_SYSLOG_ERROR_WITH_RV( rv, msg); \
    sc = DS_IO_ERROR; \
    goto label; \
  } else { \
    DS_DEBUG_OK( msg); \
  }

/**
 * @brief Prints message @a msg and jumps to @a label.
 */
#define DS_DO_CLEANUP( label, msg) \
  do { \
    DS_SYSLOG_ERROR( msg); \
    goto label; \
  } while (0)

/**
 * @brief Prints message @a msg, sets the status code variable @a sc to @a val
 * and jumps to @a label.
 */
#define DS_DO_CLEANUP_SC( val, sc, label, msg) \
  do { \
    sc = (DStatus_t) val; \
    DS_SYSLOG_ERROR_WITH_SC( sc, msg); \
    goto label; \
  } while (0)

/**
 * @brief Prints message @a msg, sets the return value variable @a rv to @a val
 * and jumps to @a label.
 */
#define DS_DO_CLEANUP_RV( val, rv, label, msg) \
  do { \
    rv = (int) val; \
    DS_SYSLOG_ERROR_WITH_RV( rv, msg); \
    goto label; \
  } while (0)

/** @} */

/** @} */



#endif /* DS_STATUS_H */
