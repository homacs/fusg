
# File Usage Monitoring Thingy (FUSG)

ALPHA VERSION!

The file usage daemon (*fusgd*) collects statistics about usage 
of files, to answer common questions such as these:

* Is a particular file or directory still being used by any program, 
  or can I finally delete it?
* Where are save and config files and caches of a certain program 
  located.

Those informations can be queried from the *fusg.db* database by use of
a command line tool called *fusg*.

## Prerequisites
FUSG depends on:

* auditd: A user-space auditing daemon. 
* libauparse: Library used to parse auditing events.
* libaudit: Library required by libauparse.
* gdbm: GNU file-based name-value db.

or simply:

    sudo apt-get install   auditd libauparse-dev libaudit-dev libgdbm-dev


## Known Issues

This is a prototype to test feasibility of the approach, 
and there are a couple of known issues:

* Slows system calls down or misses file access events 
  (depends on dispatch mode of audispd).
* Does not consider symbolic links properly.
* Its not particular user friendly.


## Concept

### Functional Requirements

File stands for file or directory.


* Watch local files of a given set of directories (e.g. /home/user)
* For each file provide the following information:
  file_action + executable + last_time_stamp (for this combination)
  Each combination of file_action and executable has exactly one entry.
  file_action is one of: create | read | write | delete
* For each executable provide the following information:
  file_name + file_action + last_time_stamp
* Tell which executable created a given file.
* Tell which files are used by an executable

### Possible Future Requirements

* parent process?
* filter actions of certain executables (e.g. updatedb)
* filter files or actions on certain files (e.g. .bashrc ?)
* classify certain file types with known usage
  * data:   (.pdf, .doc, ...)
  * config: (~/.config/*)
  * cache:  (~/.cache)

### Architecture Overview

<pre>
    -----------	pipe  ------------      ----------
    | auditd  |------>| audispd  |      |  fusg  |------> stdout
    ----------- disp. ------------      ----------
		 /\               |                  |
		 |                |                  | read
	  netlink             | pipe to          \/
		 |                | plugins         ____
		 |                \/              < ____ >
    -----------       -----------  store  |      |
    | kauditd |       |  fusgd  | ------> | fusg |
    -----------       -----------         |  .db |
		    		                      `------´
</pre>
Quite a long way ...

		  
#### auditd
Daemon, which listens to kernel events from kernel audit subsystem 
(see 'kernel/audit.c' in kernel source).
* Configure: Configuration of watches for files (see auditd.conf(5) or auditctl(8)).
* Watch: Listens to kernel events, forwards to a dispatcher
  which is usually audispd (part of auditd package).
  and logs them in /var/log/audit/audit.log (optionally).

#### audispd
Daemon which receives audit events from auditd and forwards them to 
plugins. Plugins have to have a config file in /etc/audisp/plugins.d 
(see audispd(8)).

#### fusgd
An audispd plugin which listens to file access events
and stores them in fusg.db.
##### Tasks:
  * Uses libaudit to parse audit log files.
  * Updates entries in db for each directory and file
  * stores last update time
  
#### fusg
Command line tool to query the fusg database.

1. For a given executable, list all files/folders ever used by it.
    - for each used file:
        * Last time accessed
        * Number of accesses
        * Type of access (create/read/write/delete?)
2. For a given file or folder, list all executables, which used it.
    - for each executable + file/folder:
        * Last time accessed
        * Number of accesses
        * Type of access (create/read/write/delete?)
3. List all file<->executable statistics

<pre>
    > fusg -e /bin/firefox
    
    File usage statistics
    executable: '/bin/firefox'
    
    CREAT   READ  WRITE  FILE
    ---------------------------
      1   1002    124  /home/homac/.mozilla
      1   1002    124  /home/homac/.mozilla
      1   1002    124  /home/homac/.mozilla
</pre>      

#### fusg.db
Database, containing file access events.
Its one big table (possibly partitioned by directories) with the following
schema:

    exec.db:
		exe_id path_to_exe
    execr.db:
		path_to_exe exe_id
    file.db:
		file_id path_fo_file
    filer.db:
		path_fo_file file_id
    evnt.db: 
		file_id exe_id create_count read_count write_count last_access_time_stamp
    idtb.db:
        id: next_unique_id




# PRIVACY OF DATABASE ENTRIES

Currently, fusg does not consider privacy issues. Any user can see which 
executable accessed which file/folders. Thus, seeing entries like 
/bin/wireshark accessed /home/<username>/.wireshark lets anybody know, that user 
'username' did something with wireshark. Thus, visibility of user 
actions should be restricted to the user itself and admins in the future. 

