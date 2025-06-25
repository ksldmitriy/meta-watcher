SUMMARY = "hardware watchpoint using arch_hw_breakpoint api"
DESCRIPTION = "${SUMMARY}"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://LICENSE;md5=9e7997fcb3ca1892479c156fe720975d"

inherit module

SRC_URI = "file://Makefile \
           file://watcher.c \
           file://LICENSE\
          "

S = "${UNPACKDIR}"

RPROVIDES:${PN} += "kernel-module-watcher"
