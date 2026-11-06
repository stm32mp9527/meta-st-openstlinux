SUMMARY = "Customer TCP server"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://tcp-server.c \
    file://tcp-server.service \
"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} -o tcp-server ${WORKDIR}/tcp-server.c
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/tcp-server ${D}${bindir}/tcp-server

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/tcp-server.service ${D}${systemd_system_unitdir}/tcp-server.service
}

inherit systemd

SYSTEMD_SERVICE:${PN} = "tcp-server.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

FILES:${PN} += "${systemd_system_unitdir}/tcp-server.service"
