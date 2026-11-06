SUMMARY = "Customer fullscreen SDL2 display"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "libsdl2"

SRC_URI = " \
    file://customer-display.c \
    file://customer-display.sh \
    file://customer-display.service \
"

S = "${WORKDIR}"

inherit pkgconfig systemd

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} -o customer-display \
        ${WORKDIR}/customer-display.c \
        $(pkg-config --cflags --libs sdl2)
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/customer-display ${D}${bindir}/customer-display
    install -m 0755 ${WORKDIR}/customer-display.sh ${D}${bindir}/customer-display.sh

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/customer-display.service \
        ${D}${systemd_system_unitdir}/customer-display.service
}

# Optional: install the customer's image as /opt/logo.bmp.
# Uncomment and add `file://logo.bmp` to SRC_URI.
# do_install:append() {
#     install -d ${D}/opt
#     install -m 0644 ${WORKDIR}/logo.bmp ${D}/opt/logo.bmp
# }
# FILES:${PN} += "/opt/logo.bmp"

SYSTEMD_SERVICE:${PN} = "customer-display.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

# st-image-weston usually already provides SDL2 via opengl packagegroup;
# keep RDEPENDS as a safety for custom images.
RDEPENDS:${PN} = "libsdl2"

FILES:${PN} += " \
    ${bindir}/customer-display.sh \
    ${systemd_system_unitdir}/customer-display.service \
"
