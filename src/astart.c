/* astart.c: start all the processes used by the allnet daemon */
/* takes as argument the interface(s) on which to start
 * sending and receiving broadcast packets */
/* in the future this may be automated or from config file */
/* (since the config file should tell us the bandwidth on each interface) */
/* alternately, if the single program is named astop or the single argument
 * is "stop", stops running ad, assuming the pids are in
 * /var/run/allnet-pids or /tmp/allnet-pids */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include "lib/util.h"
#include "lib/log.h"

static void init_pipes (int * pipes, int num_pipes)
{
  int i;
  for (i = 0; i < num_pipes; i++) {
    if (pipe (pipes + i * 2) < 0) {
      perror ("pipe");
      printf ("error creating pipe set %d\n", i);
      snprintf (log_buf, LOG_SIZE, "error creating pipe set %d\n", i);
      log_print ();
      exit (1);
    }
  }
}

static char * itoa (int n)
{
  char * result = malloc (12);  /* plenty of bytes, easier than being exact */
  snprintf (result, 10, "%d", n);
  return result;
}

static void print_pid (int fd, int pid)
{
  char * buffer = itoa (pid);
  int len = strlen (buffer);
  if (write (fd, buffer, len) != len)
    perror ("pid file write");
  buffer [0] = '\n';
  if (write (fd, buffer, 1) != 1)
    perror ("pid file newline write");
  free (buffer);
}

/* returned value is malloc'd */
static char * make_program_path (char * path, char * program)
{
  int size = strlen (path) + 1 + strlen (program) + 1;
  char * result = malloc (size);
  if (result == NULL) {
    printf ("error: unable to allocate %d bytes for %s/%s, aborting\n",
            size, path, program);
    exit (1);
  }
  snprintf (result, size, "%s/%s", path, program);
  return result;
}

static void exec_error (char * executable)
{
  perror ("execv");
  printf ("error executing %s\n", executable);
  static char buf [100000];
  char * pwd = getcwd (buf, sizeof (buf));
  printf ("current directory is %s\n", pwd);
  exit (1);
}

static void my_exec_trace (char * path, char * program, int fd)
{
  /* for now, hard-code to 16-bit random addresses */
  char addr [2];
  random_bytes (addr, sizeof (addr));
  char paddr [5];
  snprintf (paddr, sizeof (paddr), "%02x%02x", (addr [0] & 0xff),
            (addr [1] & 0xff));
  char alen [] = "16";
  
  int pid = fork ();
  if (pid == 0) {
    char * args [4];
    args [0] = make_program_path (path, program);
    args [1] = paddr;
    args [2] = alen;
    args [3] = NULL;
    snprintf (log_buf, LOG_SIZE, "calling %s %s %s\n",
              args [0], args [1], args [2]);
    log_print ();
    execv (args [0], args);    /* should never return! */
    exec_error (args [0]);
  } else {  /* parent, not much to do */
    print_pid (fd, pid);
    snprintf (log_buf, LOG_SIZE, "parent called %s %s %s\n",
              program, paddr, alen);
    log_print ();
  }
}

static void my_exec2 (char * path, char * program, int * pipes, int fd)
{
  int pid = fork ();
  if (pid == 0) {
    char * args [4];
    args [0] = make_program_path (path, program);
    args [1] = itoa (pipes [0]);
    args [2] = itoa (pipes [3]);
    args [3] = NULL;
    close (pipes [1]);
    close (pipes [2]);
    snprintf (log_buf, LOG_SIZE, "calling %s %s %s\n",
              args [0], args [1], args [2]);
    log_print ();
    execv (args [0], args);    /* should never return! */
    exec_error (args [0]);
  } else {  /* parent, close the child pipes */
    print_pid (fd, pid);
    close (pipes [0]);
    close (pipes [3]);
    snprintf (log_buf, LOG_SIZE, "parent called %s %d %d, closed %d %d\n",
              program, pipes [0], pipes [3], pipes [0], pipes [3]);
    log_print ();
  }
}

static void my_exec3 (char * path, char * program, int * pipes, char * extra, int fd)
{
  int pid = fork ();
  if (pid == 0) {
    char * args [5];
    args [0] = make_program_path (path, program);
    args [1] = itoa (pipes [0]);
    args [2] = itoa (pipes [3]);
    args [3] = extra;
    args [4] = NULL;
    close (pipes [1]);
    close (pipes [2]);
    snprintf (log_buf, LOG_SIZE, "calling %s %s %s %s\n",
              args [0], args [1], args [2], args [3]);
    log_print ();
    execv (args [0], args);
    exec_error (args [0]);
  } else {  /* parent, close the child pipes */
    print_pid (fd, pid);
    close (pipes [0]);
    close (pipes [3]);
    snprintf (log_buf, LOG_SIZE, "parent called %s %d %d %s, closed %d %d\n",
              program, pipes [0], pipes [3], extra, pipes [0], pipes [3]);
    log_print ();
  }
}

static void my_exec_ad (char * path, int * pipes, int num_pipes, int fd)
{
  int i;
  int pid = fork ();
  if (pid == 0) {
    /* ad takes as args the number of pipe pairs, then the actual pipe fds */
    int num_args = 2 /* program + number */ + num_pipes + 1 /* NULL */ ;
    char * * args = malloc_or_fail (num_args * sizeof (char *),
                                    "astart ad args");
    args [0] = make_program_path (path, "ad");
    args [1] = itoa (num_pipes / 2);
    int n = snprintf (log_buf, LOG_SIZE, "calling %s %s ", args [0], args [1]);
    for (i = 0; i < num_pipes / 2; i++) {
      args [2 + i * 2    ] = itoa (pipes [4 * i + 2]);
      args [2 + i * 2 + 1] = itoa (pipes [4 * i + 1]);
      close (pipes [4 * i    ]);
      close (pipes [4 * i + 3]);
      n += snprintf (log_buf + n, LOG_SIZE - n,
                     "%s %s ", args [2 + i * 2], args [2 + i * 2 + 1]);
    }
    log_print ();
    args [num_args - 1] = NULL;
    execv (args [0], args);
    exec_error (args [0]);
  } else {  /* parent, close the child pipes */
    print_pid (fd, pid);
    for (i = 0; i < num_pipes / 2; i++) {
      close (pipes [4 * i + 1]);
      close (pipes [4 * i + 2]);
    }
  }
}

static char * pid_file_name ()
{
  int is_root = (geteuid () == 0);
  if (is_root)
    return "/var/run/allnet-pids";
  else
    return "/tmp/allnet-pids";
}

/* returns 0 in case of failure */
static int read_pid (int fd)
{
  int result = -1;
  char buffer [1];
  while (read (fd, buffer, 1) == 1) {
    if ((buffer [0] >= '0') && (buffer [0] <= '9')) {  /* digit */
      if (result == -1)
        result = buffer [0] - '0';
      else
        result = result * 10 + buffer [0] - '0';
    } else if (result == -1) {   /* reading whatever precedes the number */
      /* no need to do anything, just read the next char */
    } else {                     /* done */
      if (result > 1)
        return result;
      snprintf (log_buf, LOG_SIZE, "weird result from pid file %d\n", result);
      log_print ();
      result = -1;  /* start over */
    }
  }
}

#define AIP_UNIX_SOCKET		"/tmp/allnet-addrs"

static int stop_all ()
{
  char * fname = pid_file_name ();
  int fd = open (fname, O_RDONLY, 0);
  if (fd < 0) {
    printf ("unable to stop allnet daemon, missing pid file %s\n", fname);
    if (geteuid () == 0)
      printf ("if it is running, perhaps it was started as a user process\n");
    else
      printf ("if it is running, perhaps it was started as a root process\n");
    return 1;
  }
  printf ("stopping allnet daemon\n");
  int pid;
  while ((pid = read_pid (fd)) > 0)
    kill (pid, SIGINT);
  close (fd);
  unlink (fname);
  unlink (AIP_UNIX_SOCKET);
}

static void find_path (char * arg, char ** path, char ** program)
{
  char * slash = rindex (arg, '/');
  if (slash == NULL) {
    *path = ".";
    *program = arg;
  } else {
    *slash = '\0';
    *path = arg;
    *program = slash + 1;
  }
}

int main (int argc, char ** argv)
{
  char * path;
  char * pname;
  find_path (argv [0], &path, &pname);
  if (strstr (pname, "stop") != NULL)
    return stop_all ();   /* just stop */

  init_log ("astart");  /* only do logging in astart */
  /* two pipes from ad to alocal and back */
  /* two pipes from ad to acache and back */
  /* two pipes from ad to aip and back */
#define NUM_FIXED_PIPES		6
  /* two pipes from ad to each abc and back */
#define NUM_INTERFACE_PIPES	2 
  int num_interfaces = argc - 1;
  int num_pipes = NUM_FIXED_PIPES + NUM_INTERFACE_PIPES * num_interfaces;
  int * pipes = malloc_or_fail (num_pipes * 2 * sizeof (int), "astart pipes");
  init_pipes (pipes, num_pipes);

  int pid_fd = open (pid_file_name (),
                     O_WRONLY | O_TRUNC | O_CREAT | O_CLOEXEC, 0600);
  if (pid_fd < 0) {
    perror ("open");
    snprintf (log_buf, LOG_SIZE, "unable to write pids to %s\n",
              pid_file_name ());
    log_print ();
    return 1;
  }

  /* start ad */
  my_exec_ad (path, pipes, num_pipes, pid_fd);

  /* start all the other programs */
  my_exec2 (path, "alocal", pipes, pid_fd);
  my_exec2 (path, "acache", pipes + 4, pid_fd);
  my_exec3 (path, "aip", pipes + 8, AIP_UNIX_SOCKET, pid_fd);
  int i;
  for (i = 0; i < num_interfaces; i++)
    my_exec3 (path, "abc", pipes + 12 + (4 * i), argv [i + 1], pid_fd);
  sleep (2);
  my_exec_trace (path, "traced", pid_fd);
}