#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <random>
#include <chrono>
#include <atomic>
#include <string>
#include <semaphore>
using namespace std;
counting_semaphore<2> door(2); // door allows 2 customers at a time
counting_semaphore<1> manager(1);//1 manager available
counting_semaphore<2> safe(2);// safe allows 2 customers at a time
counting_semaphore<> customerready();
queue <int> customerQueue; // queue to hold waiting customers
mutex queueMutex; // mutex to protect access to the customer queue
void customer(int id)
{
    // Simulate customer arrival time
    this_thread::sleep_for(chrono::milliseconds(rand() % 1000));
    cout << "Customer " << id << " has arrived." << endl;
    // Simulate service time
    this_thread::sleep_for(chrono::milliseconds(rand() % 2000));
    cout << "Customer " << id << " has been served." << endl;
}
void teller(int id)
{    
    while (true) {
        // Simulate teller availability
        
    }
}
int main() 
{
    vector <thread> tellers,customers;
    for(int i=0;i<3;i++)
    {
        thread t(teller,i);
        teller.push_back(std::move(t));
    
    
    } 
    for(int x=0;x<50;x++)
    {
        thread c(customer,x);
        
            customers.push_back(std::move(c));
        
        
    }
    return 0;
}