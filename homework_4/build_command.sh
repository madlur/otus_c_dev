#download by wget curl-8.6.0
#unpack tar
#cd to curl directory

./configure    \
 --disable-ftp \
 --disable-file \
 --disable-ldap \
 --disable-ldaps \
 --disable-rtsp \
 --disable-dict \
 --disable-tftp \
 --disable-gopher \
 --disable-imap \
 --disable-pop3 \
 --disable-smtp \
 --disable-scp \
 --disable-sftp \
 --disable-mqtt  \
 --disable-rtsp \
 --disable-smb \
 --disable-smbs \
 --disable-manual \
 --with-openssl \
 --enable-http \
 --enable-https

 make -j$(nproc)