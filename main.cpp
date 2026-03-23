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
using namespace std;

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