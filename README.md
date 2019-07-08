## Web Server / Web Crawler

This is my last assignment in System Programming course. It implements a web server-client relationship using **GET** requests and is divided in 3 parts explained in more detail below. The exercise had restrictions mainly about used space and cleaning afterwards, so every data structure and string is dynamically allocated and the code was tested with valgrind for memory leaks etc.

### Compile

- make: compiles all source files
- make clean: cleans directory by deleting all object files and executables
- make count: count lines, words, letters

**Important**: before setting up and running the server, the websites must be created with the use of webcreator (see below).</br>

### Website creator 
./webcreator.sh root_directory text_file w p</br>

A simple bash script that given a specific text file, creates **w** directories (sites) with **p** html files (pages), which all link to each other randomly. These are the files that server will be serving.</br>

### Web server
./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir</br>

A server listening to the specified serving port for GET request and answering with 200, 403, 404, 500 responses. For each new request, selects from the thread pool a new thread to handle it. The command port is for answering to specific commands, like STATS (stats about the process) and SHUTDOWN (for terminating the process).</br>

### Web crawler
./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL</br>

A web crawler simulating a client that given a starting webpage and by accessing all of its links, downloads the whole directory will all the websites from the server using GET requests. Similarly to the webserver, it uses threads for sending each request. From every page download the crawler has access to new links which are placed in a queue of unique new urls (uniqueness checked with a hashtable) thus accessing all sites. </br>Simultaneously, crawler listens from the command port for commands. The special command here is SEARCH, as cralwer after downloading all the sites becomes a mini **"search engine"**.  The user can search for up to 10 words in the sites and every paragraph or line including at least one of the words is returned thourgh the socket. For achieving this, the main thread makes new process(Workers) to do the job. The communication is done with **named pipes(fifos)** and **signals**. Lastly, the searching in each worker is done by implementing a **Trie**, used for storing individual words.
