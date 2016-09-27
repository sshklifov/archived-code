1. Introduction

I have uploaded the notable portion of programmes that I have written. They are divided into 2 categories. The first is 'html_css', where a website can be found. It was the end product of a group project from school. I was the leader and was responsible for designing it. The second section is 'POSIX-API'.

2. html_css

Download the whole website and open 'index.html'. From there you can browse around. Only the php page (obratna-vruzka.php) will not work, unless it is uploaded on a server which has enabled the 'mail' php function. It is a form which is automatically send to a gmail account. It makes use of Google's recaptcha against spam bots.

3. POSIX-API

Executables will only work on a POSIX comformant OS (tested only under Linux).

3.1 inotify_r

Makes use of the inotify API. It will recursively search for directories, starting with the current working one. Example usage:

mkdir dir
./inotify_r
touch file
mv file dir
rm dir/file
^C
rmdir dir

3.2 mem.c

I was inspired to implement my own malloc. You can check the source code of 'mem.c', though I have not done thorough debugging. I have included some supplementary programmes. The first, 'vpage', is a test to see how virtual memory works. It increases the programme break by 20 bytes and tries to access up to 4098 bytes (page size). It then attempts to write to the 4099th byte and, hopefully, is signalled with SIGSEGV. The other. 'malloc_defrag_test', is a result of the big difference in performance of my implementation of malloc and the glibc one. I had a feeling it was because malloc did not perform memory defragmentation, i.e. grouped adjacent free memory blocks to construct a bigger one. It turns out I was wrong.

3.3 time

A very simplistic programme that mirrors time(1). On the subject of time, I was interesting in the (x11) imlpementation of glfw's glfwGetTime(). I wrote the script 'glfwgettime.sh' (9 lines) and was amazed how fast I found it. I decided to include it here just as an example of why I love Linux.

3.4 cond_var

This programme is nothing special. It takes as command like arguments base ten numbers and creates a thread for each one that sleeps for that amount of seconds.

3.5 login

It tries to perform what login(1) does. It prompts for a username and password, verifies login credentials, writes to lastlog, umpt and wtmp files, changes uid and the current working directory, sets environemtal variables (e.g. 'HOME', 'USER') and fork-execs the user shell. It uses capabilities for reading and writing to files and, as a security mechanism, disables coredump files. Example usage:

sudo setcap "cap_dac_override=p" login
./login

3.6 popen.c

Implementation of popen(3).
