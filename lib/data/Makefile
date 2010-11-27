MKPATH=../../mk/
include $(MKPATH)buildsys.mk

PACKAGE = ${PACKAGE_NAME}/data
DATA = delete.me
DATA_ = artifact cost_adj ego_item flavor limits monster object p_class p_hist p_race set_item shop_own terrain vault

install-extra:
	if [ "x$(SETEGID)" != "x" ]; then \
		${MKDIR_P} ${DESTDIR}${datadir}/$(PACKAGE); \
		for i in ${DATA_}; do \
			touch ${DESTDIR}${datadir}/$(PACKAGE)/$${i}.raw; \
		done; \
		chown -R root:${SETEGID} ${DESTDIR}${datadir}/$(PACKAGE); \
		chmod -R g+w ${DESTDIR}${datadir}/$(PACKAGE); \
	fi
