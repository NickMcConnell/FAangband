MKPATH=../../../mk/
include $(MKPATH)buildsys.mk

DATA = bone.020 bone.025 bone.030 bone.035 bone.040 bone.045 bone.050 bone.055 \
 bone.060 bone.065 bone.070 bone.075 bone.080 bone.085 bone.090 bone.095
PACKAGE = bone

install-extra:
	if [ "x$(SETEGID)" != "x" ]; then \
		if [ "x$(DRY)" = "x" ]; then \
			${MKDIR_P} ${DESTDIR}${varshareddatadir}${PACKAGE}; \
			chown root:${SETEGID} ${DESTDIR}${varshareddatadir}${PACKAGE}; \
			chmod 171 ${DESTDIR}${varshareddatadir}${PACKAGE}; \
		fi; \
	fi
