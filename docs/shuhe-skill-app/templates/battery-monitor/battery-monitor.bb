SUMMARY = "Battery monitor (sysfs/IIO/hwmon template)"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://battery-monitor.c \
    file://battery-monitor.service \
"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} -o battery-monitor ${WORKDIR}/battery-monitor.c
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/battery-monitor ${D}${bindir}/battery-monitor

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/battery-monitor.service \
        ${D}${systemd_system_unitdir}/battery-monitor.service
}

inherit systemd

SYSTEMD_SERVICE:${PN} = "battery-monitor.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

FILES:${PN} += "${systemd_system_unitdir}/battery-monitor.service"
