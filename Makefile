all: pam_8021x

pam_8021x: nm8021xconfig.c pam_8021x.c
	gcc -fPIC -shared `pkg-config --cflags --libs gio-2.0 libnm-gtk libnm-glib` -lpam ./nm8021xconfig.c ./pam_8021x.c -o pam_8021x.so
	gcc `pkg-config --cflags --libs gio-2.0 libnm-gtk libnm-glib` ./nm8021xconfig.c -o nm8021xtest

install:
	cp pam_8021x.so /lib64/security/pam_8021x.so
uninstall:
	rm /lib64/security/pam_8021x.so
clean:
	rm pam_8021x.so nm8021xtest
