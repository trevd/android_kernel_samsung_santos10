cmd_/android/build/android-cm-10/kernel/samsung/santos10/usr/include/linux/wimax/.check := for f in i2400m.h   ; do echo "/android/build/android-cm-10/kernel/samsung/santos10/usr/include/linux/wimax/$${f}"; done | xargs perl scripts/headers_check.pl /android/build/android-cm-10/kernel/samsung/santos10/usr/include x86; touch /android/build/android-cm-10/kernel/samsung/santos10/usr/include/linux/wimax/.check