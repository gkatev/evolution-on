echo "Running aclocal -I m4"
aclocal -I m4

echo "Running autoheader"
autoheader

echo "Running autoconf"
autoconf

echo "Running automake --gnu -a -c"
automake --gnu -a -c
