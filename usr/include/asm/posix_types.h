# ifdef __i386__
#  include "posix_types_32.h"
# elif defined(__ILP32__)
#  include "posix_types_x32.h"
# else
#  include "posix_types_64.h"
# endif
