inherit upkg_base

LICENSE = "PSF-2.0 & BSD-3-Clause & MIT-variant & Zlib & ISC & MIT"
LICENSE_libpython3.8 = "PSF-2.0 & BSD-3-Clause & MIT-variant & Zlib & ISC"
LICENSE_libpython3.8-dev = "PSF-2.0 & BSD-3-Clause & MIT-variant & Zlib & ISC"
LICENSE_libpython3.8-minimal = "PSF-2.0 & BSD-3-Clause & MIT-variant & Zlib & ISC"
LICENSE_libpython3.8-testsuite = "PSF-2.0 & BSD-3-Clause & MIT-variant & Zlib & ISC"
LICENSE_libpython3.8-stdlib = "PSF-2.0 & BSD-3-Clause & MIT-variant & Zlib & ISC"
LICENSE_python3.8-dev = "PSF-2.0 & BSD-3-Clause & MIT-variant & Zlib & ISC"
LICENSE_python3.8-minimal = "PSF-2.0 & BSD-3-Clause & MIT-variant & Zlib & ISC"
LICENSE_python3.8-full = "PSF-2.0 & BSD-3-Clause & MIT-variant & Zlib & ISC"
LICENSE_python3.8-venv = "PSF-2.0 & BSD-3-Clause & MIT-variant & Zlib & ISC"
LICENSE_python3.8-examples = "PSF-2.0 & BSD-3-Clause & MIT-variant & Zlib & ISC"
LICENSE_pypy3-lib = "MIT"
LICENSE_pypy3-dev = "MIT"
LICENSE_python3-minimal = "PSF-2.0 & MIT"

# the information of ubuntu package(s)
SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/main/p/python3.8/libpython3.8_3.8.2-1ubuntu1_arm64.deb;name=libpython3.8"
SRC_URI[libpython3.8.md5sum] = "10481d8686541dd95a7d3bfeccff1faa"

SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/main/p/python3.8/libpython3.8-dev_3.8.2-1ubuntu1_arm64.deb;name=libpython3.8-dev"
SRC_URI[libpython3.8-dev.md5sum] = "8a620c70f651479f7097b4946d1f25c6"

SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/main/p/python3.8/libpython3.8-minimal_3.8.2-1ubuntu1_arm64.deb;name=libpython3.8-minimal"
SRC_URI[libpython3.8-minimal.md5sum] = "d4eff1767a354b1e192a7265f86918cc"

SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/universe/p/python3.8/libpython3.8-testsuite_3.8.10-0ubuntu1~20.04.12_all.deb;name=libpython3.8-testsuite"
SRC_URI[libpython3.8-testsuite.sha256sum] = "3a5373187e1addfc48169b0d930703fff7f6e9885ee3a6ea9af07a9cec6168b3"

SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/main/p/python3.8/python3.8-dev_3.8.2-1ubuntu1_arm64.deb;name=python3.8-dev"
SRC_URI[python3.8-dev.md5sum] = "4ac92fb8a6b942d1c795ab80ef8a8db9"

SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/main/p/python3.8/libpython3.8-stdlib_3.8.2-1ubuntu1_arm64.deb;name=libpython3.8-stdlib"
SRC_URI[libpython3.8-stdlib.md5sum] = "663a4a5f8c1fc19684da2512c70b8c41"

SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/main/p/python3.8/python3.8-minimal_3.8.2-1ubuntu1_arm64.deb;name=python3.8-minimal"
SRC_URI[python3.8-minimal.md5sum] = "be8239e6243369ac02468408768debf5"

SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/universe/p/python3.8/python3.8-full_3.8.10-0ubuntu1~20.04.12_arm64.deb;name=python3.8-full"
SRC_URI[python3.8-full.sha256sum] = "ae994c0475ae2a6ccd910088e56df4d6f37514296c1a1bc36b09af101bad0b3c"

SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/universe/p/python3.8/python3.8-venv_3.8.2-1ubuntu1_arm64.deb;name=python3.8-venv"
SRC_URI[python3.8-venv.md5sum] = "5bd74a5e05f011f3a233b7d5350f5135"

SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/main/p/python3.8/python3.8-venv_3.8.2-1ubuntu1_arm64.deb;name=python3.8-examples"
SRC_URI[python3.8-examples.md5sum] = "5bd74a5e05f011f3a233b7d5350f5135"

SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/universe/p/pypy3/pypy3-lib_7.3.1+dfsg-4_arm64.deb;name=pypy3-lib"
SRC_URI[pypy3-lib.md5sum] = "cb24970348393f06cdca3726322ff398"

SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/universe/p/pypy3/pypy3-dev_7.3.1+dfsg-4_all.deb;name=pypy3-dev"
SRC_URI[pypy3-dev.md5sum] = "0d45f49a34297f4c3e9f1aa3dc68533b"

SRC_URI += "http://ports.ubuntu.com/ubuntu-ports/pool/main/p/python3-defaults/python3-minimal_3.8.2-0ubuntu2_arm64.deb;name=python3-minimal"
SRC_URI[python3-minimal.md5sum] = "0478297efb6cfa63984502a8bfdf3444"

# other configs to feed compilation
PKG_${UPN} = "python3.8"
DEPENDS += "libtool-cross  virtual/aarch64-linux-gnu-gcc virtual/aarch64-linux-gnu-compilerlibs virtual/libc libffi bzip2 openssl sqlite3 zlib virtual/libintl xz virtual/crypt util-linux libtirpc libnsl2 autoconf-archive libffi gdbm readline virtual/update-alternatives"
RPROVIDES_python3 +="python3-core python3-src python3-dbg python3-tests python3-2to3 python3-asyncio python3-audio python3-codecs \
python3-compile python3-compression python3-crypt python3-ctypes python3-curses python3-datetime python3-db \
python3-debugger python3-difflib python3-distutils-windows python3-distutils python3-doctest python3-email python3-fcntl \
python3-gdbm python3-html python3-idle python3-image python3-io python3-json python3-logging python3-mailbox python3-math \
python3-mime python3-mmap python3-modules python3-multiprocessing python3-netclient python3-netserver python3-numbers \
python3-pickle python3-pkgutil python3-plistlib python3-pprint python3-profile python3-pydoc python3-resource python3-shell \
python3-smtpd python3-sqlite3 python3-stringold python3-syslog python3-terminal python3-threading python3-tkinter \
python3-typing python3-unittest python3-unixadmin python3-venv python3-xml python3-xmlrpc libpython3 libpython3-staticdev \
python3-staticdev python3-dev python3-doc python3-locale python3 python3-misc python3-man libpython3.8-1.0 libpython3.8-staticdev"
PROVIDES += "python3-core python3-src python3-dbg python3-tests python3-2to3 python3-asyncio python3-audio python3-codecs \
python3-compile python3-compression python3-crypt python3-ctypes python3-curses python3-datetime python3-db \
python3-debugger python3-difflib python3-distutils-windows python3-distutils python3-doctest python3-email python3-fcntl \
python3-gdbm python3-html python3-idle python3-image python3-io python3-json python3-logging python3-mailbox python3-math \
python3-mime python3-mmap python3-modules python3-multiprocessing python3-netclient python3-netserver python3-numbers \
python3-pickle python3-pkgutil python3-plistlib python3-pprint python3-profile python3-pydoc python3-resource python3-shell \
python3-smtpd python3-sqlite3 python3-stringold python3-syslog python3-terminal python3-threading python3-tkinter \
python3-typing python3-unittest python3-unixadmin python3-venv python3-xml python3-xmlrpc libpython3 libpython3-staticdev \
python3-staticdev python3-dev python3-doc python3-locale python3 python3-misc python3-man libpython3.8-1.0 libpython3.8-staticdev"


do_install_append() {
        mkdir -p ${D}${libdir}/python-sysconfigdata
        sysconfigfile=`find ${D} -name _sysconfig*.py`
        cp $sysconfigfile ${D}${libdir}/python-sysconfigdata/

        sed -i  \
                -e "s,^ 'LIBDIR'.*, 'LIBDIR': '${STAGING_LIBDIR}'\,,g" \
                -e "s,^ 'INCLUDEDIR'.*, 'INCLUDEDIR': '${STAGING_INCDIR}'\,,g" \
                -e "s,^ 'CONFINCLUDEDIR'.*, 'CONFINCLUDEDIR': '${STAGING_INCDIR}'\,,g" \
                -e "/^ 'INCLDIRSTOMAKE'/{N; s,/usr/include,${STAGING_INCDIR},g}" \
                -e "/^ 'INCLUDEPY'/s,/usr/include,${STAGING_INCDIR},g" \
                ${D}${libdir}/python-sysconfigdata/_sysconfigdata.py
}

PACKAGES += "python3-core python3-dbg python3-tests python3-2to3 python3-asyncio python3-audio python3-codecs \
python3-compile python3-compression python3-crypt python3-ctypes python3-curses python3-datetime python3-db \
python3-debugger python3-difflib python3-distutils-windows python3-distutils python3-doctest python3-email python3-fcntl \
python3-gdbm python3-html python3-idle python3-image python3-io python3-json python3-logging python3-mailbox python3-math \
python3-mime python3-mmap python3-modules python3-multiprocessing python3-netclient python3-netserver python3-numbers \
python3-pickle python3-pkgutil python3-plistlib python3-pprint python3-profile python3-pydoc python3-resource python3-shell \
python3-smtpd python3-sqlite3 python3-stringold python3-syslog python3-terminal python3-threading python3-tkinter \
python3-typing python3-unittest python3-unixadmin python3-venv python3-xml python3-xmlrpc libpython3 libpython3-staticdev \
python3-misc python3-man libpython3.8-1.0 libpython3.8-staticdev"


ALLOW_EMPTY_python3-dbg = "1"
ALLOW_EMPTY_python3-tests = "1"
ALLOW_EMPTY_python3-2to3 = "1"
ALLOW_EMPTY_python3-asyncio = "1"
ALLOW_EMPTY_python3-audio = "1"
ALLOW_EMPTY_python3-codecs = "1"
ALLOW_EMPTY_python3-compile = "1"
ALLOW_EMPTY_python3-compression = "1"
ALLOW_EMPTY_python3-core = "1"
ALLOW_EMPTY_python3-crypt = "1"
ALLOW_EMPTY_python3-ctypes = "1"
ALLOW_EMPTY_python3-curses = "1"
ALLOW_EMPTY_python3-datetime = "1"
ALLOW_EMPTY_python3-db = "1"
ALLOW_EMPTY_python3-debugger = "1"
ALLOW_EMPTY_python3-difflib = "1"
ALLOW_EMPTY_python3-distutils-windows = "1"
ALLOW_EMPTY_python3-distutils = "1"
ALLOW_EMPTY_python3-doctest = "1"
ALLOW_EMPTY_python3-email = "1"
ALLOW_EMPTY_python3-fcntl = "1"
ALLOW_EMPTY_python3-gdbm = "1"
ALLOW_EMPTY_python3-html = "1"
ALLOW_EMPTY_python3-idle = "1"
ALLOW_EMPTY_python3-image = "1"
ALLOW_EMPTY_python3-io = "1"
ALLOW_EMPTY_python3-json = "1"
ALLOW_EMPTY_python3-logging = "1"
ALLOW_EMPTY_python3-mailbox = "1"
ALLOW_EMPTY_python3-math = "1"
ALLOW_EMPTY_python3-mime = "1"
ALLOW_EMPTY_python3-mmap = "1"
ALLOW_EMPTY_python3-modules = "1"
ALLOW_EMPTY_python3-multiprocessing = "1"
ALLOW_EMPTY_python3-netclient = "1"
ALLOW_EMPTY_python3-netserver = "1"
ALLOW_EMPTY_python3-numbers = "1"
ALLOW_EMPTY_python3-pickle = "1"
ALLOW_EMPTY_python3-pkgutil = "1"
ALLOW_EMPTY_python3-plistlib = "1"
ALLOW_EMPTY_python3-pprint = "1"
ALLOW_EMPTY_python3-profile = "1"
ALLOW_EMPTY_python3-pydoc = "1"
ALLOW_EMPTY_python3-resource = "1"
ALLOW_EMPTY_python3-shell = "1"
ALLOW_EMPTY_python3-smtpd = "1"
ALLOW_EMPTY_python3-sqlite3 = "1"
ALLOW_EMPTY_python3-stringold = "1"
ALLOW_EMPTY_python3-syslog = "1"
ALLOW_EMPTY_python3-terminal = "1"
ALLOW_EMPTY_python3-threading = "1"
ALLOW_EMPTY_python3-tkinter = "1"
ALLOW_EMPTY_python3-typing = "1"
ALLOW_EMPTY_python3-unittest = "1"
ALLOW_EMPTY_python3-unixadmin = "1"
ALLOW_EMPTY_python3-venv = "1"
ALLOW_EMPTY_python3-xml = "1"
ALLOW_EMPTY_python3-xmlrpc = "1"
ALLOW_EMPTY_python3-misc = "1"
ALLOW_EMPTY_python3-man= "1"
ALLOW_EMPTY_libpython3 = "1"
ALLOW_EMPTY_libpython3-staticdev = "1"
ALLOW_EMPTY_libpython3.8-1.0 = "1"
ALLOW_EMPTY_libpython3.8-staticdev = "1"
