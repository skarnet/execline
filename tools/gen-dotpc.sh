#!/bin/sh -e

isunique () {
  x=$1
  set -- $2
  while test "$#" -gt 0 ; do
    if test "$x" = "$1" ; then
      return 1
    fi
    shift
  done
  return 0
}

uniqit () {
  res=
  while test "$#" -gt 0 ; do
    if isunique "$1" "$res" ; then
      res="${res}${res:+ }${1}"
    fi
    shift
  done
  printf %s\\n "$res"
}

filterout () {
  res=
  filter="$1"
  shift
  while test "$#" -gt 0 ; do
    if isunique "$1" "$filter" ; then
      res="${res}${res:+ }${1}"
    fi
    shift
  done
  printf %s\\n "$res"
}

splitlibs () {
  req=
  while test "$1" != -lskarnet ; do
    if test -z "$1" ; then
      echo "gen-dotpc.sh: fatal: can't happen: was called on a library that doesn't depend on libskarnet" 1>&2
      exit 102
    fi
  
  done
}

. package/info

ilist=
dlist=
slist=

if test "${includedir}" != /usr/include ; then
  ilist="-I${includedir}"
fi
if test -n "${buildtime_includedirs}" ; then
  ilist="${ilist}${ilist:+ }${buildtime_includedirs}"
fi
ilist=`uniqit ${ilist}`

if test "${dynlibdir}" != /usr/lib && test "${dynlibdir}" != /lib ; then
  dlist="-L${dynlibdir}"
fi
if test -n "${buildtime_dynlibdirs}" ; then
  dlist="${dlist}${dlist:+ }${buildtime_dynlibdirs}"
fi
dlist=`uniqit ${dlist}`

if test "${libdir}" != /usr/lib && test "${libdir}" != /lib ; then
 slist="-L${libdir}"
fi
if test -n "${buildtime_libdirs}" ; then
  slist="${slist}${slist:+ }${buildtime_libdirs}"
fi
slist="$(filterout "${dlist}" $(uniqit ${slist}))"

echo "prefix=${prefix}"
echo "includedir=${includedir}"
echo "libdir=${libdir}"
echo "dynlibdir=${dynlibdir}"
echo
echo "Name: ${library}"
echo "Version: ${version}"
echo "Description: ${description:-The ${library} library.}"
echo "URL: ${url:-https://skarnet.org/software/${package}/}"
if test -n "${requires}" ; then
  echo "Requires: ${requires}"
fi
if test -n "${requires_private}" ; then
  echo "Requires.private: ${requires_private}"
fi
if test -n "$ilist" ; then
  echo "Cflags: ${ilist}"
fi
echo "Libs: ${dlist}${dlist:+ }-l${library}${ldlibs:+ }${ldlibs}"
if test -n "${extra_libs}" ; then
  echo "Libs.private: ${slist}${slist:+ }${extra_libs}"
fi
