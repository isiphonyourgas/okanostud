// Aaron Okano, Jason Wong, Meenal Tambe, Gowtham Vijayaragavan
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

// Initialize some global variables
// These are the mutexes to lock a couple sections of code
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
// Pointer to the url file
FILE *f;
// Counter for number of accesses
int *accesses;
// Timevals used for the timer
struct timeval *recent;
// Counting number that was easier to just make global
int number = 0;
// wwidth is used by the threads occasianally,
// so it was also better off as a global
int wwidth;
// Used to terminate all the threads
int status;

// This function loads the time values into the recent array
void load_time( struct timeval x )
{
  int i;
  // Uses if-else statement to allow for the array to shift and
  // to prevent printing out garbage values later
  if( number == wwidth )
  {
    for( i = 0; i < number - 1; i++ )
      recent[i] = recent[i + 1];
    recent[i] = x;
  }
  else
  {
    recent[number] = x;
    number++;
  }
}

// This is the main function for the probe threads
void *probe( void *num )
{
  // THIS USES WAY TOO MANY VARIABLES! I AM ASHAMED OF THIS CODE!
  char *url = (char*) malloc(256);
  unsigned int urlsize = 256;
  int n = (int) num;
  accesses[n] = 0;
  // These timevals are used to make a timer for wget
  struct timeval time1, time2, timer;
  char *tmp;
  // Probably not even useful anymore
  pid_t pid;
  int stat;
  while(status)
  {
    // Prevent simultaneous writes to the FILE*
    pthread_mutex_lock( &mutex1 );
    if( feof(f) )
      rewind(f);
    getline( &url, &urlsize, f );
    // The '\n' needs to be eliminated from the string
    tmp = strchr(url, '\n');
    // The end of the file will not have a '\n',
    // so we don't need to erase it
    if( tmp != 0 )
      strcpy(tmp, "");
    pthread_mutex_unlock( &mutex1 );
    // "Begin timer" by recording current time
    gettimeofday( &time1, 0 );
    // execlp replaces the calling process, so we need to fork first
    pid = fork();
    // Child process will do this part...
    if( pid == 0 )
    {
      execlp("wget","wget","--spider","-q",url,NULL);
      _exit(0);
    }
    // ... while parent process waits for it to complete
    else
      waitpid(pid, &stat, 0);
    // "Stop timer" by recording current time
    gettimeofday( &time2, 0 );
    if( stat == 0 )
    {
      // Complicated mess of crap to subtract ending time from starting time
      if( time2.tv_usec < time1.tv_usec )
      {
        int nsec = (time1.tv_usec - time2.tv_usec) / 1000000 + 1;
        time1.tv_usec -= 1000000 * nsec;
        time1.tv_sec += nsec;
      }
      if( time2.tv_usec - time1.tv_usec > 1000000 )
      {
        int nsec = (time2.tv_usec - time1.tv_usec) / 1000000 + 1;
        time1.tv_usec += 1000000 * nsec;
        time1.tv_sec -= nsec;
      }
      timer.tv_sec = time2.tv_sec - time1.tv_sec;
      timer.tv_usec = time2.tv_usec - time1.tv_usec;
      // load_time() makes changes to a couple shared variables,
      // so we lock this section first
      pthread_mutex_lock( &mutex2 );
      load_time(timer);
      // Increment the count for number of accesses
      accesses[n]++;
      pthread_mutex_unlock( &mutex2 );
    }
  }
  return 0;
}

// Function that the reporting thread operates from
void *reporter( void *num )
{
  // The length of sleep time will be in microseconds,
  // so this line is a jumble of crap to deal with that
  double length = 10.0/((int)num) * 1000000;
  int i;
  while(status)
  {
    // Sleep for 10 / numthreads seconds
    usleep(length);
    printf("\n\nRecent times:\n");
    for( i = 0; i < number; i++ )
      printf("%d:  %ld.%ld\n", i, recent[i].tv_sec, recent[i].tv_usec);
    printf("\nThread accesses:\n");
    for( i = 0; i < (int)num; i++ )
      printf("Thread %d: %d\n", i, accesses[i]);
  }
  return 0;
}

int main( int argc, char *argv[] )
{
  char check;
  // Check number of arguments
  if( argc != 4 )
  {
    printf("Too few arguments\n");
    printf("Usage: webprobe urlfile wwidth numthreads\n");
    return 0;
  }

  status = 1;
  // Set arguments to values
  f = fopen(argv[1], "r");
  wwidth = atoi(argv[2]);
  int numthreads = atoi(argv[3]);
  // Set the arrays to their proper sizes
  pthread_t *id = (pthread_t*) malloc(sizeof(pthread_t)*numthreads);
  // Make numthreads equal to the number of probe threads only
  numthreads = numthreads - 1;
  accesses = (int*) malloc(sizeof(int)*numthreads);
  recent = (struct timeval*) malloc(sizeof(struct timeval)*numthreads);

  // Check if file exists
  if( f == NULL )
  {
    printf("File doesn't exist!\n");
    return 0;
  }
  
  int i;
  // Create probing threads
  for( i = 0; i < numthreads; i++ )
    pthread_create( &id[i], NULL, probe, (void*) i );
  // Create reporting thread
  pthread_create( &id[numthreads], NULL, reporter, (void*) numthreads );
  
  do
  {
  check = getchar();
  } while( check != 'q' );

  status = 0;

  for( i = 0; i < numthreads+1; i++ )
    pthread_join( id[i], NULL );

  free(id);
  free(accesses);
  free(recent);
  printf("Bye\n");

  return 0;

}
