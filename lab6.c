#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define SEM_RESOURCE_MAX  1       
#define SEMMSL 32

union semun {
  int             val;    
  struct semid_ds *buf;   
  unsigned short  *array;  
  struct seminfo  *__buf;  
};

void opensem(int *sid, key_t key);
void createsem(int *sid, key_t key, int members);
void locksem(int sid, int member);
void unlocksem(int sid, int member);
void removesem(int sid);
unsigned short get_member_count(int sid);
int getval(int sid, int member);
void dispval(int sid, int member);
void changemode(int sid, char *mode);
void usage(void);

int main(int argc, char *argv[])
{
  key_t key;
  int   semset_id;

  key = ftok(".", 's');

  switch(tolower(argv[1][0]))
  {
    case 'c':
      createsem(&semset_id, key,  atoi(argv[2]));
      break;
    case 'l':
      opensem(&semset_id, key);
      locksem(semset_id, atoi(argv[2]));
      break;
    case 'u':
      opensem(&semset_id, key);
      unlocksem(semset_id, atoi(argv[2]));
      break;
    case 'd':
      opensem(&semset_id, key);
      removesem(semset_id);
      break;
    case 'm':
      opensem(&semset_id, key);
      changemode(semset_id, argv[2]);
      break;
  }

  return 0;
}

void opensem(int *sid, key_t key)
{
  *sid = semget(key, 0, 0666);
}

void createsem(int *sid, key_t key, int members)
{
  int cntr;
  union semun semopts;
  printf("Attempting to create new semaphore set with %d members\n", members);

  *sid = semget(key, members, IPC_CREAT|IPC_EXCL|0666);
  semopts.val = SEM_RESOURCE_MAX;

  for (cntr = 0; cntr < members; cntr++) {
    semctl(*sid, cntr, SETVAL, semopts);
  }
}

void locksem(int sid, int member)
{
  struct sembuf sem_lock= {0, -1, IPC_NOWAIT};

  if (!getval(sid, member))
  {
    fprintf(stderr, "Semaphore resources exhausted (no lock)!\n");
    exit(1);
  }

  sem_lock.sem_num = member;

  semop(sid, &sem_lock, 1);
    printf("Semaphore resources decremented by one (locked)\n");
    dispval(sid, member);
}

void unlocksem(int sid, int member)
{
  struct sembuf sem_unlock={ member, 1, IPC_NOWAIT};
  int semval;

  semval = getval(sid, member);
  sem_unlock.sem_num = member;
    printf("Semaphore resources incremented by one (unlocked)\n");
    dispval(sid, member);
}

void removesem(int sid)
{
  semctl(sid, 0, IPC_RMID, 0);
  printf("Semaphore removed\n");
}

unsigned short get_member_count(int sid)
{
  union semun semopts;
  struct semid_ds mysemds;

  semopts.buf = &mysemds;

  return semopts.buf->sem_nsems;
}

int getval(int sid, int member)
{
  int semval;

  semval = semctl(sid, member, GETVAL, 0);

  return semval;
}

void changemode(int sid, char *mode)
{
  int rc;
  union semun semopts;
  struct semid_ds mysemds;

  semopts.buf = &mysemds;

  rc = semctl(sid, 0, IPC_STAT, semopts);

  printf("Old permissions were %o\n", semopts.buf->sem_perm.mode);

  sscanf(mode, "%ho", &semopts.buf->sem_perm.mode);

  semctl(sid, 0, IPC_SET, semopts);

  printf("Updated...\n");
}

void dispval(int sid, int member)
{
  int semval;

  semval = semctl(sid, member, GETVAL, 0);

  printf("semval for member %d is %d\n", member, semval);
}
