#include <pthread.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <queue>
#include <string>

using namespace std;

struct Task
{
    int start;
    int middle;
    int end;
};

vector<string> ReadFromFile(char* filename);
void printarr(vector<string> &arr);
void simplemerge(vector<string> &arr, Task task);
void mergesort(vector<string> &arr, Task task, vector<Task> &res);
void WriteInFile(char* filename, vector<string> arr);

struct TaskManager
{
    vector<string> arr;
    queue<Task> tasks;
    pthread_t *threads;
    int taskcounter;
    
    pthread_mutex_t taskqueuemutex;
    pthread_mutex_t waitmutex;
    pthread_mutex_t finishmutex;
      
    pthread_cond_t taskcomplete;
    pthread_cond_t finishwork;
      
    int num_of_threads;
    bool finish;
};

void *threadwork(void *data)
{
    TaskManager *manager = (TaskManager *)data;
    while (! manager->finish)
    {
        pthread_mutex_lock( &manager->taskqueuemutex);
        if (!manager->tasks.empty())
        {
            Task tk = manager->tasks.front();
            manager->tasks.pop();
            printf("%i \n",(int)pthread_self());
            pthread_mutex_unlock( &manager->taskqueuemutex );   
            
            simplemerge(manager->arr, tk);   
            
            pthread_mutex_lock( &manager->waitmutex );   
            manager->taskcounter--; 
            pthread_cond_signal(&manager->taskcomplete); 
            pthread_mutex_unlock( &manager->waitmutex );     
        }
        else
        {
            pthread_mutex_unlock( &manager->taskqueuemutex );        
            pthread_mutex_lock( &manager->finishmutex );                
            pthread_cond_wait(&manager->finishwork, &manager->finishmutex);           
            pthread_mutex_unlock( &manager->finishmutex );
        }
    }
    
}

void InitManager(TaskManager &manager)
{
    manager.waitmutex = PTHREAD_MUTEX_INITIALIZER;
    manager.taskqueuemutex = PTHREAD_MUTEX_INITIALIZER;
    manager.finishmutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_cond_init(&manager.taskcomplete, 0);
    pthread_cond_init(&manager.finishwork, 0);    
    
    manager.threads = new pthread_t[manager.num_of_threads];
    manager.finish = false;
    int t;
	for (t = 0; t < manager.num_of_threads; t++)
	{

		pthread_create(&manager.threads[t], NULL, threadwork, (void *)&manager);
	}
	
	printf("init complited\n");
    
}

int main(int argc, char** argv)
{   


    const int argLim = 4;
    if (argc != argLim)
    {
        printf("usage: ./sort <input> <output> <num of thread>");
        return 0;
    }
    TaskManager manager;
    manager.num_of_threads = atoi(argv[3]);
    InitManager(manager);
    manager.arr = ReadFromFile(argv[1]);
     
 
    printf("start\n");
    Task begin;
    begin.start = 0;
    begin.end = manager.arr.size();
    
    vector<Task> TaskList;
    
    //create tasks
    mergesort(manager.arr,begin, TaskList);
    TaskList.push_back(begin);
    
    
    int k = 0;

    while ( k < TaskList.size())
    {
        manager.taskcounter = 0;
        
        int old = TaskList[k].end - TaskList[k].start;
        
        while (( k < TaskList.size()) && (TaskList[k].end - TaskList[k].start == old))
        {
            manager.tasks.push(TaskList[k]);
            manager.taskcounter++;
            old = TaskList[k].end - TaskList[k].start;
            k++;
        }
        printf("%i\n",manager.taskcounter);
        
        pthread_cond_signal(&manager.finishwork);
        

        while (manager.taskcounter > 0)
        {
            pthread_cond_wait(&manager.taskcomplete, &manager.waitmutex);
        }

        
    }
    manager.finish = true;
    printf("before join\n");
    
  	for(int z = 0; z < manager.num_of_threads; z++)
  	{
  	    printf("th %i",z);
  	    pthread_detach(manager.threads[z]);
  		//pthread_join(manager.threads[z], NULL);
  	}
    
    
    
    printarr(manager.arr);
    WriteInFile(argv[2], manager.arr);    
    printf("the end\n");
    return 0;
}

void simplemerge(vector<string> &arr, Task task)
{
    vector<string> temp;
    task.middle = task.start + (task.end - task.start)/2;
    
    
    string tmp;
    
    int first = task.start; 
    int second = task.middle;
    while ((task.middle > first) && (task.end > second))
    {
        //printf("f=%i s=%i a1=%s a2=%s\n", first, second, arr[first].c_str(), arr[second].c_str());
        if (arr[first] > arr[second])
        {
            tmp = arr[second];
            for(int i=0; second-i>first;i++)
            {
                arr[second-i] = arr[second-i-1];
            }
            arr[first] = tmp;
            task.middle++;
            //temp.push_back(arr[second]);
            second++;
        }       
        else
        {
            //temp.push_back(arr[first]);
            first++;
        }
        
        //printarr(temp);
    }
    
    /*
    if (task.middle > first)
    {
        for (int i = first; i < task.middle; i++)
            temp.push_back(arr[i]);
    }
    else
    {
        for (int i = second; i < task.end; i++)
            temp.push_back(arr[i]);
    }
    for (int i = task.start; i < task.end; i++)
    {
        arr[i] = temp[i - task.start];
    }
    */
    //printarr(temp);
}

void mergesort(vector<string> &arr, Task task, vector<Task> &res)
{
    int len = task.end - task.start;
    //printf("len= %i ",len);
    const int lenLim = 2;
    if (len > lenLim)
    {
        
        int middle = task.start + len/2;
        
        Task left;
        left.start = task.start;
        left.end = middle;
        Task right;
        right.start = middle;
        right.end = task.end;
       
        mergesort(arr, left, res);
        mergesort(arr, right, res);                  
        res.push_back(left);
        res.push_back(right);

    }
}

vector<string> ReadFromFile(char* filename)
{
    vector<string> res;
    ifstream fs;
    //int i = 1;
    fs.open(filename);
    string s;
    while (std::getline(fs, s))
    {
        res.push_back(s);
    }
    fs.close();
    return res;
}

void WriteInFile(char* filename, vector<string> arr)
{
    ofstream out;
    out.open(filename);
    for(int i=0; i<arr.size();i++)
    {
        out<<arr[i]<<endl;
        i++;
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

