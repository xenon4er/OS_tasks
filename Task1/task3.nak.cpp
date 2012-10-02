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
        pthread_mutex_lock( &manager->taskqueuemutex );
        if (!manager->tasks.empty())
        {
            Task tk = manager->tasks.front();
            manager->tasks.pop();
            pthread_mutex_unlock( &manager->taskqueuemutex );   
            //printf("%i <-\n", tk.start);
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
            //printf("lock zone\n");        
            pthread_cond_wait(&manager->finishwork, &manager->finishmutex);
            pthread_mutex_unlock( &manager->finishmutex );
        }
    }
}

void InitManager(TaskManager &manager, int num_of_threads)
{
    manager.waitmutex = PTHREAD_MUTEX_INITIALIZER;
    manager.taskqueuemutex = PTHREAD_MUTEX_INITIALIZER;
    manager.finishmutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_cond_init(&manager.taskcomplete, 0);
    pthread_cond_init(&manager.finishwork, 0);    
    manager.threads = new pthread_t[num_of_threads];
    manager.finish = false;
    int t;
	for (t = 0; t < num_of_threads; t++)
	{
		pthread_create(&manager.threads[t], NULL, threadwork, (void *)&manager);
	}
    
}

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        printf("usage: ./sort <input> <output> <num of thread>");
        return 0;
    }
    TaskManager manager;
    InitManager(manager, atoi(argv[3]));
    manager.arr = ReadFromFile(argv[1]);
    printarr(manager.arr);
    printf("start\n");
    Task begin;
    begin.start = 0;
    begin.end = manager.arr.size();
    vector<Task> TaskList;
    mergesort(manager.arr,begin, TaskList);
    //printf("ass\n");
    for(int i=0; i<TaskList.size();i++)
    {
        printf("s=%i e=%i \n",TaskList[i].start,TaskList[i].end);
    }
    //return 0;
    int k = 0;
    while ( k < TaskList.size())
    {
        manager.taskcounter = 0;
        int step = steps[k];
        for (int i = 0; i < manager.arr.size() / step; i++)
        {
            Task task;
            task.start = i * step;
            printf("st=%i\n", task.start);
            //task.middle = task.start + step/ 2;
            //printf("md=%i\n", task.middle);            
            task.end = task.start + step;
            printf("en=%i\n", task.end);            
            manager.tasks.push(task);
            manager.taskcounter++;
        } 
        //return 0;
        printf("start cointer =%i\n", manager.taskcounter);
        pthread_cond_signal(&manager.finishwork);
        //
        while (manager.taskcounter > 0)
        {
            //printf("oppa!\n");
            //pthread_mutex_lock( &manager.waitmutex );                
            pthread_cond_wait(&manager.taskcomplete, &manager.waitmutex);
            //manager.taskcounter--;
            //printf("->%i\n", manager.taskcounter);
            //pthread_mutex_unlock( &manager.waitmutex );
        }
    }
    //simplemerge(manager.arr, task);
    printarr(manager.arr);    
    printf("the end\n");
    return 0;
}

void simplemerge(vector<string> &arr, Task task)
{
    vector<string> temp;
    task.middle = task.start + (task.end - task.start)/2;
    int first = task.start; 
    int second = task.middle;
    while ((task.middle > first) && (task.end > second))
    {
        //printf("f=%i s=%i a1=%s a2=%s\n", first, second, arr[first].c_str(), arr[second].c_str());
        if (arr[first] > arr[second])
        {
            temp.push_back(arr[second]);
            second++;
        }       
        else
        {
            temp.push_back(arr[first]);
            first++;
        }
        //printarr(temp);
    }
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
    //printarr(temp);
}

void mergesort(vector<string> &arr, Task task, vector<Task> &res)
{
    int len = task.end - task.start;
    //printf("len= %i ",len);
    if (len <= 2)
    {
        
    }
    else
    {
        
        int middle = task.start + len/2;
        
        Task left;
        left.start = task.start;
        left.end = middle;
        Task right;
        right.start = middle;
        right.end = task.end;
       /// printf("ls= %i le= %i rs= %i re= %i \n",left.start,left.end,right.start,right.end);      
        //if (middle == 2)
        //return;
        mergesort(arr, left, res);
        mergesort(arr, right, res);                  
        res.push_back(left);
        res.push_back(right);

    }
}

/*
function mergesort(m)
    var list left, right, result
    if length(m) ≤ 1
        return m
    else
        middle = length(m) / 2
        for each x in m up to middle
            add x to left
        for each x in m after middle
            add x to right
        left = mergesort(left)
        right = mergesort(right)
        result = merge(left, right)
        return result
    end if
*/

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

void printarr(vector<string> &arr)
{
    for (int i = 0; i < arr.size(); i++)
    {
        printf("%i %s\n", i, arr[i].c_str());
    }
}

