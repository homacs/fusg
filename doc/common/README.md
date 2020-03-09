


# File Usage Monitoring Tool

## Purpose

The file usage data base stores summarised information about usage 
of files, to answer common questions such as these:

* Is a particular file or directory still being used by any program, 
  or can I finally delete it?
* Where are save and config files and caches of a certain program 
  located.

These are vital informations if you want to keep your system clean 
over long periods of time. Especially home directories get utterly 
cluttered by lots of hidden directories and in many cases it's hard 
to determine, whether these files are still in use by some program.


## Concept

### Functional Requirements

File stands for file or directory.


* Watch local files of a given set of directories (e.g. /home/user)
* For each file provide the following information:
  file_action + executable + user + last_time_stamp (for this combination)
  Each combination of file_action, executable, user has exactly one entry.
  file_action is one of: create | read | write/read | remove
* For each executable and/or user, provide the following information:
  file_name + file_action + last_time_stamp
* List users of a file.
* Tell which executable and/or user created a given file.
* Tell which files are used by a user
* Tell which files are used by an executable

### Possible Future Requirements

* parent process?
* filter actions of certain executables (e.g. updatedb)
* filter files or actions on certain files (e.g. .bashrc ?)
* classify certain file types with known usage
  * data:   (.pdf, .doc, ...)
  * config: (~/.config/*)
  * cache:  (~/.cache)

### Architecture


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

Quite a long way ...

		  
#### auditd
Daemon, which listens to kernel events from kernel audit subsystem 
(see 'kernel/audit.c' in kernel source).
* Configure: Configuration of watches for files (/etc/audit/* or auditctl).
* Watch: Listens to kernel events, forwards to a dispatcher
  which is usually audispd (part of auditd package).
  and logs them in /var/log/audit/audit.log (optionally).

#### fusgd
Daemon, which monitors audit logs.
* triggers data base update on demand (by user request, 
  by each new entry, or when audit log rotates)
* When update is triggered:
  * Uses libaudit to parse audit log files.
  * Updates entries in db for each directory and file
  * stores last update time
  * stores time stamp of last event processed
  
#### fusg (or fu)

1. For a given Executable, list all files/folders ever used by it.
    - for each used file:
        * Last time accessed
        * Number of accesses
        * Type of access (create/read/write/delete?)
2. For a given file or folder, list all executables, which used it.
    - for each executable + file/folder:
        * Last time accessed
        * Number of accesses
        * Type of access (create/read/write/delete?)


  > fusg -e /bin/firefox

  File usage statistics
  executable: '/bin/firefox'
  
  CREAT   READ  WRITE  FILE
  ---------------------------
      1   1002    124  /home/homac/.mozilla
      1   1002    124  /home/homac/.mozilla
      1   1002    124  /home/homac/.mozilla
      

#### fusg.db
Database, containing file action events.
Its one big table (possibly partitioned by directories) with the following
schema:



executables:
		exe_id path_to_exe

files:
		file_id path_fo_file
		
actions: 
		file_id exe_id create_count read_count write_count last_time_stamp



That's all.



# PRIVACY OF DATABASE ENTRIES

Currently, fusg does not consider privacy issues. Any user can see which 
executable accessed which file/folders. Thus, seeing entries like 
/bin/wireshark accessed /home/yourname/.wireshark lets anybody know, that user 
'yourname' did something suspicious (maybe). Thus, visibility of user 
actions should be restricted to the user itself and admins which could 
be achieved by storing user actions in user-specific databases. 

