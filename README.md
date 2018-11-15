# Pintos Project2: User Programs
Working on the parts of the system that allow running user programs

## Demo: how to use
To run user program "echo" on pintos
```bash
pintos-mkdisk filesys.dsk --filesys-size=2
pintos -f -q
pintos -p ../../examples/echo -a echo -- -q
pintos -q run 'echo x'
```

## Requirements
1. Process Termination Messages
2. Argument Passing
3. System Calls

## Implements
```C
void halt (void)
void exit (int status)
pid_t exec (const char *cmd_line)
int wait (pid_t pid)
bool create (const char *file, unsigned initial_size)
bool remove (const char *file)
int open (const char *file)
int filesize (int fd)
int read (int fd, void *buffer, unsigned size)
int write (int fd, const void *buffer, unsigned size)
void seek (int fd, unsigned position)
unsigned tell (int fd)
void close (int fd)
```

## Build
First, cd into the userprog directory. Then, issue the make command. This will create a build directory under userprog, populate it with a Makefile and a few subdirectories, and then build the kernel inside.
```bash
./src/userprog/make
```
## Testing
A script scores the output as "pass" or "fail" and writes the verdict to t.result.
```bash
./src/userprog/builds make SIMULATOR=--bochs check
```

## Result
Tests: 71/76 pass

## Reference
Provide the [Pintos Gudie](https://web.stanford.edu/class/cs140/projects/pintos/pintos_3.html#SEC32):+1:
