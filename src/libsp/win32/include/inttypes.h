#ifndef _INTTYPES_H
#define _INTTYPES_H	1

#ifndef __int8_t_defined
# define __int8_t_defined
typedef signed char		int8_t;
typedef short int		int16_t;
typedef int			int32_t;
typedef __int64 int64_t;
#endif

/* Unsigned.  */
typedef unsigned char		uint8_t;
typedef unsigned short int	uint16_t;
#ifndef __uint32_t_defined
typedef unsigned int		uint32_t;
# define __uint32_t_defined
#endif
typedef unsigned __int64	uint64_t;

#ifndef _UINTPTR_T_DEFINED
typedef unsigned long int	uintptr_t;
#endif

# define INT8_MIN		(-128)
# define INT16_MIN		(-32767-1)
# define INT32_MIN		(-2147483647-1)
# define INT64_MIN		(-__INT64_C(9223372036854775807)-1)
/* Maximum of signed integral types.  */
# define INT8_MAX		(127)
# define INT16_MAX		(32767)
# define INT32_MAX		(2147483647)
# define INT64_MAX		(__INT64_C(9223372036854775807))

/* Maximum of unsigned integral types.  */
# define UINT8_MAX		(255)
# define UINT16_MAX		(65535)
# define UINT32_MAX		(4294967295U)
# define UINT64_MAX		(__UINT64_C(18446744073709551615))

#endif /* inttypes.h */

#include "win32-stuff.h"