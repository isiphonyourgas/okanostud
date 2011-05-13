// Aaron Okano, Jason Wong, Meenal Tambe, Gowtham Vijayaragavan
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

// Initialize some global variables
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
// Pointer to the url file
FILE *f;
// Counter for number of accesses
int *accesses;
long int *recent;
int number = 0;
int wwidth;

void load_time( long int x )
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
  struct timeval timer1, timer2;
  long int time;
  char *tmp;
  while(1)
  {
    pthread_mutex_lock( &mutex1 );
    if( feof(f) )
      rewind(f);
    getline( &url, &urlsize, f );
    tmp = strchr(url, '\n');
    strcpy(tmp, "");
    pthread_mutex_unlock( &mutex1 );
    gettimeofday( &timer1, NULL );
    execlp("wget","wget","--spider",url,NULL);
    gettimeofday( &timer2, NULL );
    time = timer2.tv_usec - timer1.tv_usec;
    load_time(time);
    accesses[n]++;
  }
}

void *reporter( void *num )
{
  double length = 10.0/((int)num) * 10000;
  int i;
  while(1)
  {
    usleep(length);
    printf("Recent times:\n");
    for( i = 0; i < number; i++ )
      printf("%d:  %ld\n", i, recent[i]);
    printf("\n\nThread accesses:\n");
    for( i = 0; i < (int)num; i++ )
      printf("Thread %d: %d\n", i, accesses[i]);
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
  recent = (long int*) malloc(sizeof(int)*numthreads);

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


