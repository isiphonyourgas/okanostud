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
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
// Pointer to the url file
FILE *f;
// Counter for number of accesses
int *accesses;
struct timeval *recent;
int number = 0;
int wwidth;

void load_time( struct timeval x )
{
  int i;
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

void *probe( void *num )
{
  char *url = (char*) malloc(256);
  unsigned int urlsize = 256;
  int n = (int) num;
  accesses[n] = 0;
  struct timeval time1, time2, timer;
  char *tmp;
  pid_t pid;
  int stat;
  while(1)
  {
    pthread_mutex_lock( &mutex1 );
    if( feof(f) )
      rewind(f);
    getline( &url, &urlsize, f );
    tmp = strchr(url, '\n');
    if( tmp != 0 )
      strcpy(tmp, "");
    pthread_mutex_unlock( &mutex1 );
    gettimeofday( &time1, 0 );
    pid = fork();
    if( pid == 0 )
    {
      execlp("wget","wget","--spider","-q",url,NULL);
      _exit(0);
    }
    else
      waitpid(pid, &stat, 0);
    gettimeofday( &time2, 0 );
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
    pthread_mutex_lock( &mutex2 );
    load_time(timer);
    accesses[n]++;
    pthread_mutex_unlock( &mutex2 );
  }
}

void *reporter( void *num )
{
  double length = 10.0/((int)num) * 1000000;
  int i;
  while(1)
  {
    usleep(length);
    pthread_mutex_lock( &mutex2 );
    printf("\n\nRecent times:\n");
    for( i = 0; i < number; i++ )
      printf("%d:  %ld.%ld\n", i, recent[i].tv_sec, recent[i].tv_usec);
    printf("\nThread accesses:\n");
    for( i = 0; i < (int)num; i++ )
      printf("Thread %d: %d\n", i, accesses[i]);
    pthread_mutex_unlock( &mutex2 );
  }
}

int main( int argc, char *argv[] )
{
  // Check number of arguments
  if( argc != 4 )
  {
    printf("Too few arguments\n");
    printf("Usage: webprobe urlfile wwidth numthreads\n");
    return 0;
  }

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

  pthread_join( id[0], NULL );

  return 0;

}


