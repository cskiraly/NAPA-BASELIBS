dnl Process this file with autoconf to produce a configure script.
AC_INIT([GRAPES], [0.1], [tivadar.szemethy@netvisor.hu])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(tests/peer/peer.c)
AC_CONFIG_MACRO_DIR(m4)

AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_CXX
#AC_PROG_LIBTOOL
AC_PROG_RANLIB
AC_LANG_C


if test "$ac_cv_header_sys_types_h" == no
then
	AC_MSG_ERROR([sys/types.h is missing, unable to continue])
	exit -1
fi

if test "$ac_cv_header_stdbool_h" == no
then
	AC_MSG_ERROR([stdbool.h is missing, unable to continue])
	exit -1
fi


AC_ARG_WITH(libevent2, [  --with-libevent2=libevent2_dir use libevent2 in dir])
if test "$with_libevent2" == "yes"
then
	AC_MSG_ERROR([missing directory specification after --libevent2])
	exit -1
fi
if test -d "$with_libevent2"
then
	CPPFLAGS="$CPPFLAGS -I$with_libevent2/include"
	LDFLAGS="$LDFLAGS -L$with_libevent2/lib"
fi

AC_CHECK_HEADER(event2/util.h)
if test "$ac_cv_header_libevent_h" == no
then
	AC_MSG_ERROR([libevent2 seems to be missing, unable to continue])
	exit -1
fi

AC_CHECK_LIB(event, event_init)
if test $ac_cv_lib_event_event_init == no
then
	AC_MSG_ERROR([libevent2 seems to be missing, unable to continue])
	exit -1
fi

AC_ARG_WITH(libconfuse, [  --with-libconfuse=libconfuse_dir use libconfuse in dir])
if test "$with_libconfuse" == "yes"
then
	AC_MSG_ERROR([missing directory specification after --libconfuse])
	exit -1
fi

if test -d "$with_libconfuse"
then
	CPPFLAGS="$CPPFLAGS -I$with_libconfuse/include"
	LDFLAGS="$LDFLAGS -L$with_libconfuse/lib"
fi
AC_CHECK_HEADER(confuse.h)
if test "$ac_cv_header_libconfuse_h" == no
then
	AC_MSG_ERROR([libconfuse seems to be missing, unable to continue])
	exit -1
fi
	
AC_CHECK_LIB(confuse, cfg_init)
if test $ac_cv_lib_confuse_cfg_init == no
then
	AC_MSG_ERROR([libconfuse seems to be missing, unable to continue])
	exit -1
fi

AC_OUTPUT( [ Makefile \
	common/Makefile \
	dclog/Makefile \
	monl/Makefile rep/Makefile \
	tests/Makefile \
	tests/log/Makefile \
	tests/RepoClient/Makefile \
	tests/MLTest/Makefile \
	tests/MonTest/Makefile \
	tests/MonTestDist/Makefile
	tests/peer/Makefile \
	tests/neighborlist/Makefile \
	tests/Broadcaster/Makefile \
	tests/nvtest1/Makefile \
	tests/nvtest2/Makefile ] )

