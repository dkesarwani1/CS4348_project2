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
counting_semaphore<50> customerready(0); // semaphore to signal when a customer is ready to be served
mutex coutMutex; // mutex to protect cout
mutex servedlock; // mutex to protect access to the served customers count
int servedCustomers = 0; // count of customers served
atomic<bool> bankClosed(false);
queue <int> customerQueue; // queue to hold waiting customers
mutex queueMutex; // mutex to protect access to the customer queue
vector<binary_semaphore*> customerDone(50); // per-customer semaphore to signal when teller is done serving them
void customer(int id)
{
    // Simulate customer arrival time
    this_thread::sleep_for(chrono::milliseconds(rand() % 1000));
    { lock_guard<mutex> lock(coutMutex); cout << "Customer " << id << " has arrived." << endl; }

    door.acquire(); // enter the bank (max 2 at a time)
    { lock_guard<mutex> lock(coutMutex); cout << "Customer " << id << " has entered the bank." << endl; }

    {
        lock_guard<mutex> lock(queueMutex);
        customerQueue.push(id); // join the queue
    }
    customerready.release(); // signal a teller that a customer is ready

    customerDone[id]->acquire(); // wait until teller finishes serving me

    { lock_guard<mutex> lock(coutMutex); cout << "Customer " << id << " is leaving the bank." << endl; }
    door.release(); // exit the bank
}
void teller(int id)
{
    while (true) {
        // Simulate teller availability
        { lock_guard<mutex> lock(coutMutex); cout << "Teller " << id << " is ready to serve." << endl; }
        // Wait for a customer to be ready
        customerready.acquire();
        if (bankClosed) break; // all customers served, exit
        // Get the next customer from the queue
        int customerId;
        {
            lock_guard<mutex> lock(queueMutex);
            if (!customerQueue.empty()) {
                customerId = customerQueue.front();
                customerQueue.pop();
            } else {
                continue; // No customers in the queue, continue waiting
            }
        }
        { lock_guard<mutex> lock(coutMutex); cout << "Teller " << id << " is serving customer " << customerId << "." << endl; }
            // Simulate service time
        this_thread::sleep_for(chrono::milliseconds(rand() % 2000));
        { lock_guard<mutex> lock(coutMutex); cout << "Teller " << id << " has finished serving customer " << customerId << "." << endl; }

        string transactiontype = (rand() % 2 == 0) ? "Deposit" : "Withdrawal";
        { lock_guard<mutex> lock(coutMutex); cout << "Customer " << customerId << " is performing a " << transactiontype << "." << endl; }
        // Simulate transaction time
        this_thread::sleep_for(chrono::milliseconds(rand() % 2000));
        if(transactiontype == "Withdrawal")
        {
           { lock_guard<mutex> lock(coutMutex); cout << "Teller " << id << " Serving Customer " << customerId << " is waiting for Manager." << endl; }
              manager.acquire();
            { lock_guard<mutex> lock(coutMutex); cout << "Manager is now serving Teller " << id << " with Customer " << customerId << " for withdrawal." << endl; }
            this_thread::sleep_for(chrono::milliseconds(rand() % 26 + 5));
            { lock_guard<mutex> lock(coutMutex); cout << "Manager has finished serving Teller " << id << " with Customer " << customerId << " for withdrawal." << endl; }
            manager.release();
        }
        { lock_guard<mutex> lock(coutMutex); cout << "Teller " << id << " going to safe with Customer " << customerId << " for " << transactiontype << "." << endl; }
        safe.acquire();
        { lock_guard<mutex> lock(coutMutex); cout << "Teller " << id << " is at the safe with Customer " << customerId << " for " << transactiontype << "." << endl; }
        this_thread::sleep_for(chrono::milliseconds(rand()%41 + 10)); // Simulate time at the safe
        { lock_guard<mutex> lock(coutMutex); cout << "Teller " << id << " has finished at the safe with Customer " << customerId << " for " << transactiontype << "." << endl; }
        safe.release();
        { lock_guard<mutex> lock(coutMutex); cout << "Teller " << id << " has completed the transaction for customer " << customerId << "." << endl; }

        customerDone[customerId]->release(); // signal the customer that they have been served

        {
            lock_guard<mutex> lock(servedlock);
            servedCustomers++;
            if(servedCustomers == 50)
            {
                { lock_guard<mutex> lock(coutMutex); cout << "All customers have been served. Teller " << id << " is closing." << endl; }
                bankClosed = true;
                customerready.release(); // wake remaining blocked tellers
                customerready.release();
                break;
            }
        }

    }

}
int main()
{
    // Initialize per-customer semaphores
    for(int i = 0; i < 50; i++)
        customerDone[i] = new binary_semaphore(0);

    vector <thread> tellers, customers;
    for(int i=0;i<3;i++)
    {
        thread t(teller,i+1);
        tellers.push_back(std::move(t));
    }
    for(int x=0;x<50;x++)
    {
        thread c(customer,x);
        customers.push_back(std::move(c));
    }

    for(auto& t : tellers) t.join();
    for(auto& c : customers) c.join();

    // Cleanup
    for(int i = 0; i < 50; i++)
        delete customerDone[i];

    return 0;
}
