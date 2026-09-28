#pragma once
/**
 * Include early in engine TUs when building the Qt-free target.
 * Fails the build if a Qt header slipped into the translation unit.
 */
#if defined(KSENGINE_QT_FREE) && KSENGINE_QT_FREE
#  if defined(QT_VERSION) || defined(QT_CORE_LIB) || defined(QT_WIDGETS_LIB)
#    error "ksengine Qt-free build: Qt headers must not be included (QT_VERSION defined)"
#  endif
#  if defined(QObject) || defined(QString)
     /* QObject/QString as macros would be unusual; still flag common includes */
#  endif
#endif
