/**
 * @file libsamrender_global.h
 * SAM renderer library global defines
 * @author Michelle Daniels
 * @copyright UCSD 2014
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#ifndef LIBSAMRENDER_GLOBAL_H
#define LIBSAMRENDER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(LIBSAMRENDER_LIBRARY)
#  define LIBSAMRENDERSHARED_EXPORT Q_DECL_EXPORT
#else
#  define LIBSAMRENDERSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // LIBSAMRENDER_GLOBAL_H
