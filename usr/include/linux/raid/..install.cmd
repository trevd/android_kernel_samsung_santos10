cmd_/android/build/android-cm-10/kernel/samsung/santos10/usr/include/linux/raid/.install := perl scripts/headers_install.pl /android/build/android-cm-10/kernel/samsung/santos10/include/linux/raid /android/build/android-cm-10/kernel/samsung/santos10/usr/include/linux/raid x86 md_p.h md_u.h; perl scripts/headers_install.pl /android/build/android-cm-10/kernel/samsung/santos10/include/linux/raid /android/build/android-cm-10/kernel/samsung/santos10/usr/include/linux/raid x86 ; perl scripts/headers_install.pl /android/build/android-cm-10/kernel/samsung/santos10/include/generated/linux/raid /android/build/android-cm-10/kernel/samsung/santos10/usr/include/linux/raid x86 ; for F in ; do echo "\#include <asm-generic/$$F>" > /android/build/android-cm-10/kernel/samsung/santos10/usr/include/linux/raid/$$F; done; touch /android/build/android-cm-10/kernel/samsung/santos10/usr/include/linux/raid/.install