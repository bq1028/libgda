providerdir=$(libdir)/libgda-$(GDA_ABI_MAJOR_VERSION).$(GDA_ABI_MINOR_VERSION)/providers
provider_LTLIBRARIES = libgda-mysql.la

#Rem: MYSQL_CFLAGS and MYSQL_LIBS are the compile and link flags necessary to use the
# C API. It is specific to the API and should be computed in the configure.in script.

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libgda \
	-I$(top_builddir) \
	-I$(top_srcdir)/providers/reuseable/mysql \
	$(COREDEPS_CFLAGS) $(COREDEPS_WFLAGS) $(MYSQL_CFLAGS) -DCLASS_PREFIX=\""GdaMySQL"\"

gda-mysql-handler-bin.c: $(top_srcdir)/libgda/sqlite/gda-sqlite-handler-bin.c
	cat $(top_srcdir)/libgda/sqlite/gda-sqlite-handler-bin.c | sed -e 's/SQLITE/MYSQL/g' -e 's/sqlite/mysql/g' -e 's/Sqlite/Mysql/g' -e 's/SQLite/MySQL/g' > gda-mysql-handler-bin.c

gda-mysql-handler-bin.h: $(top_srcdir)/libgda/sqlite/gda-sqlite-handler-bin.h
	cat $(top_srcdir)/libgda/sqlite/gda-sqlite-handler-bin.h | sed -e 's/SQLITE/MYSQL/g' -e 's/sqlite/mysql/g' -e 's/Sqlite/Mysql/g' -e 's/SQLite/MySQL/g' > gda-mysql-handler-bin.h

genfiles = gda-mysql-handler-bin.c gda-mysql-handler-bin.h

$(libgda_mysql_la_OBJECTS): $(genfiles)

libgda_mysql_la_SOURCES = \
	gda-mysql-blob-op.c \
	gda-mysql-blob-op.h \
	gda-mysql-ddl.c \
	gda-mysql-ddl.h \
	gda-mysql-handler-boolean.c \
	gda-mysql-handler-boolean.h \
	$(genfiles) \
	gda-mysql-provider.c \
	gda-mysql-provider.h \
	gda-mysql-recordset.c \
	gda-mysql-recordset.h \
	gda-mysql-util.c \
	gda-mysql-util.h \
	gda-mysql-pstmt.c \
	gda-mysql-pstmt.h \
	gda-mysql.h \
	mysql.gresources.c \
	libmain.c

libgda_mysql_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED) $(LIBTOOL_PROV_EXPORT_OPTIONS)
libgda_mysql_la_LIBADD = \
	$(top_builddir)/libgda/libgda-6.0.la \
	$(top_builddir)/providers/reuseable/mysql/libgda-mysql.la \
	$(COREDEPS_LIBS) $(MYSQL_LIBS)

xmldir   = $(datadir)/libgda-6.0
xml_in_files = \
	mysql_specs_create_db.xml.in \
	mysql_specs_drop_db.xml.in \
	mysql_specs_dsn.xml.in \
	mysql_specs_create_table.xml.in \
        mysql_specs_drop_table.xml.in \
        mysql_specs_comment_table.xml.in \
        mysql_specs_create_index.xml.in \
        mysql_specs_drop_index.xml.in \
	mysql_specs_rename_table.xml.in \
	mysql_specs_add_column.xml.in \
	mysql_specs_drop_column.xml.in \
	mysql_specs_comment_column.xml.in \
	mysql_specs_create_view.xml.in \
	mysql_specs_drop_view.xml.in

@INTLTOOL_XML_RULE@

xml_DATA = $(xml_in_files:.xml.in=.xml)

# resources
RESOURCES=$(xml_in_files:.xml.in=.raw.xml)
mysql.gresources.c: mysql.gresource.xml $(RESOURCES)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --generate-source $(srcdir)/mysql.gresource.xml

%.raw.xml:%.xml.in
	@echo "  GEN      $@"
	@cat $< | sed -e 's/_\([a-zA-Z0-9]*\)=/\1=/g' -e 's/<_\([a-zA-Z0-9_]*\)>/<\1>/g' -e 's/<\/_\([a-zA-Z0-9_]*\)>/<\/\1>/g' -e 's/<!-- .* -->//'> $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgda-mysql-6.0.pc

EXTRA_DIST = $(xml_in_files) libgda-mysql-6.0.pc.in
DISTCLEANFILES = $(xml_DATA) $(genfiles)

CLEANFILES = \
	$(genfiles) \
	mysql.gresources.c \
	$(RESOURCES)
