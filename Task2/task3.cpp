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
 	
   printf("start\n");
    
   struct Task begin;
   begin.start = 0;
   begin.end = manager.size_items;
   begin.finish = 0; 
   vector<Task> TaskList;
   pipe(manager.pipes);
   
   
   //createTaskQueue(begin, TaskList);
   
   
   //TaskList.push_back(begin);
   //write(manager.pipes[1], &begin, sizeof(Task));
   
   for(int i=0;i<TaskList.size();i++)
   {
   	printf("s=%i e=%i ",TaskList[i].start,TaskList[i].end);
   	if (TaskList[i].finish == 1) 
   	{
   		printf("true\n");
   	}
   	else printf("false\n");
   }
   
	//return 0;
	// create sem
	manager.work_sem = semget(IPC_PRIVATE, 1, 0666);
	chWork(manager, begin);
	/*
	int k =0;
	while (k<TaskList.size())
	{
		int count = 0;
		int old = TaskList[k].end - TaskList[k].start;	
		while((k<TaskList.size())&&(TaskList[k].end - TaskList[k].start == old))
		{
			write(manager.pipes[1], &TaskList[k], sizeof(Task));
			k++;
			count++;
			old = TaskList[k].end - TaskList[k].start;
		}
		
		struct sembuf buf;
		buf.sem_num = manager.current_sem;
		buf.sem_op = -count;
		buf.sem_flg = 0;//SEM_UNDO; 
		printf("!!!!!!!!!%i\n",buf.sem_op);
		if((semop(manager.work_sem, &buf, 1)) < 0) {
			perror("semop");
			exit(EXIT_FAILURE);
		}
		
		
		//manager.last_ind_sem += count;
		
	}
	
	
   //del sem
   */
   semctl(manager.work_sem, 0, IPC_RMID);
 
   sleep(1);
   manager.adr = shmat(manager.sharm_id,0,0);
		manager.items = (string **)manager.adr;
		for(int i =0; i<manager.size_items; i++)
		{
			printf("%s\n",manager.items[i]->c_str());
		}
	
		shmdt(manager.adr); 
   printf("the end\n");
   return 0;
}

void chWork(TaskManager &manager, Task task)
{

	pid_t cid = fork();
		if (cid  == 0)
		{
			//printf("%i = %i\n",i,cid);
			//simplemerge(manager);
			createTaskQueue(manager, task);
			write(manager.pipes[1],&task,sizeof(Task));
			printf("+++++++++++++++++++++++++++\n");
			exit(0);
			
			
		} else if(cid == 1)
		{
			
		}
	
	for (int i = 0; i < manager.num_of_process; i++)
	{
		pid_t cid = fork();
		if (cid  == 0)
		{
			printf("%i = %i\n",i,cid);
			simplemerge(manager);
			exit(0);
			
			
		}  if(cid == 1)
		{
			
		}
	}
	
}


void simplemerge(TaskManager &manager)
{
	Task task;
	read(manager.pipes[0],&task,sizeof(Task));
	while(!manager.finish_work)
	{
		
		 task.middle = task.start + (task.end - task.start)/2;
		  //printf("st = %i end = %i\n",task.start, task.end );
		 
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
    		
    		struct sembuf buf;
			buf.sem_num = manager.current_sem;
			buf.sem_op = 1;
			buf.sem_flg = IPC_NOWAIT; 
			if((semop(manager.work_sem, &buf, 1)) < 0) {
				perror("semop1");
				exit(EXIT_FAILURE);
			}
			
    		//printf("inc\n");
    		if(task.finish == 1)
    		{
    			 manager.finish_work = 1;
    			 return;
    		}	
    		read(manager.pipes[0],&task,sizeof(Task));
    		if(task.finish) printf("ffff!!!\n");
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
        
        write(manager.pipes[1], &left, sizeof(Task));
        write(manager.pipes[1], &right, sizeof(Task));
       
      
      if(right)
      {
		   struct sembuf buf;
			buf.sem_num = manager.current_sem;
			buf.sem_op = -manager.taskcount;
			buf.sem_flg = 0;//SEM_UNDO; 
			printf("!!!!!!!!!%i\n",buf.sem_op);
			if((semop(manager.work_sem, &buf, 1)) < 0) {
				perror("semop");
				exit(EXIT_FAILURE);
			}
			manager.taskcount = 0;
		}
        /*
        res.push_back(left);
        printf("lef was add    %i  %i ", left.start, left.end);
        if (left.finish) 
   	  {
   		 printf("true\n");
   	  }
   	  else printf("false\n");
        /.push_back(right);
			printf("right was add %i  %i ", right.start, right.end);
			if (left.finish) 
   	  {
   		 printf("true\n");
   	  }
   	  else printf("false\n");
   	  */
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
