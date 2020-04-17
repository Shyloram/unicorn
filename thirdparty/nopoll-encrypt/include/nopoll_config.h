/*
 * Nopoll Library nopoll_config.h
 * Platform dependant definitions.
 *
 * This is a generated file.  Please modify 'configure.in'
 */

#ifndef __NOPOLL_CONFIG_H__
#define __NOPOLL_CONFIG_H__

/**
 * \addtogroup nopoll_decl_module
 * @{
 */

/**
 * @brief Allows to convert integer value (including constant values)
 * into a pointer representation.
 *
 * Use the oposite function to restore the value from a pointer to a
 * integer: \ref PTR_TO_INT.
 *
 * @param integer The integer value to cast to pointer.
 *
 * @return A \ref noPollPtr reference.
 */
#ifndef INT_TO_PTR
#define INT_TO_PTR(integer)   ((noPollPtr)  ((int)integer))
#endif

/**
 * @brief Allows to convert a pointer reference (\ref noPollPtr),
 * which stores an integer that was stored using \ref INT_TO_PTR.
 *
 * Use the oposite function to restore the pointer value stored in the
 * integer value.
 *
 * @param ptr The pointer to cast to a integer value.
 *
 * @return A int value.
 */
#ifndef PTR_TO_INT
#define PTR_TO_INT(ptr) ((int)  (ptr))
#endif

/**
 * @brief Allows to get current platform configuration. This is used
 * by Nopoll library but could be used by applications built on top of
 * Nopoll to change its configuration based on the platform information.
 */
#define NOPOLL_OS_UNIX (1)





/* @} */

#endif
