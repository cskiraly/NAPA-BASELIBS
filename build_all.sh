#!/bin/bash
BUILD_ROOT_DIR=`dirname $0`
cd "$BUILD_ROOT_DIR"
BUILD_ROOT_DIR=`pwd`

#check the architecture
UNAME=`uname`
if [ "$UNAME" == "Linux" ]; then
   # do something Linux-y
   echo "Building for Linux"
elif [ "$UNAME" == "Darwin" ]; then
   # do something OSX-y
   echo "Building for OSX"
   CPPFLAGS="$CPPFLAGS -DMAC_OS"
   OBJ_CHECK=0
   [ "$STATIC"==2 ] && STATIC=1
fi

if [ -n "$ALL_DIR" ] ; then
  [ -e "$ALL_DIR" ] || { echo "Directory in \$ALL_DIR does not exist: $ALL_DIR"; exit 1; }  
  [ -z "$LIBEVENT_DIR" ] && LIBEVENT_DIR=$ALL_DIR/libevent
  [ -z "$LIBXML2_DIR" ] && LIBXML2_DIR=$ALL_DIR/libxml2;
fi


# Control the inclusion of 3rd party libraries
# <build_local_if_missing>: download and build if not found locally or in system
# <build_local> 	  : download and build if not found locally
# <rebuild_local> 	  : rebuild it locally
# <system>	          : must be installed in system libs/includes
# any_pathname	          : must be installed under specified pathname
#
# If built locally, it will be built under ${BUILD_ROOT_DIR}/3RDPARTY-LIBS

[ -n "$LIBEVENT_DIR" ] || LIBEVENT_DIR="<build_local>"
[ -n "$LIBXML2_DIR" ] || LIBXML2_DIR="<build_local>"

# leave empty or assign 0 to disable these features.
# or assign 1 or anything else to enable them. 
# IT IS RECOMMENDED THAT YOU SET THESE FROM THE COMMAND LINE!!!
[ -z "$DUMMY" ] && DUMMY=0
[ -z "$STATIC" ] && STATIC=1
[ -z "$ML" ] && ML=1
[ -z "$MONL" ] && MONL=1
[ -z "$ALTO" ] && ALTO=1
[ -z "$RTX" ] && RTX=1
[ -z "$FEC" ] && FEC=0

# Refresh base libraries if this is an svn checkout?
[ -n "$UPDATE_BASELIBS" ] || UPDATE_BASELIBS="" 
# Unconditionally clean and rebuild base libraries? 
[ -n "$REBUILD_BASELIBS" ] || REBUILD_BASELIBS=""

#which flex || { echo "please install flex!"; exit 1; }
#which libtoolize || { echo "please install libtool!"; exit 1; }
WGET_OR_CURL=`which wget`
[ -n "$WGET_OR_CURL" ] || { WGET_OR_CURL=`which curl` ; WGET_OR_CURLOPT="-L -O"; }
[ -n "$WGET_OR_CURL" ] || { echo "please install wget or curl!"; exit 1; }
which autoconf >/dev/null || { echo "please install autoconf!"; exit 1; }
#which autopoint || { echo "please install gettext (or autopoint) !"; exit 1; }

###############################################################################
####### Under normal conditions, the lines below should not be changed ########
###############################################################################

# option value of '0' is equivalent to empty
[ "$DUMMY" == "0" ] && DUMMY=
[ "$STATIC" == "0" ] && STATIC=
[ "$ML" == "0" ] && ML=
[ "$MONL" == "0" ] && MONL=
[ "$ALTO" == "0" ] && ALTO=
[ "$RTX" == "0" ] && RTX=
[ "$FEC" == "0" ] && FEC=

# if not empty, options are set to '1' (except for STATIC)
[ -n "$DUMMY" ] && DUMMY=0
[ -n "$ML" ] && ML=1
[ -n "$MONL" ] && MONL=1
[ -n "$ALTO" ] && ALTO=1

if [ -n "$RTX" ]; then
	CPPFLAGS="$CPPFLAGS -DRTX"
fi

if [ -n "$FEC" ]; then
	CPPFLAGS="$CPPFLAGS -DFEC"
fi

#Set the otpions to use with configure
[ -n "$CPPFLAGS" ] && CONF_CPPFLAGS="CPPFLAGS='$CPPFLAGS'"

if [ "$UNAME" == "Linux" ]; then
  MAKE="make -j `grep processor /proc/cpuinfo | wc -l`"
else
  MAKE="make -j 4"
fi

THIRDPARTY_DIR="${BUILD_ROOT_DIR}/3RDPARTY-LIBS"
[ -z "$DOWNLOAD_CACHE" ] && { DOWNLOAD_CACHE="${THIRDPARTY_DIR}/_download_cache"; mkdir -p $DOWNLOAD_CACHE; }
[ "$DOWNLOAD_CACHE" == "<no>" ] || [ -d "$DOWNLOAD_CACHE" -a -w "$DOWNLOAD_CACHE" ] || \
     { echo "ERROR: Download cache $DOWNLOAD_CACHE is not a writable directory"; exit 1; }

QUICK=false
REMOVE_OBJECTS=false
FAST_APP_BUILD=false

for ARG in $* ; do
  [ "$ARG" == "-q" ] && QUICK=true
  [ "$ARG" == "-r" ] && REMOVE_OBJECTS=true
  [ "$ARG" == "-f" ] && FAST_APP_BUILD=true
done

$REMOVE_OBJECTS && $FAST_APP_BUILD && echo "-r and -f can't be used together" && exit 1

cd "$BUILD_ROOT_DIR"
echo; echo; echo
if  ! $QUICK  ; then
echo "=================================================="
echo "Welcome to the NAPA-Wine applications build script"
echo "Options for this script:"
echo "     -q: quick_build (no user input after phases)"
echo "     -r: rebuild (clean NAPA, GRAPES and Streamer. 3rd party libs are not rebuilt)"
echo "         WARNING: if this fails, please check out a fresh copy from SVN and start again"
echo "     -f: fast (do not make clean in Applications. This may cause build to fail)"
echo
echo "WARNING: build will most probably fail after an svn update. Use -r. If even that fails"
echo "         download a new fresh copy from SVN!"
echo "=================================================="
echo
#echo Build is done in `pwd`
echo ">>> Press Enter to continue..."
read
fi

$REMOVE_OBJECTS && REBUILD_BASELIBS=true && REBUILD_GRAPES=true

get_system_includes() {
   DUMMY_FILE=`mktemp temp_file.XXXX`
   DUMMY_OUT=`mktemp temp_out.XXXX`
   ${CPP:-cpp} -v $DUMMY_FILE 2>$DUMMY_OUT;
   INC_DIRS1=`awk '/#include <...> search.*/, /^End.*/' <$DUMMY_OUT | grep -v '^#include\|^End'`
   GCC_INCLUDEPATH=""
   for D in $INC_DIRS1 ; do GCC_INCLUDEPATH="$GCC_INCLUDEPATH:$D" ; done;
   rm -f $DUMMY_FILE $DUMMY_OUT
   echo "Your system includes are: $GCC_INCLUDEPATH"
}


get_system_libs() {
   GCC_LIBPATH=`gcc -print-search-dirs  | grep "^libraries:" | sed -e 's/^libraries.*=//'`
   GCC_LIBPATH="$GCC_LIBPATH:/usr/local/lib"    
   echo "Your system libdirs are: $GCC_LIBPATH"
}

check_files_in_system_paths() {
   FILENAMES=$1

   FOUND_ALL=1   
   for FNAME in $FILENAMES ; do 
     echo $FNAME | grep -q '\.h$' && SPATH=$GCC_INCLUDEPATH
     echo $FNAME | grep -q '\.a$' && SPATH=$GCC_LIBPATH
     [ -z "$SPATH" ] && echo "Unknown file type to check: $FNAME (.a and .h supported)" && exit 1
     FOUND=
     for P in `tr ":" " " <<<$SPATH` ; do
       [ -n "$P" -a -e "$P/$FNAME" ] && FOUND=$P
     done
     if [ -z "$FOUND" ] ; then
          FOUND_ALL= 
          echo "$FNAME is not installed on your system"
     fi
   done
   [ -n "$FOUND_ALL" ]
} 
   
cache_or_wget() {
	URL=$1
		FILE=$2
		[ -z "$FILE" ] && FILE=`grep -o '[^/]*$' <<<"$URL"`
		if [ "$DOWNLOAD_CACHE" == "<no>" -o ! -e "$DOWNLOAD_CACHE/$FILE" ] ; then 
			$WGET_OR_CURL $WGET_OR_CURLOPT $URL || [ ! -e $FILE ] || { echo "ERROR: download of $URL failed"; exit 1; }
	[ "$DOWNLOAD_CACHE" == "<no>" ] || cp $FILE $DOWNLOAD_CACHE;
		else 
			cp $DOWNLOAD_CACHE/$FILE .
				fi
}
   
prepare_lib() {
    LIB_NAME="$1"
    LIB_DIR_VARNAME="$2"
    eval "LIB_DIR=\"\$$2\""
#echo $LIB_DIR_VARNAME is $LIB_DIR 
    TEST_FILES="$3"
    DOWNLOAD_CMD="$4"
    BUILD_CMD="$5"
    
    LIB_HOME="${THIRDPARTY_DIR}/${LIB_NAME}"
    if [ "$LIB_DIR" == '<rebuild_local>' ] ; then INSTALL_COMPONENT=true
    elif [ "$LIB_DIR" == '<system>' \
        -o "$LIB_DIR" == '<build_local>' \
	-o "$LIB_DIR" == '<build_local_if_missing>' ] ; then
       INSTALL_COMPONENT=true
       if [ "$LIB_DIR" == '<build_local>' -o "$LIB_DIR" == '<build_local_if_missing>' ] ; then
          for F in $TEST_FILES ; do
             INSTALL_COMPONENT=
             if [ ! -e "$LIB_HOME/include/$F" -a ! -e "$LIB_HOME/lib/$F" ] ; then
   	       INSTALL_COMPONENT=true  
             fi
             echo "$LIB_HOME/include/$F -o -e $LIB_HOME/lib/$F $INSTALL_COMPONENT" ; 
          done
       fi
       if [ -z "$INSTALL_COMPONENT" ]  ; then
          echo "INFO: Library $LIB_NAME is found locally. Will not be rebuilt"  
       elif [ "$LIB_DIR" != '<build_local>' ] && check_files_in_system_paths "$TEST_FILES" ; then
          echo "INFO: Library $LIB_NAME is deployed on your system"
          LIB_HOME=
          INSTALL_COMPONENT=
       elif [ "$LIB_DIR" == '<system>' ] ; then
           echo "ERROR: $LIB_NAME specified as '<system>' but not found on search path"
           exit 1
       else
          echo "INFO: Library $LIB_NAME will be built locally"
       fi
      
    else 
      INSTALL_COMPONENT=
      grep -q '^<' <<<$LIB_DIR && \
	 { echo "ERROR: Invalid location for $LIB_NAME: $LIB_DIR" ; exit 1;  }
      [ -e "$LIB_DIR" ] || \
	 { echo "ERROR: Nonexistent location for $LIB_NAME: $LIB_DIR"; exit 1; }
      LIB_HOME=$(readlink -f $LIB_DIR)
    fi
    if [ -n "$INSTALL_COMPONENT" ] ; then
       echo
       echo "==============================================="
       echo "Installing $LIB_NAME to $LIB_HOME"
       echo "==============================================="
       echo "INFO: Download command: $DOWNLOAD_CMD"
       echo "INFO: Build command: $BUILD_CMD"
       echo ">>> Press Enter to proceed..."
       $QUICK || read
       rm -rf $LIB_HOME

       echo "Downloading $LIB_NAME"  
       mkdir -p $LIB_HOME/_src; 
       pushd $LIB_HOME
       cd _src
       eval "$DOWNLOAD_CMD" || { echo "ERROR Downloading $LIB_NAME" ; exit 1; }
       cd ..

       echo "Building $LIB_NAME"  
       cd _src/*/.
       eval "$BUILD_CMD" || { echo "ERROR BUilding $LIB_NAME" ; exit 1; }
       popd
    fi      
    if [ -n "$LIB_HOME" ] ; then
       for F in $TEST_FILES ; do
          MISSING=
          if [ ! -e "$LIB_HOME/include/$F" -a ! -e "$LIB_HOME/lib/$F" ] ; then
            MISSING=true  
            echo "$F not found in $LIB_HOME" 
          fi
       done
       [ -n "$MISSING" ] && \
          { echo "ERROR: $LIB_NAME not found in $LIB_HOME"; exit 1; } 
       echo "INFO: $LIB_NAME is found in $LIB_HOME"
    fi
    eval "$LIB_DIR_VARNAME=$LIB_HOME"
}

prepare_libs() {
get_system_includes
get_system_libs

prepare_lib libevent LIBEVENT_DIR "event2/event.h libevent.a" \
        "cache_or_wget http://www.monkey.org/~provos/libevent-2.0.3-alpha.tar.gz; \
                  tar xvzf libevent-2.0.3-alpha.tar.gz" \
			 "./configure --prefix=\$LIB_HOME --libdir=\$LIB_HOME/lib ${HOSTARCH:+--host=$HOSTARCH};\
        $MAKE; make install" 

if [ -n "$ALTO" ] ; then
prepare_lib libxml2 LIBXML2_DIR "libxml2/libxml/xmlversion.h libxml2/libxml/xmlIO.h libxml2/libxml/parser.h libxml2/libxml/tree.h libxml2.a" \
        "cache_or_wget http://xmlsoft.org/sources/libxml2-2.7.6.tar.gz; \
              tar xvzf libxml2-2.7.6.tar.gz" \
        "./configure --with-python=no --with-threads --prefix=\$LIB_HOME --libdir=\$LIB_HOME/lib ${HOSTARCH:+--host=$HOSTARCH};\
		    $MAKE; make install" 
fi
}

prepare_libs 

grep -q "^<" <<<$LIBEVENT_DIR  && LIBEVENT_DIR=
grep -q "^<" <<<$LIBXML2_DIR  && LIBXML2_DIR=

echo "=============== YOUR 3RDPARTY DIRECTORIES ============"
echo "LIBEVENT_DIR=$LIBEVENT_DIR" 
echo "LIBXML2_DIR=$LIBXML2_DIR"

echo
echo ">>> Press enter continue with building NAPA-BASELIBS..."
$QUICK || read

export LIBEVENT_DIR LIBXML2_DIR

   EVOPT= ; [ -n LIBEVENT_DIR ] && EVOPT="--with-libevent2=$LIBEVENT_DIR"
   if [ -e .svn -a -n "$UPDATE_BASELIBS" ] ; then
      svn update
   fi
   if [ ! -e Makefile -o -n "$REBUILD_BASELIBS" ] ; then
     [ -e Makefile ] && $MAKE distclean
   mkdir -p m4 config
     autoreconf --force -I config -I m4 --install
     echo "./configure $EVOPT $CONFOPT $CONF_CPPFLAGS ${HOSTARCH:+--host=$HOSTARCH}"
     echo "./configure $EVOPT $CONFOPT $CONF_CPPFLAGS ${HOSTARCH:+--host=$HOSTARCH}" > conf.sh
     sh conf.sh
     echo "//blah" > common/chunk.c
   fi
   ALTO_NEEDED=`grep 1 <<<"$ALTO"`
   for SUBDIR in dclog common monl rep ${ALTO_NEEDED:+ALTOclient} ; do
       echo "Making `pwd`/$SUBDIR"
       $MAKE -C $SUBDIR || { echo "ERROR Building $SUBDIR"; exit 1; }
   done
   cd ml
      if [ ! -e Makefile -o -n "$REBUILD_BASELIBS" ] ; then
        mkdir -p m4 config
        autoreconf --force -I config -I m4 --install
     echo "./configure $EVOPT $CONFOPT $CONF_CPPFLAGS ${HOSTARCH:+--host=$HOSTARCH}"
     echo "./configure $EVOPT $CONFOPT $CONF_CPPFLAGS ${HOSTARCH:+--host=$HOSTARCH}" > conf.sh
     sh conf.sh
	$MAKE clean
      fi
      $MAKE || { echo "ERROR Building ml"; exit 1; }
   cd .. 

echo "=============== YOUR NAPA-BASELIBS ============"
find . -name '*.a' -exec ls -l '{}' ';'
echo
