cmd_/android/build/android-cm-10/kernel/samsung/santos10/usr/include/linux/mmc/.check := for f in ioctl.h   ; do echo "/android/build/android-cm-10/kernel/samsung/santos10/usr/include/linux/mmc/$${f}"; done | xargs perl scripts/headers_check.pl /android/build/android-cm-10/kernel/samsung/santos10/usr/include x86; touch /android/build/android-cm-10/kernel/samsung/santos10/usr/include/linux/mmc/.check