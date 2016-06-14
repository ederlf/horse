#/bin/bash

# Redis
wget http://download.redis.io/redis-stable.tar.gz
tar xvzf redis-stable.tar.gz
rm redis-stable.tar.gz && cd redis-stable
make
sudo make install
sudo mkdir /etc/redis
sudo mkdir /var/redis
# Same init.d script
sudo cp utils/redis_init_script /etc/init.d/redis_6379
# Modified redis.conf
sudo cp ~/Horse/setup/redis.conf /etc/redis/6379.conf
sudo mkdir /var/redis/6379
sudo update-rc.d redis_6379 defaults
sudo /etc/init.d/redis_6379 start

#hiredis
sudo apt-get install libhiredis0.10 libhiredis-dev
