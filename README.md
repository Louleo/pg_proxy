Statement of originality:

This is to certify that to the best of our knowledge, the content of this project is our own
work. 

Yuntong Dai & Yanlin Liu


How to compile:
Put all the files in the comp3310-2016-pg_proxy and use "gcc" commands to complie and run the program.

Overview:
pg_proxy.c builds a simple proxy for postgreSQL server using psql client. It uses IPv4 to
recieve requests from psql clients and uses IPv6 to forward them to the real server and vice 
versa.

pg_proxy-f.c can do the same job with pg_proxy.c but use different methods. In pg_proxy.c, we implement 
select() method to achive the requirements. In pg_proxy-f.c, we implement fork() method as it is suggested.

pg_proxy2.c and pg_proxy2a are based on the previous one. The former one adds a filter
to control the access by rules specified on a configuration file. A user can use option: -c
followed by a file name to specify their own configuration or just use the default configuration.
The latter one use the database to specify the rules other than a configuration file.

Instructions:
Programming a series of proxy is a really long period. Yuntong and I try many methods and update 
numerous versions of the proxy. In the comp3310-2016-pg_proxy file folder, there 
are four versions proxies. Pg_proxy1.0 is named pg_proxy.c which implement select() method and 
can successfully provide the access to IPV6 database of all the IPV4 users. Pg_proxy1.5 is named 
pg_proxy-f.c which implement fork() method and can successfully work as pg_proxy1.0. The 1.5 version is
really important because all the last versions are based on this one. Pg_proxy2.0 is named pg_proxy2.c which 
can identify the user’s IP address should access the database or not. And we update Pg_proxy2.0 to pg_proxy2.0a
which contains more powerful functions than pg_proxy2.0.

Use gcc to compile the .c file and run the program. Some parameters in all the programs can be replaced by users.




