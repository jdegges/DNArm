all: ./build/cgmtest ./build/cgmtest_image ./build/cgmtest_cached

./build/cgmtest: cgmtest.c cgmcl.cl cgmcl2.cl
	 gcc -o ./build/cgmtest ${CPPFLAGS} ${LDFLAGS} -I. cgmtest.c -lOpenCL

./build/cgmtest_image: cgmtest_image.c cgmcl_image.cl
	 gcc -o ./build/cgmtest_image ${CPPFLAGS} ${LDFLAGS} -I. cgmtest_image.c -lOpenCL

./build/cgmtest_cached: cgmtest_cached.c cgmcl_cached.cl
	 gcc -o ./build/cgmtest_cached ${CPPFLAGS} ${LDFLAGS} -I. cgmtest_cached.c -lOpenCL

