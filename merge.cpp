#include "pool.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
using namespace std;

//初始化threadNum个线程数的Pool.
Pool::Pool(int threadNum)
{
    pDestroyAll = false;
    CreateAll(threadNum);
}

//析构
Pool::~Pool()
{
    printf("Delete Pool\n");
    DestroyAll();
}

//线程所运行的函数.每个线程均应该执行ThreadFunc函数.
void* Pool::ThreadFunc(void *threadData)
{
    if ( threadData == nullptr ) {
        printf("Thread data is null.");
        return NULL;
    }
    //将'threadData'指针铸造为Pool指针'PoolData',以便使用Pool中的方法
    Pool* PoolData = (Pool*)threadData;
    while (true)
    {
        Task* pTask = nullptr;
        //抢夺锁以保护临界区
        pthread_mutex_lock(&PoolData->pMutex);
        //***************临界区开始**************
        while (PoolData->pTaskList.empty()&&!PoolData->pDestroyAll)
        {
            pthread_cond_wait(&PoolData->pCond,&PoolData->pMutex);
        }
        if (!PoolData->pTaskList.empty())
        {
            //如果TaskList不是空的,那么就干嘛呢?
            auto iter = PoolData->pTaskList.begin();
            pTask = *iter;
            PoolData->pTaskList.erase(iter);
        }
        //****************临界区结束**************
        pthread_mutex_unlock(&PoolData->pMutex);

        if (pTask!=nullptr)
        {
            pTask->Run();
            pTask->isFinished = true;
        }

        if (PoolData->pDestroyAll && PoolData->pTaskList.empty())
        {
            printf("Thread exit");
            pthread_exit(NULL);
            break;
        }
    }
    return NULL;
}


void Pool::CreateAll(int num)
{
    //初始化互斥锁pMutex和条件变量pCond,重置pTid <vector>数组大小
    pthread_mutex_init(&pMutex,NULL);
    pthread_cond_init(&pCond,NULL);
    pTid.resize(num);
    //使用pthread_create方法创建pthread线程
    for (int i=0;i<num;++i)
    {
        pthread_create(&pTid[i],NULL,ThreadFunc,(void *)this);
        printf("%d start",i);
    }
}

//向Pool中添加任务.
void Pool::AddTask(Task* task)
{
    //显然,AddTask和threadFunc之间有读写者问题,适用生产者-消费者(P-C)模型
    //添加任务时需要先给mutex上锁.保护临界区
    pthread_mutex_lock(&pMutex);
    //加入任务.
    pTaskList.push_back(task);
    //wait&signal机制.
    pthread_cond_signal(&pCond);
    //释放锁.
    pthread_mutex_unlock(&pMutex);
    //<?1>:这里unlock和signal的顺序可以颠倒吗?
}

int Pool::GetTaskSize(){
    pthread_mutex_lock(&pMutex);
    int taskSize = pTaskList.size();
    pthread_cond_signal(&pCond);
    pthread_mutex_unlock(&pMutex);
    return taskSize;
}


void Pool::DestroyAll()
{
    if (!pDestroyAll)
    {
        //设置线程销毁标志,由销毁标志触发pthread_exit,退出线程
        pDestroyAll = true;
        printf("start to destroy");

        //唤醒所有线程,此时由于触发pDestroy=true条件的判断会依次自动销毁.
        pthread_cond_broadcast(&pCond);

        //清除cond,mutex,pTid
        for ( int i = 0;i<pTid.size();++i)
        {
            //pthread_join是以阻塞的方式等待指定线程结束.
            pthread_join(pTid[i],NULL);
            printf("Destroy thread %d",i);
        }
        pTid.clear();
        pthread_cond_destroy(&pCond);
        pthread_mutex_destroy(&pMutex);
    }
}


struct Data {
    int id;
    int age;
    float weight;
    float height;
    std::string name;
    std::string address;
    std::string email;
    std::string phone;
    std::string city;
    std::string country;
    int salary;
    int years_of_experience;
    bool employed;
    float rating;
    float score;
};

// Compare function for std::sort by score
bool compare_score(const Data& a, const Data& b) {
    return a.score < b.score;
}

// Validation function to check if the array is sorted correctly by score
bool validate_score(const std::vector<Data>& array) {
    for (size_t i = 1; i < array.size(); i++) {
        if (array[i].score < array[i - 1].score)
            return false; // Array is not sorted correctly
    }
    return true; // Array is sorted correctly
}

void waitAllTasks(Pool* pool){
    while (true)
    {
        if(pool->GetTaskSize() == 0){
            break;
        }else{
            continue;
        }
    } 
}

class BlockSort : public Task
{
public:
    bool done;
    //实现Task中的virtual函数
    void Run()
    {
        //***********************************************************需要填补的第一个位置,如何操作自己的pData***********************************************************
        //pData是继承自Task的protected属性,通过setData直接设置
        //示例:
        // std::sort(pData->?,pData->?, compare_func);
        if (pData != nullptr) {
            std::vector<Data>* dataVec = static_cast<std::vector<Data>*>(pData);
            std::sort(dataVec->begin()+start_index, dataVec->begin()+end_index, compare_score);
        }
        done = true;        //done变量用于判断Task是否已经完成,在Run()最后设置 = true
        //***********************************************************需要填补的第一个位置,如何操作自己的pData***********************************************************
    }

};

class MergeTask : public Task {
public:
    bool done;
    // 实现 Task 中的虚函数 Run
    void Run() override {
        std::vector<Data>* dataVec = static_cast<std::vector<Data>*>(pData);
        std::inplace_merge(dataVec->begin() + start_index,
                           dataVec->begin() + middle_index,
                           dataVec->begin() + end_index,
                           compare_score);
    done = true;
    }
};

int main(int argc, char *argv[]){

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <array_size>" << std::endl;
        return 1;
    }
    uint64_t array_size = std::strtoull(argv[1], nullptr, 10);
    if (array_size == 0) {
        std::cout << "Array size must be a positive integer" << std::endl;
        return 1;
    }

    std::vector<Data> array(array_size);
        // Initialize data
    srand(time(NULL)); // Seed the random number generator
    for (uint64_t i = 0; i < array_size; i++) {
        array[i].id = i + 1;
        array[i].age = rand() % 100 + 1; // Random age between 1 and 100
        array[i].weight = static_cast<float>(rand()) / RAND_MAX * 100.0f; // Random weight between 0 and 100
        array[i].height = static_cast<float>(rand()) / RAND_MAX * 200.0f; // Random height between 0 and 200
        array[i].name = "Name" + std::to_string(i);
        array[i].address = "Address" + std::to_string(i);
        array[i].email = "Email" + std::to_string(i) + "@example.com";
        array[i].phone = "+1234567890" + std::to_string(i);
        array[i].city = "City" + std::to_string(i);
        array[i].country = "Country" + std::to_string(i);
        array[i].years_of_experience = rand() % 30; // Random years of experience between 0 and 29
        array[i].employed = rand() % 2; // Random employed status (0 or 1)
        array[i].rating = static_cast<float>(rand()) / RAND_MAX * 5.0f; // Random rating between 0 and 5
        array[i].score = static_cast<float>(rand()) / RAND_MAX * 11000000.0f - 1000000.0f; // Random score between -1000000 and 10000000
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    int num_thread = 8;
    int num_tasks = 1000;
    std::vector<uint64_t> ranges(num_tasks+1);
    ranges[0] = 0;
    Pool* pool = new Pool(num_thread);
    BlockSort* pTaskList  = new BlockSort[num_tasks];
    // 计算每个块的大小
    uint64_t block_size = array_size / num_tasks;
    // 为每个任务分配数据
    for (int index = 0; index < num_tasks; ++index) {
        // 计算当前任务应该处理的数据的起始索引和结束索引
        uint64_t start_index = index * block_size;
        uint64_t end_index = (index == num_tasks-1) ? array_size : (start_index + block_size);
        //添加归并的边界
        ranges[index+1] = end_index;
        // 为当前任务设置数据
        pTaskList[index].setData(&array,start_index,0,end_index);

        // 将当前任务添加到线程池
        pool->AddTask(&pTaskList[index]);
    }
    //简单轮询所有线程是否已经完成,显然可以优化
    waitAllTasks(pool);
    while(ranges.size()>2){
        uint64_t num_merge = (ranges.size()-1)/2;
        std::cout<<endl<<num_merge<<endl;
        MergeTask* mergeList = new MergeTask[num_merge];
        std::cout<<endl<<"-------------------------------------"<<endl;
        
        for (int index = 0; index < num_merge; ++index) {  
            // 为当前任务设置数据
            // std::cout<<ranges[index]<<" "<<ranges[index+1]<<" "<<ranges[index+2]<<endl;
            mergeList[index].setData(&array,ranges[index],ranges[index+1],ranges[index+2]);
            // for(int i =0;i<ranges.size();i++){
            //         std::cout<<ranges[i]<<"  ";
            //     }
                // std::cout<<endl<<"-------------------------------------"<<endl;
            ranges.erase(ranges.begin()+index+1);
                
          // 将当前任务添加到线程池
            pool->AddTask(&mergeList[index]);
        }
        // for(int i =0;i<ranges.size();i++){
        //             std::cout<<ranges[i]<<"  ";
        //         }
                // std::cout<<endl<<"-------------------------------------"<<endl;
        waitAllTasks(pool);
    }
    waitAllTasks(pool);
   
    // //激活析构
    // delete pool;
    // delete []pTaskList;

    // End timer
    auto end_time = std::chrono::high_resolution_clock::now();

    double time_spent = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();

    // Print time taken
    std::cout << "Time taken: " << time_spent << " seconds" << std::endl;
    // std::vector<Data> array2(1000000);


    // for(int j=0;j<10;j++){
    //        // 用初始值填充前1000个元素
    //     for (size_t i = 0; i < 1000000; ++i) {
    //     array2[i] = array[1000000*j+i]; // 假设Data的构造函数可以接受int值
    //     }
    //     if(validate_score(array2))
    //         std::cout << "Validation: Array is sorted correctly by score" << std::endl;
    //     else
    //         std::cout << "Validation: Array is not sorted correctly by score" << std::endl;      
        
    // }

 

    if (validate_score(array))
        std::cout << "Validation: Array is sorted correctly by score" << std::endl;
    else
        std::cout << "Validation: Array is not sorted correctly by score" << std::endl;


    return 1;
}


// get end clock