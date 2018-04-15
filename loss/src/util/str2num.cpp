#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "str2num.h"

namespace loss
{
    /* Avoid warnings on solaris, where isspace() is an index into an array, and
     * gcc uses signed chars */
    #define xisspace(c) isspace((unsigned char)c)
   
    bool SafeStrToNum::StrToULL(const char *str, uint64_t *out)
    {
        if (out == NULL)
            return false;
        errno = 0;
        *out = 0;
        char *endptr = NULL;
        unsigned long long uLL = strtoull(str, &endptr, 10);
        if ((errno == ERANGE) || (str == endptr))
        {
            return false;
        }
        if (xisspace(*endptr) || (*endptr == '\0' && endptr != str))
        {
            if ((long long) uLL < 0)
            {
                /* only check for negative signs in the uncommon
                 case when
                 the unsigned number is so big that
                 it's negative as a
                 signed number. */
                if (strchr(str, '-') != NULL) 
                {
                    return false;
                }
            }
            *out = uLL;
            return true;
        }
        return false;
    }
    bool SafeStrToNum::StrToLL(const char *str, int64_t *out)
    {
        if (out == NULL)
            return false;
        errno = 0;
        char *endptr = NULL;
        long long LL = strtoll(str, &endptr, 10);
        if ((errno == ERANGE)  || (str == endptr))
        {
            return false;
        }
        if (xisspace(*endptr) || (*endptr == '\0' && endptr != str))
        {
            *out = LL;
            return true;
        }
        return false;
    }

    bool SafeStrToNum::StrToUL(const char *str, uint32_t *out)
    {
        char *endptr = NULL;
        unsigned long uL = 0;
        if (out == NULL || str == NULL)
        {
            return false;
        }
        *out = 0;
        errno = 0;
        uL = strtoul(str, &endptr, 10);
        if ((errno == ERANGE) || (str == endptr))
        {
            return false;
        }
        if (xisspace(*endptr) || (*endptr == '\0' &&endptr != str))
        {
            if ((long) uL < 0) 
            {
				/* only check for negative signs in the uncommon case when
				 * the unsigned number is so big that it's negative as a
				 * signed number. */
                if (strchr(str, '-') != NULL)
                {
                    return false;
                }
            }
			*out = uL;
			return true;
        }
        return false;
    }

    bool SafeStrToNum::StrToL(const char *str, int32_t *out)
    {
        if (out == NULL)
            return false;
        errno =0;
        *out = 0;
        char *endptr = NULL;
        long lV = strtol(str, &endptr, 10);
        if ((errno == ERANGE) || (str == endptr))
        {
            return false;
        }
        if (xisspace(*endptr) || (*endptr == '\0' && endptr != str))
        {
            *out = lV;
            return true;
        }
        return false;
    }

    bool SafeStrToNum::StrToD(const char *str, double *out)
    {
        if (out == NULL)
            return false;
        errno = 0;
        *out = 0.0;
        char *endptr = NULL;
        double dV = strtod(str, &endptr);
        if ((errno == ERANGE) || (str == endptr))
        {
            return false;
        }
        if (xisspace(*endptr) || (*endptr == '\0' && endptr != str))
        {
            *out = dV;
            return true;
        }
        return false;
    }

    bool SafeStrToNum::StrToF(const char* str, float *out)
    {
        if (out == NULL)
            return false;
        errno = 0;
        *out = 0.0;
        char *endptr = NULL;
        float  fV = strtof(str, &endptr);
        if ((errno == ERANGE) || (str == endptr))
        {
            return false;
        }
        if (xisspace(*endptr) || (*endptr == '\0' && endptr != str))
        {
            *out = fV;
            return true;
        }
        return false;
    }
}
