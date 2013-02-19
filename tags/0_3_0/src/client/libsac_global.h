/**
 * @file libsac_global.h
 * SAM client library global defines
 * @author Michelle Daniels
 * @copyright UCSD 2012
 */

#ifndef LIBSAC_GLOBAL_H
#define LIBSAC_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(LIBSAC_LIBRARY)
#  define LIBSACSHARED_EXPORT Q_DECL_EXPORT
#else
#  define LIBSACSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // LIBSAC_GLOBAL_H
