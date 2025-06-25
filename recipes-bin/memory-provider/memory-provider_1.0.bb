DESCRIPTION = "Simple memory provider"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://LICENSE;md5=9e7997fcb3ca1892479c156fe720975d"

SRC_URI = "file://main.c\
           file://LICENSE"

S = "${UNPACKDIR}"

do_compile() {
	${CC} ${LDFLAGS} main.c -o memory-provider
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 memory-provider ${D}${bindir}
}

RPROVIDES:${PN} += "memory-provider"