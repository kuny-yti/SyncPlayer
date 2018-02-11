#ifndef EXPORT
#define EXPORT

#  ifdef WIN32
#    define LIB_SHARED_EXPORT     __declspec(dllexport)
#    define LIB_SHARED_IMPORT     __declspec(dllimport)
#  else
#    define LIB_SHARED_EXPORT     __attribute__((visibility("default")))
#    define LIB_SHARED_IMPORT     __attribute__((visibility("default")))
#    define LIB_SHARED_HIDDEN     __attribute__((visibility("hidden")))
#  endif

#ifdef EXPORT_STATIC
#define LIBEXPORT
#else
#ifdef EXPORT_SHARED
#define LIBEXPORT LIB_SHARED_EXPORT
#else
#define LIBEXPORT LIB_SHARED_IMPORT
#endif
#endif

#endif // EXPORT

