#!/bin/sh

rm -rf fd/
mkdir fd

echo
echo "**** Building boot loader ****"
cd start_up && sh build.sh && cd -

echo
echo "**** Building common ****"
cd Common && gmake clean all && cd -

echo
echo "**** Building Elf interpreter ****"
cd Elfo && gmake clean all && cd -

echo
echo "**** Building Memory Manager ****"
cd Memo && gmake clean all && cd -

ls -al fd/

echo
echo "**** Building NewFs ****"
cd newfs && gmake clean all && cd -

echo
echo "**** Building image ****"
cd images && sh build.sh && cd -

ls -al images/

if [ "$1" = "--with-docs" ]
then
    echo
    echo "**** Building documentation ****"
    cd wiki && sh build.sh && cd -
fi

