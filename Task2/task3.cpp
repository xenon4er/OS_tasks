#include <stdlib.h>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <queue>
#include <string>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;



struct Task
{
    int start;
    int middle;
    int end;
    int finish;
};

struct TaskManager
{  
    int num_of_process;
    int sharm_id;
    int pipes[2];
    void* adr;
    string** items;
    int size_items;
    
    int current_sem;
    int work_sem;
    int last_ind_sem;
    
    int finish_work;
    int taskcount;
    int old_len;
};

void ReadFromFile(char* filename, TaskManager &man);
void simplemerge(TaskManager &man);
void createTaskQueue(TaskManager &manager, Task task);
//void WriteInFile(char* filename, vector<string> arr);
void chWork(TaskManager &manager, Task task);


int main(int argc, char** argv)
{   

   const int argLim = 4;
   if (argc != argLim)
   {
       printf("usage: ./sort <input> <output> <num of thread>");
       return 0;
   }
    
   TaskManager manager;
   manager.num_of_process = atoi(argv[3]);
   ReadFromFile(argv[1], manager);
   manager.last_ind_sem = 0;
   manager.current_sem = 0;
	manager.finish_work = 0;
	manager.taskcount = 0;
	manager.old_len = 2;
 	
   printf("start\n");
    
   struct Task begin;
   begin.start = 0;
   begin.end = manager.size_items;
   begin.finish = 0; 
   
   pipe(manager.pipes);
   
   
	// create sem
	manager.work_sem = semget(IPC_PRIVATE, 1, 0666);
	printf("semop created\n");
	chWork(manager, begin);
	createTaskQueue(manager, begin);
	
	write(manager.pipes[1],&begin,sizeof(Task));

	for(int i =0; i<manager.num_of_process;i++)
	{
		Task finish;
		finish.finish = 1;
		write(manager.pipes[1],&finish,sizeof(Task));
	}
	//return 0;
	
   //del sem
   semctl(manager.work_sem, 1, IPC_RMID);
	printf("semop closed\n"); 
   sleep(2);
   manager.adr = shmat(manager.sharm_id,0,0);
	manager.items = (string **)manager.adr;
	for(int i =0; i<manager.size_items; i++)
	{
		printf("%i = %s\n",i+1,manager.items[i]->c_str());
	}
	
	shmdt(manager.adr); 
   printf("the end\n");
   return 0;
}

void chWork(TaskManager &manager, Task task)
{

	
	for (int i = 0; i < manager.num_of_process; i++)
	{
		pid_t cid = fork();
		if (cid  == 0)
		{
			printf("%i = %i\n",i,getpid());
			simplemerge(manager);
			printf("child fin\n\n");
			exit(0);
			
			
		}  if(cid == 1)
		{
			
		}
	}
	
}


void simplemerge(TaskManager &manager)
{
	Task task;
	//task.finish = 0;
	
	read(manager.pipes[0],&task,sizeof(Task));
	while(true)
	{
		if (task.finish == 1) return;	
		task.middle = task.start + (task.end - task.start)/2;
		 
		 string *tmp;
		 
		 int first = task.start; 
		 int second = task.middle;

		 manager.adr = shmat(manager.sharm_id,0,0);
		 manager.items = (string **)manager.adr;
		 
		 while ((task.middle > first) && (task.end > second))
		 {
		   
		   	string *s1 = manager.items[first];
		 		string *s2 = manager.items[second]; 
				if (*s1 > *s2)
		      {
		     		
		         tmp = manager.items[second];
		         for(int i=0; second-i>first;i++)
		         {
		             manager.items[second-i] = manager.items[second-i-1];
		         }
		         manager.items[first] = tmp;
		         task.middle++;
		         second++;
		     }       
		     else
		     {
		         first++;
		     }
		     
    		}
    		shmdt(manager.adr);
    	//printf("task middle = %i",task.middle);
    	
    	if(task.finish == 1)
    	{
    		 manager.finish_work = 1;
    		 //return;
    	}
    		
    		struct sembuf buf;
			buf.sem_num = manager.current_sem;
			buf.sem_op = 1;
			buf.sem_flg = IPC_NOWAIT; 
			if((semop(manager.work_sem, &buf, 1)) < 0) {
				perror("semop1");
				exit(EXIT_FAILURE);
			}
			
    		printf("inc\n");
    	
    	read(manager.pipes[0],&task,sizeof(Task));	
    	if(task.finish == 1) printf("\nfinish task!!!\n\n");
		
    		
    }
}

void createTaskQueue(TaskManager &manager, Task task)
{  
	
    int len = task.end - task.start;
    task.finish = 0;
    const int lenLim = 2;
    if (len > lenLim)
    {
        
        int middle = task.start + len/2;
        
        struct Task left;
        left.start = task.start;
        left.end = middle;
        left.finish = task.finish;
        
        struct Task right;
        right.start = middle;
        right.end = task.end;
        left.finish = task.finish;
        
        
       
        createTaskQueue(manager, left );
        createTaskQueue(manager, right);                  
        
      printf("taskcount = %i\n",manager.taskcount);      
      
      
		
		write(manager.pipes[1], &left, sizeof(Task));
      printf("lef was add    %i  %i \n", left.start, left.end);
      write(manager.pipes[1], &right, sizeof(Task));
		printf("right was add %i  %i \n", right.start, right.end);
		
		manager.taskcount+=2; 
		
		if((right.end - right.start) != manager.old_len)
      {
      	printf("old = %i now = %i\n",manager.old_len, right.end - right.start);
      	manager.old_len = right.end - right.start;
		   
		   struct sembuf buf;
			buf.sem_num = manager.current_sem;
			buf.sem_op =  - manager.taskcount;
			buf.sem_flg = 0;//SEM_UNDO; 
			
			printf("sem_op = %i\n\n",buf.sem_op);
			if((semop(manager.work_sem, &buf, 1)) < 0) {
				perror("semop");
				exit(EXIT_FAILURE);
			}
			
			manager.taskcount = 0;
			
		}
		      
    }
}

void ReadFromFile(char* filename, TaskManager &man)
{
   vector<string*> arr;
   ifstream fs;
    
   fs.open(filename);
   string* s = new string();
   while (std::getline(fs, *s))
   {
       arr.push_back(s);
       s = new string();
   }
   fs.close();
    
   man.size_items = arr.size();
   int size_memory = sizeof(string *) * man.size_items;
	man.sharm_id = shmget(IPC_PRIVATE, size_memory, 0666);
	
	man.adr = shmat(man.sharm_id,0,0);
	man.items = (string **)man.adr;
    
   for(int i = 0; i<man.size_items;i++)
   {
   	man.items[i] = arr[i];
   	//printf("%s\n",man.items[i]->c_str());
   }
   int q = shmdt(man.adr);
   
   printf("good\n");
}





/*
void WriteInFile(char* filename, vector<string> arr)
{
    ofstream out;
    out.open(filename);
    for(int i=0; i<arr.size();i++)
    {
        out<<arr[i]<<endl;
    }
   
    out.close();
}

void printarr(vector<string> &arr)
{
    for (int i = 0; i < arr.size(); i++)
    {
        printf("%i %s\n", i, arr[i].c_str());
    }
}
*/
